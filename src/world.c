#include "ph.h"

void phWorldDump(phworld *self) {
  phDump(&self->particles, NULL);
  phFree(self->particleArray.items);
  phDump(&self->particleData, free);
  phDump(&self->constraints.beforeStep, NULL);
  phDump(&self->constraints.afterStep, NULL);
  phDdvtDump(&self->_optimization.ddvt);
  phDump(&self->_optimization.collisions.collisions, free);
  phDump(&self->_optimization.collisions.nextCollisions, free);
  phDump(&self->_garbage.garbage, free);
  phDump(&self->_garbage.next, free);
}

static phcollision *_phNextCollision(phcollisionlist *self) {
  phcollision *col;
  // Get the first collision or create a new one.
  if (self->nextCollisions.first) {
    col = self->nextCollisions.first->item;
  } else {
    col = phAlloc(phcollision);
    phAppend(&self->nextCollisions, col);
  }
  return col;
}

static void _phCollisionsPushFreeList(
  phcollisionlist *self, phlist *collisions
) {
  if (!collisions->freeList) {
    self->nextCollisions.freeList =
      (collisions->freeList =
        self->nextCollisions.freeList)->next;
    collisions->freeList->next = NULL;
  }
}

static void _phCollisionsUnloadFreeList(
  phcollisionlist *self, phlist *collisions
) {
  // Empty collisions into nextCollisions.
  phlist *nextCollisions = &self->nextCollisions;
  if (collisions->length) {
    collisions->last->next = nextCollisions->first;
    if (nextCollisions->first) {
      nextCollisions->first->prev = collisions->last;
    }
    nextCollisions->first = collisions->first;
    if (!nextCollisions->last) {
      nextCollisions->last = collisions->last;
    }
    collisions->first = NULL;
    collisions->last = NULL;
    nextCollisions->length += collisions->length;
    collisions->length = 0;
  }
}

static void _phSaveCollision(phcollisionlist *self) {
  phcollision *col = phShift(&self->nextCollisions);
  if (col->isNormal) {
    // phparticle *a = col->a;
    // phparticle *b = col->b;
    // if (a->_worldData.inLeafDdvt && b->_worldData.inLeafDdvt) {
    //   _phCollisionsPushFreeList(self, &self->inBoxCollisions);
    //   phAppend(&self->inBoxCollisions, col);
    // } else {
    //   _phCollisionsPushFreeList(self, &self->outBoxCollisions);
    //   phAppend(&self->outBoxCollisions, col);
    // }
    _phCollisionsPushFreeList(self, &self->normalCollisions);
    phAppend(&self->normalCollisions, col);
  } else if (col->isStatic) {
    _phCollisionsPushFreeList(self, &self->staticCollisions);
    phAppend(&self->staticCollisions, col);
  } else {
    _phCollisionsPushFreeList(self, &self->triggerCollisions);
    phAppend(&self->triggerCollisions, col);
  }
}

static void _phResetCollisions(phcollisionlist *self) {
  // _phCollisionsUnloadFreeList(self, &self->inBoxCollisions);
  // phClean(&self->inBoxCollisions, NULL);
  // _phCollisionsUnloadFreeList(self, &self->outBoxCollisions);
  // phClean(&self->outBoxCollisions, NULL);
  _phCollisionsUnloadFreeList(self, &self->normalCollisions);
  phClean(&self->normalCollisions, NULL);
  _phCollisionsUnloadFreeList(self, &self->staticCollisions);
  phClean(&self->staticCollisions, NULL);
  _phCollisionsUnloadFreeList(self, &self->triggerCollisions);
  phClean(&self->triggerCollisions, NULL);
}

