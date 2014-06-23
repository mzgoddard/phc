#include "ph.h"

// phbyte * phCompositeFreeze(phcomposite *);
// phcomposite * phCompositeThaw(phcomposite *, phbyte *);
// phcomposite * phCompositeCopy(phcomposite *dst, phcomposite *src);

phcomposite * phCompositeDump(phcomposite *self) {
  phlistiterator litr;
  phIterate(
    phIterator(&self->particles, &litr),
    (phitrfn) phCall,
    (phitrfn) phParticleDump
  );
  phIterate(
    phIterator(&self->constraints, &litr),
    (phitrfn) phCall,
    (phitrfn) phConstraintDump
  );
  phDump(&self->particles, free);
  phDump(&self->constraints, free);
  return self;
}

phcompositeline * phCompositeLineDump(phcompositeline *self) {
  phCompositeDump((phcomposite *) self);
  return self;
}

phcompositeline * phCompositeLineAdd(
  phcompositeline *self, phv pt, phitrfn adder
) {
  if (!self->composite.particles.length) {
    adder(self, &pt);
  } else {
    phparticle *last = phLast(&self->composite.particles);
    phv lastPoint = last->position;
    while (phMag(phSub(pt, lastPoint)) > self->distanceBetweenPoints) {
      lastPoint = phAdd(
        lastPoint,
        phScale(phUnit(phSub(pt, lastPoint)), self->distanceBetweenPoints)
      );
      adder(self, &lastPoint);
    }
  }
  return self;
}
