#include "ph.h"

void phWorldDump(phworld *self) {
  phDump(&self->particles, NULL);
  phDump(&self->particleData, NULL);
  phDump(&self->constraints.beforeStep, NULL);
  phDump(&self->constraints.afterStep, NULL);
  phDdvtDump(&self->_optimization.ddvt);
}

phcollision *_phWorldNextCollision(phworld *self) {
  phlistiterator _litr;
  phiterator *itr = phIterator(&self->_optimization.nextCollisions, &_litr);
  phcollision *col;
  if (phNext(itr)) {
    col = phDeref(itr);
  } else {
    col = phAlloc(phcollision);
    phAppend(&self->_optimization.nextCollisions, col);
  }
  return col;
}

void _phWorldSaveCollision(phworld *self, phcollision *col) {
  phRemove(&self->_optimization.nextCollisions, col);
  phAppend(&self->_optimization.collisions, col);
}

void phWorldStep(phworld *self, phdouble dt) {
  // before constraints
  phlistiterator _litr;
  phiterator *itr = phIterator(&self->constraints.beforeStep, &_litr);
  phIterate(itr, (phitrfn) phConstraintUpdate, self);

  // integrate, sleep, and update ddvt
  phbox oldBox, newBox;
  itr = phIterator(&self->particles, &_litr);
  while (phNext(itr)) {
    phparticle *particle = phDeref(itr);
    oldBox = particle->_worldData->oldBox;
    phIntegrate(particle, self->timing.dt);
    newBox = particle->_worldData->oldBox =
      phAabb(particle->position, particle->radius);
    // Update particle in ddvt
    phDdvtUpdate(&self->_optimization.ddvt, particle, oldBox, newBox);
    // If velocity is below threshold, increment counter to sleep
  }

  // test and unsleep
  phddvtiterator _ditr;
  itr = phDdvtIterator(&self->_optimization.ddvt, &_ditr);
  while (phNext(itr)) {
    phcollision *nextCollision = _phWorldNextCollision(self);
    phddvtpair *pair = phDeref(itr);
    if (phTest(pair->a, pair->b, nextCollision)) {
      nextCollision->a = pair->a;
      nextCollision->b = pair->b;
      _phWorldSaveCollision(self, nextCollision);
    }
  }

  // solve
  itr = phIterator(&self->_optimization.collisions, &_litr);
  while (phNext(itr)) {
    phcollision *col = phDeref(itr);
    phSolve(col->a, col->b, col);
  }

  // post constraint
  itr = phIterator(&self->constraints.afterStep, &_litr);
  phIterate(itr, (phitrfn) phConstraintUpdate, self);
}

// phparticle * phWorldRayCast(phworld *, phray);
// phiterator * phWorldRayIterator(phworld *, phray, phrayiterator *);
// phiterator * phWorldBoxIterator(phworld *, phbox, phboxiterator *);

phworld * phWorldAddParticle(phworld *self, phparticle *particle) {
  particle->_worldData = phAlloc(phparticleworlddata);
  particle->_worldData->oldBox = phAabb(particle->position, particle->radius);
  phAppend(&self->particles, particle);
  phDdvtAdd(&self->_optimization.ddvt, particle);
  return self;
}

phworld * phWorldRemoveParticle(phworld *self, phparticle *particle) {
  phbox oldBox = particle->_worldData->oldBox;
  phDdvtRemove(&self->_optimization.ddvt, particle, oldBox);
  phRemove(&self->particles, particle);
  phFree(&particle->_worldData);
  particle->_worldData = NULL;
  return self;
}

// phworld * phWorldAddConstraint(phworld *, phconstraint *);
// phworld * phWorldRemoveConstraint(phworld *, phconstraint *);