static void _phSolveUnthreadedCollisisions(phcollisionlist *self) {
  phlistiterator _litr;
  // phIterator(&self->outBoxCollisions, &_litr);
  // while (phListNext(&_litr)) {
  //   // phcollision *col = phListDeref((phlistiterator *) itr);
  //   phcollision *col = _litr.node->item;
  //   phSolve(col);
  // }
  phIterator(&self->normalCollisions, &_litr);
  while (phListNext(&_litr)) {
    // phcollision *col = phListDeref((phlistiterator *) itr);
    phcollision *col = _litr.node->item;
    phSolve(col);
  }
  phIterator(&self->staticCollisions, &_litr);
  while (phListNext(&_litr)) {
    // phcollision *col = phListDeref((phlistiterator *) itr);
    phcollision *col = _litr.node->item;
    phSolveStatic(col);
  }
  phIterator(&self->triggerCollisions, &_litr);
  while (phListNext(&_litr)) {
    // phcollision *col = phListDeref((phlistiterator *) itr);
    phcollision *col = _litr.node->item;
    phSolveTrigger(col);
  }
}

static void _phSolveCollisions(phcollisionlist *self) {
  // phlistiterator _litr;
  // phIterator(&self->inBoxCollisions, &_litr);
  // while (phListNext(&_litr)) {
  //   // phcollision *col = phListDeref((phlistiterator *) itr);
  //   phcollision *col = _litr.node->item;
  //   phSolve(col);
  // }

  _phSolveUnthreadedCollisisions(self);

  _phResetCollisions(self);
}

static void _phDdvtTestParticles(phddvt *self, phcollisionlist *collisions) {
  phcollision *nextCollision = _phNextCollision(collisions);
  phparticle **items = (phparticle **) self->_particleArray.items;
  // #if PH_THREAD
  // phlist *inBoxCollisions = &collisions->inBoxCollisions;
  // phlist *outBoxCollisions = &collisions->outBoxCollisions;
  // #endif
  phint length = self->length;
  for (phint i = 0; i < length; ++i) {
    phparticle *a = items[i], *_a;
    phbox boxA = a->_worldData.oldBox;
    // #if PH_THREAD
    // phbool inLeafDdvt = a->_worldData.inLeafDdvt;
    // #endif
    for (phint j = i + 1; j < length; ++j) {
      phparticle *b = items[j];
      phbox boxB = b->_worldData.oldBox;

      // Pre test with boxes, which is cheaper than circle test. Then
      // circle test.
      if (phIntersect(boxA, boxB)) {
        _a = a;
        // Make the static particle always be first.
        if (b->isStatic) {
          _a = b;
          b = a;
        }
        // Both particles are static, skip testing it further.
        if (b->isStatic) {
          continue;
        }

        if (phTest(_a, b, nextCollision)) {
          nextCollision->a = _a;
          nextCollision->b = b;
          _phSaveCollision(collisions);
          // Sorting collisions by whether they are solely contained in a ddvt
          // has a fairly large performance hit. And solving those in threads
          // doesn't counteract the hit enough to keep it.
          // #if PH_THREAD
          // if (inLeafDdvt && b->_worldData.inLeafDdvt) {
          //   phAppend(inBoxCollisions, nextCollision);
          // } else {
          //   phAppend(outBoxCollisions, nextCollision);
          // }
          // #endif
          nextCollision = _phNextCollision(collisions);
        }
      }
    }
  }
}

static void _phWorldIntegrateParticles(
  pharrayiterator *itr, phlist *shouldUpdate, phdouble dt
) {
  phparticle **items = (phparticle **) itr->items;
  phparticle **end = (phparticle **) itr->end;
  for (; items < end; ++items) {
    phparticle *particle = *items;
    phIntegrate(particle, dt);
    phTestReset(particle);
    phv position = particle->position;
    phv oldPosition = particle->_worldData.oldPosition;
    phdouble radius = particle->radius;
    // Update particle in ddvt
    if (
      phMag2(phSub(position, oldPosition)) > radius * radius / 10
    ) {
      phAppend(shouldUpdate, particle);
    }
  }
}

