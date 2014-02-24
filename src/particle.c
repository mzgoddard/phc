#include "ph.h"

static const phdouble posmul = 1.999999999;
static const phdouble lastposmul = 0.999999999;
static const phdouble friction = 0.001;

void phParticleDump(phparticle *self) {
  phDump(&self->ignore, NULL);
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

phbool phTest(phparticle *self, phparticle *other, phcollision *col) {
  if (self->isStatic && other->isStatic) {return 0;}
 
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
    phlistiterator _listitr;
    if (
      self->_worldData &&
        phContains(phIterator(&self->_worldData->collideWith, &_listitr), other)
    ) {
      return 0;
    }

    if (phContains(phIterator(&self->ignore, &_listitr), other)) {
      return 0;
    }
    if (phContains(phIterator(&self->ignore, &_listitr), other->data)) {
      return 0;
    }

    if (self->_worldData) {
      phAppend(&self->_worldData->collideWith, other);
    }
    if (other->_worldData) {
      phAppend(&other->_worldData->collideWith, self);
    }

    ingress = sqrt(ingress);

    if ( ingress == 0.0 ) {
      ingress = 1e-5;
    }

    phdouble
      lx = abx,
      ly = aby,
      al = ingress,
      pt = (al - ar) / al,
      qt = br / al;
    col->distance = abr - ingress;
    col->lambx = lx * (qt - pt);
    col->lamby = ly * (qt - pt);

    return 1;
  }

  return 0;
}

void phSolve(phparticle *self, phparticle *other, phcollision *col) {
  phv
    *selfpos = &(self->position),
    *otherpos = &(other->position),
    *selflast = &(self->lastPosition),
    *otherlast = &(other->lastPosition);

  phdouble
    factor = (self->factor * other->factor),
    lambx = (col->lambx) * factor,
    lamby = (col->lamby) * factor,
    amsq = self->mass,
    bmsq = other->mass,
    mass = amsq + bmsq,
    am = bmsq / mass,
    bm = amsq / mass,
    avx = selflast->x - selfpos->x,
    avy = selflast->y - selfpos->y,
    avm = phHypot(avx, avy),
    bvx = otherlast->x - otherpos->x,
    bvy = otherlast->y - otherpos->y,
    bvm = phHypot(bvx, bvy),
    fric = fabs(col->distance) * self->friction * other->friction;

  if (avm != 0) {
    avx = (avx / avm) * (avm - fric);
    avy = (avy / avm) * (avm - fric);
  }
  if (bvm != 0) {
    bvx = bvx / bvm * (bvm - fric);
    bvy = bvy / bvm * (bvm - fric);
  }

  if (self->isStatic) {
    am = 0;
    bm = 1;
  } else if (other->isStatic) {
    am = 1;
    bm = 0;
  }

  if (!self->isTrigger && !other->isTrigger) {
    selflast->x = selfpos->x + avx;
    selflast->y = selfpos->y + avy;
    selfpos->x += lambx * am;
    selfpos->y += lamby * am;

    otherlast->x = otherpos->x + bvx;
    otherlast->y = otherpos->y + bvy;
    otherpos->x -= lambx * bm;
    otherpos->y -= lamby * bm;
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