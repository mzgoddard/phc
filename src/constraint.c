#include "ph.h"

phconstraint * phConstraintCopy(void *self, phlist *dst, phlist *src) {
  return ((phconstraint*) self)->type->copy(self, dst, src);
}

phconstraint * phConstraintDump(void *self) {
  return ((phconstraint*) self)->type->dump(self);
}

phbool phConstraintBeforeStep(void *self) {
  return ((phconstraint*) self)->type->beforeStep(self);
}

void phConstraintUpdate(phworld *world, void *self) {
  ((phconstraint*) self)->type->update(self, world);
}
