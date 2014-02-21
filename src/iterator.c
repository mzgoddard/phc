#include "ph.h"

phbool phNext(phiterator *self) {
  return self->next(self);
}

void * phDeref(phiterator *self) {
  return self->deref(self);
}

void phIterate(phiterator *self, phitrfn itr, void *ctx) {
  while (phNext(self)) {
    itr(ctx, phDeref(self));
  }
}

void phIterateN(phiterator *self, phint n, phitrfn itr, void *ctx) {
  while (phNext(self) && n--) {
    itr(ctx, phDeref(self));
  }
}

void phIterateStop(phiterator *self, phitrstopfn itr, void *ctx) {
  phbool stop = 1;
  while (phNext(self) && stop) {
    itr(ctx, phDeref(self), &stop);
  }
}
