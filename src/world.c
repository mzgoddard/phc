#include "ph.h"

void phWorldDump(phworld *self) {
  phDump(&self->particles, NULL);
  phDump(&self->particleData, free);
  phDump(&self->constraints.beforeStep, NULL);
  phDump(&self->constraints.afterStep, NULL);
  phDdvtDump(&self->_optimization.ddvt);
  phDump(&self->_optimization.collisions, free);
  phDump(&self->_optimization.nextCollisions, free);
  phDump(&self->_garbage.garbage, free);
  phDump(&self->_garbage.next, free);
}

static phcollision *_phWorldNextCollision(phworld *self) {
  // phlistiterator _litr;
  // phiterator *itr = phIterator(&self->_optimization.nextCollisions, &_litr);
  phcollision *col;
  // Get the first collision or create a new one.
  if (self->_optimization.nextCollisions.first) {
    col = self->_optimization.nextCollisions.first->item;
  } else {
    col = phAlloc(phcollision);
    phAppend(&self->_optimization.nextCollisions, col);
  }
  return col;
}

static void _phWorldSaveCollision(phworld *self) {
  phcollision *col = phShift(&self->_optimization.nextCollisions);
  phAppend(&self->_optimization.collisions, col);
}

void phWorldInternalStep(phworld *self) {
  self->timing.insideStep = 1;

  // before constraints
  phlistiterator _litr;
  phiterator *itr = phIterator(&self->constraints.beforeStep, &_litr);
  phIterate(itr, (phitrfn) phConstraintUpdate, self);

  // integrate, sleep, and update ddvt
  phbox oldBox, newBox;
  itr = phIterator(&self->particles, &_litr);
  while (phListNext((phlistiterator *) itr)) {
    phparticle *particle = phListDeref((phlistiterator *) itr);
    oldBox = particle->_worldData.oldBox;
    phIntegrate(particle, self->timing.dt);
    phTestReset(particle);
    // Update particle in ddvt
    if (
      phMag2(phSub(particle->position, phCenter(oldBox))) >
        particle->radius / 10
    ) {
      newBox = phAabb(particle->position, particle->radius);
      phDdvtUpdate(&self->_optimization.ddvt, particle, oldBox, newBox);
      particle->_worldData.oldBox = newBox;
    }
    // If velocity is below threshold, increment counter to sleep
  }

  // test and unsleep
  phddvtpairiterator _ditr;
  itr = phDdvtPairIterator(&self->_optimization.ddvt, &_ditr);
  while (phDdvtPairNext((phddvtpairiterator *) itr)) {
    pharray array = _ditr.particles;
    // phint length = _ditr.ddvt->particles.length;
    phint length = _ditr.ddvt->length;
    for (phint i = 0; i < length; ++i) {
      phparticle *a = array.items[i];
      for (phint j = i + 1; j < length; ++j) {
        phcollision *nextCollision = _phWorldNextCollision(self);
        phparticle *b = array.items[j];

        // Pre test with boxes, which is cheaper than circle test. Then
        // circle test.
        if (
          phIntersect(a->_worldData.oldBox, b->_worldData.oldBox) &&
            phTest(a, b, nextCollision)
        ) {
          nextCollision->a = a;
          nextCollision->b = b;
          _phWorldSaveCollision(self);
        }
      }
    }
    _ditr.arrayItr1.test = 0;
  }

  // solve
  itr = phIterator(&self->_optimization.collisions, &_litr);
  while (phListNext((phlistiterator *) itr)) {
    phcollision *col = phListDeref((phlistiterator *) itr);
    phSolve(col->a, col->b, col);
  }

  // Empty collisions into nextCollisions.
  phlist *collisions = &self->_optimization.collisions;
  phlist *nextCollisions = &self->_optimization.nextCollisions;
  if (collisions->length > nextCollisions->length) {
    phlist tmp = *collisions;
    *collisions = *nextCollisions;
    *nextCollisions = tmp;
  }
  while (collisions->length) {
    phcollision *col = phShift(collisions);
    phAppend(nextCollisions, col);
  }

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
    phparticleworlddata(phAabb(particle->position, particle->radius));
  phAppend(&self->particles, particle);
  phDdvtAdd(&self->_optimization.ddvt, particle);
  return self;
}

phworld * phWorldRemoveParticle(phworld *self, phparticle *particle) {
  phbox oldBox = particle->_worldData.oldBox;
  phDdvtRemove(&self->_optimization.ddvt, particle, oldBox);
  phRemove(&self->particles, particle);
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

phworld * _phWorldSafeRemove(
  phworld *self, void *item, phitrfn fn, phfreefn freefn
) {
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
  return _phWorldSafeRemove(
    self, particle, (phitrfn) phWorldRemoveParticle, freefn
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
  return _phWorldSafeRemove(
    self, constraint, (phitrfn) phWorldRemoveConstraint, freefn
  );
}