static void _phDdvtUpdateList(phddvt *ddvt, phlist *shouldUpdate) {
  phlistiterator _litr;
  phIterator(shouldUpdate, &_litr);
  while (phListNext(&_litr)) {
    phparticle *particle = _litr.node->item;
    phv position = particle->position;
    phbox newBox = phBoxConstrainSmaller(
      phAabb(position, particle->radius), ddvt->box
    );
    phDdvtUpdate(ddvt, particle, particle->_worldData.oldBox, newBox);
    particle->_worldData.oldPosition = position;
    particle->_worldData.oldBox = newBox;
    // particle->_worldData.inLeafDdvt =
    //   !((phddvt *) particle->_worldData.topDdvt)->tl;
  }
  phClean(shouldUpdate, NULL);
}

#if PH_THREAD

void _phParticleIntegrateThreadHandle(phparticleintegratethreaddata *self) {
  phdouble dt = ((phworld *) self->world)->timing.dt;
  _phWorldIntegrateParticles(&self->particleItr, &self->shouldUpdate, dt);
}

static void _phParticleTestDataResetDdvts(phparticletestthreaddata *self) {
  phlistiterator litr;
  phIterator(&self->ddvts, &litr);
  while (phListNext(&litr)) {
    phddvt *ddvt = phListDeref(&litr);
    phAppend(&self->freeDdvts, ddvt);
  }
  phClean(&self->ddvts, NULL);
}

static phddvt * _phParticleTestDataNextDdvt(
  phparticletestthreaddata *self, phddvt *toCopy
) {
  phddvt *ddvt = phShift(&self->freeDdvts);
  if (!ddvt) {
    ddvt = phCreate(
      phddvt, NULL, toCopy->box, toCopy->minParticles, toCopy->maxParticles
    );
    ddvt->_particleArray.items = (void**) &ddvt->_particleItems;
  }
  phAppend(&self->ddvts, ddvt);
  return ddvt;
}

void _phParticleTestThreadHandle(phparticletestthreaddata *self) {
  if (self->ddvts.length) {
    phlistiterator _litr;
    phIterator(&self->ddvts, &_litr);
    while (phListNext(&_litr)) {
      _phDdvtTestParticles(_litr.node->item, &self->collisions);
    }
    _phParticleTestDataResetDdvts(self);
    // phClean(&self->ddvts, NULL);
  }
}

void _phParticleSolveThreadHandle(phparticlesolvethreaddata *self) {
  phlistiterator _litr;
  phIterator(self->collisions, &_litr);
  while (phListNext(&_litr)) {
    phcollision *col = _litr.node->item;
    phSolve(col);
  }
}

void * _phThreadMain(void *_self) {
  phthread *self = _self;
  pthread_mutex_lock(&self->active);

  while (1) {
    if (self->signalCtrl) {
      pthread_mutex_lock(&self->ctrl->endMutex);
      pthread_cond_signal(&self->ctrl->endCond);
      pthread_mutex_unlock(&self->ctrl->endMutex);
    }
    self->activeBool = 0;
    pthread_cond_wait(&self->step, &self->active);

    self->ctrl->handle(self->data);
  }

  return NULL;
}

static void _phThreadInit(phthread *self, phthreadctrl *ctrl) {
  *self = phthread();
  pthread_cond_init(&self->step, NULL);
  pthread_mutex_init(&self->active, NULL);
  self->ctrl = ctrl;

  pthread_create(&self->thread, NULL, _phThreadMain, self);
  pthread_cond_wait(&self->ctrl->endCond, &self->ctrl->endMutex);
  pthread_mutex_lock(&self->active);
}

static void _phThreadInitGroup(phthreadctrl *self, phint num) {
  self->threads = pharray(num, calloc(num, sizeof(phthread *)));
  for (phint i = 0, l = self->threads.capacity; i < l; ++i) {
    phthread *thread = self->threads.items[i] = phAlloc(phthread);
    _phThreadInit(thread, self);
    if (!i) {
      thread->signalCtrl = 0;
    }
  }
}

