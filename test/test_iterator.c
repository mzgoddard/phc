#include "ph.h"
#include "tap.h"

struct testObject {
  phint length;
  void *values[4];
};

typedef struct phtestiterator {
  phiterator iterator;
  struct testObject *data;
  int i;
} phtestiterator;

phbool phTestNext(phtestiterator *self) {
  self->i++;
  return self->i < self->data->length;
}

void * phTestDefer(phtestiterator *self) {
  if (self->i < 0 || self->i >= self->data->length) {
    return NULL;
  }
  return self->data->values[self->i];
}

phiterator * phTestIterator(struct testObject *self, phtestiterator *itr) {
  *itr = (phtestiterator) {
    (phiteratornext) phTestNext,
    (phiteratorderef) phTestDefer,
    self,
    -1
  };
  return (phiterator *) itr;
}

void test_iterator() {
  note("\n** iterator **");

  {
    note("phNext, phDeref");
    phint a = 1, b = 2;
    struct testObject object = {
      2,
      {(void *) &a, (void *) &b, NULL, NULL}
    };
    phtestiterator _testiterator;
    phiterator *itr = phTestIterator(&object, &_testiterator);
    ok(phNext(itr), "phNext calls internal next fn.");
    ok(_testiterator.i == 0, "iterator stepped from initial state.");
    ok(phDeref(itr) == &a, "phDeref returned pointer to first object.");
    ok(phNext(itr), "phNext stepped.");
    ok(!phNext(itr), "phNext reached end.");
    ok(!phNext(itr), "phNext did not step after reaching end.");
    ok(phDeref(itr) == NULL, "phDeref returned NULL after end.");
  }
}
