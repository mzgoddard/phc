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
}