static void _phThreadCtrlInit(phthreadctrl *self, phworld *w) {
  *self = phthreadctrl();
  pthread_cond_init(&self->endCond, NULL);
  pthread_mutex_init(&self->endMutex, NULL);
  pthread_mutex_lock(&self->endMutex);

  phint cpus = sysconf(_SC_NPROCESSORS_ONLN);

  _phThreadInitGroup(self, cpus);
}

static void _phParticleIntegrateDataInit(pharray *self, phint num, phworld *w) {
  *self = pharray(num, calloc(num, sizeof(phparticleintegratethreaddata *)));
  for (phint i = 0, l = self->capacity; i < l; ++i) {
    phparticleintegratethreaddata *data = self->items[i] =
      phAlloc(phparticleintegratethreaddata);
    *data = phparticleintegratethreaddata();
    data->world = w;
  }
}

static void _phParticleTestDataInit(pharray *self, phint num) {
  *self = pharray(num, calloc(num, sizeof(phparticletestthreaddata *)));
  for (phint i = 0, l = self->capacity; i < l; ++i) {
    self->items[i] = phCreate(phparticletestthreaddata);
  }
}

static void _phParticleSolveDataInit(pharray *self, phint num) {
  *self = pharray(num, calloc(num, sizeof(phparticlesolvethreaddata *)));
  for (phint i = 0, l = self->capacity; i < l; ++i) {
    phparticlesolvethreaddata *data = self->items[i] =
      phAlloc(phparticlesolvethreaddata);
    *data = phparticlesolvethreaddata();
  }
}

static void _phThreadCtrlRun(
  phthreadctrl *self, phthreadhandle handle, pharray *data
) {
  self->handle = handle;

  // Activate threads.
  phint activeThreads = data->capacity;
  for (phint i = 0, l = data->capacity; i < l; ++i) {
    phthread *thread = self->threads.items[i];
    thread->data = data->items[i];
    thread->activeBool = 1;
    pthread_mutex_unlock(&thread->active);
    pthread_cond_signal(&thread->step);
  }
  // Wait for one of the threads to say its done.
  pthread_cond_wait(&self->endCond, &self->endMutex);
  // Loop, checking whether they are active until all threads have stopped.
  pthread_mutex_unlock(&self->endMutex);
  while (activeThreads) {
    activeThreads = data->capacity;
    for (phint i = 0, l = data->capacity; i < l; ++i) {
      phthread *thread = self->threads.items[i];
      if (!thread->activeBool) {
        activeThreads--;
      }
    }
    if (!activeThreads) {
      break;
    }
    sched_yield();
  }

  // Lock all threads up for the next kickoff.
  for (phint i = 0, l = data->capacity; i < l; ++i) {
    phthread *thread = self->threads.items[i];
    pthread_mutex_lock(&thread->active);
  }
  pthread_mutex_lock(&self->endMutex);
}

static void _phSolveCollisionsWithThreads(phworld *self) {
  phlistiterator _litr;
  if (!self->particleSolveThreadData.items) {
    _phParticleSolveDataInit(
      &self->particleSolveThreadData, self->threadCtrl.threads.capacity
    );
  }
  for (phint i = 0, l = self->threadCtrl.threads.capacity; i < l; ++i) {
    phparticletestthreaddata *testData = self->particleTestThreadData.items[i];
    phparticlesolvethreaddata *solveData =
      self->particleSolveThreadData.items[i];
    solveData->collisions = &testData->collisions.inBoxCollisions;
  }
  _phThreadCtrlRun(
    &self->threadCtrl,
    (void (*)(void *)) _phParticleSolveThreadHandle,
    &self->particleSolveThreadData
  );
  for (phint i = 0, l = self->threadCtrl.threads.capacity; i < l; ++i) {
    phparticlesolvethreaddata *data = self->particleSolveThreadData.items[i];
    phparticletestthreaddata *testData = self->particleTestThreadData.items[i];
    _phSolveUnthreadedCollisisions(&testData->collisions);
    // phIterator(&testData->collisions.outBoxCollisions, &_litr);
    // while (phListNext(&_litr)) {
    //   // phcollision *col = phListDeref((phlistiterator *) itr);
    //   phcollision *col = _litr.node->item;
    //   phSolve(col);
    // }
    _phResetCollisions(&testData->collisions);
  }
}

