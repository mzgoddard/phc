#include "ph.h"

void phIterateN(phiterator *self, phint n, phitrfn itr, void *ctx) {
  while (n-- && phNext(self)) {
    itr(ctx, phDeref(self));
  }
}

void phIterateStop(phiterator *self, phitrstopfn itr, void *ctx) {
  phbool stop = 0;
  while (!stop && phNext(self)) {
    itr(ctx, phDeref(self), &stop);
  }
}

phiterator * phFind(phiterator *self, phcmpfn cmp, void *ctx) {
  while (phNext(self)) {
    if (cmp(ctx, phDeref(self))) {
      break;
    }
  }
  return self;
}

phint phIndexOf(phiterator *self, phcmpfn cmp, void *ctx) {
  phint index = -1;
  while (phNext(self)) {
    index++;
    if (cmp(ctx, phDeref(self))) {
      return index;
    }
  }
  return -1;
}

phbool phContains(phiterator *self, void *ctx) {
  return phIndexOf(self, phSame, ctx) != -1;
}

void * phGetIndex(phiterator *self, phint index) {
  index += 1;
  while (index-- && phNext(self)) {}
  return phDeref(self);
}

phbool phSame(void *ctx, void *item) {
  return ctx == item;
}
