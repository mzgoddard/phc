#include "ph.h"

typedef struct _pharrayiterator {
  phiterator iterator;
  pharray *array;
  phint index;
} _pharrayiterator;

#define pharrayiterator(array, index) ((pharrayiterator) { \
  (phiteratornext) phArrayNext, (phiteratorderef) phArrayDeref, array, index \
})

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

void * phGetIndex(phiterator *self, phint index) {
  index += 1;
  while (index-- && phNext(self)) {}
  return phDeref(self);
}

phbool phSame(void *ctx, void *item) {
  return ctx == item;
}

pharray * phToArray(phiterator *self, pharray *ary) {
  phint i = 0;
  phint capacity = ary->capacity;
  void **items = ary->items;
  for (; i < capacity && phNext(self); ++i, items++) {
    *items = phDeref(self);
  }
  return ary;
}

phbool phArrayNext(_pharrayiterator *self) {
  self->index++;
  return self->index < self->array->capacity;
}

void * phArrayDeref(_pharrayiterator *self) {
  return self->index > -1 && self->index < self->array->capacity ?
    self->array->items[self->index] :
    NULL;
}

phiterator * phArrayIterator(pharray *self, pharrayiterator *itr) {
  if (itr == NULL) {
    itr = phAlloc(pharrayiterator);
  }
  *itr = pharrayiterator(self, -1);
  return (phiterator *) itr;
}