#endif

void phWorldInternalStep(phworld *self) {
  self->timing.insideStep = 1;

  // before constraints
  phlistiterator _litr;
  phiterator *itr = phIterator(&self->constraints.beforeStep, &_litr);
  phIterate(itr, (phitrfn) phConstraintUpdate, self);

  // integrate, sleep, and update ddvt
  phddvt *ddvt = &self->_optimization.ddvt;
  #if PH_THREAD
  if (!self->threadCtrl.threads.items) {
    _phThreadCtrlInit(&self->threadCtrl, self);
  }
  if (!self->particleIntegrateThreadData.items) {
    _phParticleIntegrateDataInit(
      &self->particleIntegrateThreadData,
      self->threadCtrl.threads.capacity,
      self
    );
  }
  for (phint i = 0, l = self->threadCtrl.threads.capacity; i < l; ++i) {
    phparticleintegratethreaddata *data =
      self->particleIntegrateThreadData.items[i];
    phArrayIterator(&self->particleArray, &data->particleItr);
    phint perThread = (self->particleIndex + l - self->particleIndex % l) / l;
    data->particleItr.items = self->particleArray.items + perThread * i;
    data->particleItr.end = data->particleItr.items + perThread;
    if (
      data->particleItr.end >
        self->particleArray.items + self->particleIndex
    ) {
      data->particleItr.end = self->particleArray.items + self->particleIndex;
    }
  }
  _phThreadCtrlRun(
    &self->threadCtrl,
    (void (*)(void *)) _phParticleIntegrateThreadHandle,
    &self->particleIntegrateThreadData
  );
  for (phint i = 0, l = self->threadCtrl.threads.capacity; i < l; ++i) {
    phparticleintegratethreaddata *data =
      self->particleIntegrateThreadData.items[i];
    _phDdvtUpdateList(ddvt, &data->shouldUpdate);
  }
  #else
  phbox oldBox, newBox;
  phdouble dt = self->timing.dt;
  pharrayiterator _aitr;
  phArrayIterator(&self->particleArray, &_aitr);
  _aitr.items = self->particleArray.items;
  _aitr.end = _aitr.items + self->particleIndex;
  // De-duped integration and ddvt update code works but incurs a fair hit
  // to speed.
  // _phWorldIntegrateParticles(&_aitr, &self->_optimization.shouldUpdate, dt);
  // _phDdvtUpdateList(ddvt, &self->_optimization.shouldUpdate);
  phparticle **items = (phparticle **) _aitr.items;
  phparticle **end = (phparticle **) _aitr.end;
  for (; items < end; ++items) {
    phparticle *particle = *items;
    phIntegrate(particle, dt);
    phTestReset(particle);
    phv position = particle->position;
    phv oldPosition = particle->_worldData.oldPosition;
    phdouble radius = particle->radius;
    // Update particle in ddvt
    if (
      phMag2(phSub(position, oldPosition)) > radius * radius / 10
    ) {
      newBox = phBoxConstrainSmaller(phAabb(position, radius), ddvt->box);
      phDdvtUpdate(ddvt, particle, particle->_worldData.oldBox, newBox);
      particle->_worldData.oldPosition = position;
      particle->_worldData.oldBox = newBox;
    }
  }
  #endif

  // test and unsleep
  #if PH_THREAD
  if (!self->particleTestThreadData.items) {
    _phParticleTestDataInit(
      &self->particleTestThreadData, self->threadCtrl.threads.capacity
    );
  }
  // Build list of ddvts that threads will test.
  {
    phint i = 0;
    phint numThreads = self->threadCtrl.threads.capacity;
    phddvtpairiterator _ditr;
    phDdvtPairIterator(&self->_optimization.ddvt, &_ditr);
    while (phDdvtPairNext(&_ditr)) {
      _ditr.arrayItr1.test = 0;
      phparticletestthreaddata *data = self->particleTestThreadData.items[i];
      // Copy particles and length into ddvt clones that only that thread's
      // data will access.
      phddvt *ddvtClone = _phParticleTestDataNextDdvt(data, _ditr.ddvt);
      for (phint j = 0; j < _ditr.ddvt->length; ++j) {
        ddvtClone->_particleArray.items[j] =
          _ditr.ddvt->_particleArray.items[j];
      }
      ddvtClone->_particleArray.capacity = _ditr.ddvt->length;
      ddvtClone->length = _ditr.ddvt->length;
      // phAppend(&data->ddvts, _ditr.ddvt);
      if (++i >= numThreads) {
        i = 0;
      }
    }
  }
  _phThreadCtrlRun(
    &self->threadCtrl,
    (void (*)(void *)) _phParticleTestThreadHandle,
    &self->particleTestThreadData
  );
  #else
  phddvtpairiterator _ditr;
  itr = phDdvtPairIterator(&self->_optimization.ddvt, &_ditr);
  while (phDdvtPairNext((phddvtpairiterator *) itr)) {
    _phDdvtTestParticles(_ditr.ddvt, &self->_optimization.collisions);
    _ditr.arrayItr1.test = 0;
  }
  #endif

  // solve
  #if PH_THREAD
  for (phint i = 0, l = self->threadCtrl.threads.capacity; i < l; ++i) {
    phparticletestthreaddata *data = self->particleTestThreadData.items[i];
    _phSolveCollisions(&data->collisions);
  }
  // _phSolveCollisionsWithThreads(self);
  #else
  _phSolveCollisions(&self->_optimization.collisions);
  #endif

  // post constraint
  itr = phIterator(&self->constraints.afterStep, &_litr);
  phIterate(itr, (phitrfn) phConstraintUpdate, self);

  self->timing.insideStep = 0;

  itr = phIterator(&self->_garbage.garbage, &_litr);
  phbool isMore = phNext(itr);
  while (isMore) {
    phworldgarbage *garbage = phDeref(itr);
    isMore = phNext(itr);
    phRemove(&self->_garbage.garbage, garbage);
    phAppend(&self->_garbage.next, garbage);
    garbage->fn(self, garbage->item);
    if (garbage->freefn) {
      garbage->freefn(garbage->item);
    }
  }
}

