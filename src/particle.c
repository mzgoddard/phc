#include "ph.h"

static const phdouble posmul = 1.999999999;
static const phdouble lastposmul = 0.999999999;
static const phdouble friction = 0.001;

void phParticleDump(phparticle *self) {
  phDump(&self->ignore, NULL);
}

void phParticleWorldDataDump(phparticleworlddata *self) {
  free(self->collideWith.items);
}

void phParticleCopy(phparticle *self, phparticle *source) {
  self->position = source->position;
  self->lastPosition = source->position;

  self->radius = source->radius;
  self->mass = source->mass;
  self->friction = source->friction;
  self->factor = source->factor;

  self->isStatic = source->isStatic;
  self->isTrigger = source->isTrigger;
  self->isSleeping = source->isSleeping;

  self->collideMask = source->collideMask;
  self->collideAgainst = source->collideAgainst;

  self->data = source->data;
  self->collide = source->collide;
}

void phIntegrate(phparticle *self, phdouble dt) {
  if (self->isStatic) {return;}
  if (self->isSleeping) {return;}

  phv position = self->position;
  self->position = phAdd(
    phSub(
      phScale(self->position, posmul),
      phScale(self->lastPosition, lastposmul)
    ),
    phScale(self->acceleration, dt * dt)
  );
  self->lastPosition = position;
  self->acceleration = phZero();
}

void phTestReset(phparticle *self) {
  // phClean(&self->_worldData.collideWith, NULL);
  self->_worldData.collideWithIndex = 0;
}

static phbool _phHasCollidedAlready(phparticle *self, phparticle *other) {
  // return phStaticContains(
  //   (phiteratornext) phListNext, (phiteratorderef) phListDeref,
  //   phIterator(&self->_worldData.collideWith, &_listitr),
  //   other
  // );
  phint length = self->_worldData.collideWithIndex;
  void **items = self->_worldData.collideWith.items;
  for (phint i = length - 1; i + 1; --i) {
    if (items[i] == other) {
      return 1;
    }
  }
  return 0;
}

phbool phIgnoresOther(phparticle *self, void *other) {
  phlistiterator _listitr;
  return phStaticContains(
    (phiteratornext) phListNext, (phiteratorderef) phListDeref,
    phIterator(&self->ignore, &_listitr),
    other
  );
}

static void _phAckCollision(phparticle *self, phparticle *other) {
  // if (self->_worldData) {
    // phAppend(&self->_worldData.collideWith, other);
    pharray *array = &self->_worldData.collideWith;
    if (array->capacity == self->_worldData.collideWithIndex) {
      void **oldItems = array->items;
      array->items = calloc(array->capacity + 32, sizeof(phparticle *));
      for (phint i = 0; i < array->capacity; ++i) {
        array->items[i] = oldItems[i];
      }
      array->capacity += 32;
      free(oldItems);
    }

    array->items[self->_worldData.collideWithIndex++] = other;
  // }
}

phbool phTest(phparticle *self, phparticle *other, phcollision *col) {
  if (
    (self->collideAgainst & other->collideMask) == 0 || 
      (other->collideAgainst & self->collideMask) == 0
  ) {
    return 0;
  }

  phdouble
    ax = self->position.x,
    ay = self->position.y,
    bx = other->position.x,
    by = other->position.y,
    abx = ax - bx,
    aby = ay - by,
    ar = self->radius,
    br = other->radius,
    abr = ar + br,
    ingress;

  ingress = abx*abx+aby*aby;
  if (((ingress < abr*abr))) {
    col->isTrigger = self->isTrigger || other->isTrigger;

    if (col->isTrigger) {
      col->isNormal = col->isStatic = 0;
      return 1;
    }

    col->isStatic = self->isStatic;
    col->isNormal = !col->isStatic;

    // Increase the ingress by a small value so its not zero.
    ingress = sqrt(ingress) + 1e-5;

    phdouble pqt = abr / ingress - 1;
    // col->distance = abr - ingress;
    col->lambx = abx * pqt;
    col->lamby = aby * pqt;

    return 1;
  }

  return 0;
}

static phbool _phPreSolve(phparticle *self, phparticle *other) {
  if (_phHasCollidedAlready(self, other)) {
    return 1;
  }

  _phAckCollision(self, other);
  _phAckCollision(other, self);

  // Test ignores after so that the faster hasCollided can handle
  // repeated collisions.
  if (phIgnoresOther(self, other) || phIgnoresOther(self, other->data)) {
    return 1;
  }

  return 0;
}

void phSolve(phcollision *col) {
  phparticle *self = col->a, *other = col->b;

  if (_phPreSolve(self, other)) {
    return;
  }

  phdouble
    factor = (self->factor * other->factor),
    lambx = (col->lambx) * factor,
    lamby = (col->lamby) * factor,
    amsq = self->mass,
    bmsq = other->mass,
    mass = amsq + bmsq,
    am = bmsq / mass,
    bm = amsq / mass,
    avx = self->lastPosition.x - self->position.x,
    avy = self->lastPosition.y - self->position.y,
    bvx = other->lastPosition.x - other->position.x,
    bvy = other->lastPosition.y - other->position.y,
    fric = 1 - (self->friction * other->friction);

  avx *= fric;
  avy *= fric;
  bvx *= fric;
  bvy *= fric;

  self->lastPosition.x = self->position.x + avx;
  self->lastPosition.y = self->position.y + avy;
  self->position.x += lambx * am;
  self->position.y += lamby * am;

  other->lastPosition.x = other->position.x + bvx;
  other->lastPosition.y = other->position.y + bvy;
  other->position.x -= lambx * bm;
  other->position.y -= lamby * bm;
}

void phSolveStatic(phcollision *col) {
  phparticle *self = col->a, *other = col->b;

  if (_phPreSolve(self, other)) {
    return;
  }

  phdouble
    factor = (self->factor * other->factor),
    lambx = (col->lambx) * factor,
    lamby = (col->lamby) * factor,
    bvx = other->lastPosition.x - other->position.x,
    bvy = other->lastPosition.y - other->position.y,
    fric = 1 - (self->friction * other->friction);

  bvx *= fric;
  bvy *= fric;

  other->lastPosition.x = other->position.x + bvx;
  other->lastPosition.y = other->position.y + bvy;
  other->position.x -= lambx;
  other->position.y -= lamby;
}

void phSolveTrigger(phcollision *col) {
  phparticle *self = col->a, *other = col->b;

  if (_phPreSolve(self, other)) {
    return;
  }

  if (self->collide) {
    self->collide(self, other, col);
  }
  if (other->collide) {
    other->collide(other, self, col);
  }
}

// Ignore another particle or it's data.
void phIgnore(phparticle *self, void *value) {
  phAppend(&self->ignore, value);
}

void phStopIgnore(phparticle *self, void *value) {
  phRemove(&self->ignore, value);
}

phv phVelocity(phparticle *self) {
  return phSub(self->position, self->lastPosition);
}

phv phScaledVelocity(phparticle *self, phdouble dt) {
  return phScale(phSub(self->position, self->lastPosition), 1.0 / dt);
}

void phSetVelocity(phparticle *self, phv velocity) {
  self->lastPosition = phSub(self->position, velocity);
}

void phSetScaledVelocity(phparticle *self, phv velocity, phdouble dt) {
  self->lastPosition = phSub(self->position, phScale(velocity, dt));
}
