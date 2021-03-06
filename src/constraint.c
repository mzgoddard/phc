#include "ph.h"

phconstraint * phConstraintCopy(void *self, phlist *dst, phlist *src) {
  return ((phconstraint*) self)->type->copy(self, dst, src);
}

phbool phConstraintBeforeStep(void *self) {
  return ((phconstraint*) self)->type->beforeStep(self);
}

void phConstraintUpdate(phworld *world, void *self) {
  ((phconstraint*) self)->type->update(self, world);
}