void phWorldStep(phworld *self, phdouble dt) {
  self->timing.dtInterval += dt;
  int steps = 0;
  while (
    steps++ < self->timing.maxSubSteps &&
      self->timing.dtInterval > self->timing.dt
  ) {
    self->timing.dtInterval -= self->timing.dt;
    phWorldInternalStep(self);
  }
}

// phparticle * phWorldRayCast(phworld *, phray);
// phiterator * phWorldRayIterator(phworld *, phray, phrayiterator *);
// phiterator * phWorldBoxIterator(phworld *, phbox, phboxiterator *);

phworld * phWorldAddParticle(phworld *self, phparticle *particle) {
  // particle->_worldData = phAlloc(phparticleworlddata);
  particle->_worldData =
    phparticleworlddata(
      phBoxConstrainSmaller(
        phAabb(particle->position, particle->radius),
        self->_optimization.ddvt.box
      )
    );
  particle->_worldData.oldPosition = particle->position;
  phAppend(&self->particles, particle);
  if (self->particleArray.capacity == self->particleIndex) {
    phResizeItems(&self->particleArray, 1024);
  }
  self->particleArray.items[self->particleIndex++] = particle;
  phDdvtAdd(&self->_optimization.ddvt, particle);
  // particle->_worldData.inLeafDdvt =
  //   !((phddvt *) particle->_worldData.topDdvt)->tl;
  return self;
}

