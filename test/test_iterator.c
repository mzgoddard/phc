#include "ph.h"
#include "tap.h"

struct testObject {
  phint length;
  void **values;
};

typedef struct phtestiterator {
  phiterator iterator;
  struct testObject *data;
  int i;
} phtestiterator;

#define pharraytestprelude(name) note(#name); \
  phint a = 1, b = 2, c = 3; \
  pharray array = pharrayv(&a, &b, &c); \
  pharrayiterator _itr; \
  phiterator *itr = phArrayIterator(&array, &_itr);

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

void phCmpItr(phiterator *ctx, void *item) {
  ok(phNext(ctx), "Iterator context stepped.");
  ok(phDeref(ctx) == item, "Iterator value is same as item.");
}

void incrementCtx(void *ctx, void *item) {
  phint *n = ctx;
  (*n)++;
}

void stopAfterCtx(void *ctx, void *item, phbool *stop) {
  phint *x = ctx;
  if (--(*x) == 0) {
    *stop = 1;
  }
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
      (void *[]) {&a, &b, NULL, NULL}
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

  {
    note("phToArray");
    phint a = 1, b = 2, c = 3;
    phlist list = phlist();
    phAppend(&list, &a);
    phAppend(&list, &b);
    phAppend(&list, &c);

    void *rawarray[] = {NULL, NULL, NULL};
    pharray array = pharray(3, rawarray);
    phlistiterator _itr;
    phToArray(phIterator(&list, &_itr), &array);
    phDump(&list, NULL);

    ok(array.items[0] == &a, "First array value is address of `a`.");
    ok(array.items[1] == &b, "Second array value is address of `b`.");
    ok(array.items[2] == &c, "Third array value is address of `c`.");
  }

  {
    pharraytestprelude(phArrayIterator);
    struct testObject testobj = {
      3,
      (void *[]) {&a, &b, &c}
    };
    phtestiterator _titr;
    phIterate(
      itr,
      (phitrfn) phCmpItr,
      phTestIterator(&testobj, &_titr)
    );
  }

  {
    pharraytestprelude(phIterateN);
    phint n = 0;
    phIterateN(itr, 1, incrementCtx, &n);
    ok(n == 1, "Iterated 1 step forward.");
    ok(phDeref(itr) == &a, "Iterator is at the first value.");
  }

  {
    pharraytestprelude(phIterateStop);
    phint x = 1;
    phIterateStop(itr, stopAfterCtx, &x);
    ok(phDeref(itr) == &a, "Iterator is at the first value.");
  }

  {
    pharraytestprelude(phFind);
    phFind(itr, phSame, &b);
    ok(phDeref(itr) == &b, "phFind found `b`.");
    phFind(itr, phSame, &a);
    ok(
      phDeref(itr) == NULL,
      "phFind could not find `a` since it had already past it."
    );
  }

  {
    pharraytestprelude(phIndexOf);
    ok(phIndexOf(itr, phSame, &b) == 1, "Index of `b` is 1.");
    itr = phArrayIterator(&array, &_itr);
    ok(phIndexOf(itr, phSame, NULL) == -1, "Index of NULL is -1.");
  }

  {
    pharraytestprelude(phGetIndex);
    ok(phGetIndex(itr, 1) == &b, "`b` is at index 1.");
    itr = phArrayIterator(&array, &_itr);
    ok(phGetIndex(itr, -1) == NULL, "NULL is at index -1.");
  }
}
