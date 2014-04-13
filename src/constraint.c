#include "ph.h"

// phconstraint * phConstraintCopy(void *self, phlist *dst, phlist *src);

phbool phConstraintBeforeStep(void *self) {
  return ((phconstraint*) self)->type->beforeStep(self);
}

void phConstraintUpdate(void *self, phworld *world) {
  ((phconstraint*) self)->type->update(self, world);
}