phworld * phWorldRemoveParticle(phworld *self, phparticle *particle) {
  phbox oldBox = particle->_worldData.oldBox;
  phDdvtRemove(&self->_optimization.ddvt, particle, oldBox);
  phRemove(&self->particles, particle);
  for (phint i = 0, l = self->particleIndex; i < l; ++i) {
    if (self->particleArray.items[i] == particle) {
      self->particleArray.items[i] =
        self->particleArray.items[--self->particleIndex];
      break;
    }
  }
  phParticleWorldDataDump(&particle->_worldData);
  // phFree(particle->_worldData);
  // particle->_worldData = NULL;
  return self;
}

phworldgarbage * _phWorldNextGarbage(phworld *self) {
  phlistiterator _litr;
  phiterator *itr = phIterator(&self->_garbage.next, &_litr);
  phworldgarbage *garbage;
  if (phNext(itr)) {
    garbage = phDeref(itr);
    phRemove(&self->_garbage.next, garbage);
  } else {
    garbage = phAlloc(phworldgarbage);
  }
  phAppend(&self->_garbage.garbage, garbage);
  return garbage;
}

phworld * phAfterStep(phworld *self, phitrfn fn, phfreefn freefn, void *item) {
  if (self->timing.insideStep) {
    phworldgarbage *garbage = _phWorldNextGarbage(self);
    *garbage = phworldgarbage(item, fn, freefn);
  } else {
    fn(self, item);
    if (freefn) {
      freefn(item);
    }
  }
  return self;
}

phworld * phWorldSafeRemoveParticle(
  phworld *self, phparticle *particle, phfreefn freefn
) {
  return phAfterStep(
    self, (phitrfn) phWorldRemoveParticle, freefn, particle
  );
}

phworld * phWorldAddConstraint(phworld *self, phconstraint *constraint) {
  if (phConstraintBeforeStep(constraint)) {
    phAppend(&self->constraints.beforeStep, constraint);
  } else {
    phAppend(&self->constraints.afterStep, constraint);
  }
  return self;
}

phworld * phWorldRemoveConstraint(phworld *self, phconstraint *constraint) {
  if (phConstraintBeforeStep(constraint)) {
    phRemove(&self->constraints.beforeStep, constraint);
  } else {
    phRemove(&self->constraints.afterStep, constraint);
  }
  return self;
}

phworld * phWorldSafeRemoveConstraint(
  phworld *self, phconstraint *constraint, phfreefn freefn
) {
  return phAfterStep(
    self, (phitrfn) phWorldRemoveConstraint, freefn, constraint
  );
}

phworld * phWorldAddComposite(phworld *self, phcomposite *composite) {
  phlistiterator litr;
  phIterate(
    phIterator(&composite->particles, &litr),
    (phitrfn) phWorldAddParticle,
    self
  );
  phIterate(
    phIterator(&composite->constraints, &litr),
    (phitrfn) phWorldAddConstraint,
    self
  );
  return self;
}

phworld * phWorldRemoveComposite(phworld *self, phcomposite *composite) {
  phlistiterator litr;
  phIterate(
    phIterator(&composite->particles, &litr),
    (phitrfn) phWorldRemoveParticle,
    self
  );
  phIterate(
    phIterator(&composite->constraints, &litr),
    (phitrfn) phWorldRemoveConstraint,
    self
  );
  return self;
}

phworld * phWorldSafeRemoveComposite(
  phworld *self, phcomposite *composite, phfreefn freefn
) {
  return phAfterStep(
    self, (phitrfn) phWorldRemoveConstraint, freefn, composite
  );
}
