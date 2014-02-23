#include "ph.h"
#include "bench.h"

void sumReduce(phint *ctx, phint *item) {
  phint result = *item;
}

void stepN(phint n, void *ctx, pharray *ary, phitrfn fn) {
  for (int i = 0; i < n; ++i) {
    fn(ctx, ary->items[i]);
  }
}

void b_forloop(void *ctx) {
  phint sum = 0;
  pharray *array = ctx;
  stepN(array->capacity, &sum, ctx, (phitrfn) sumReduce);
}

void b_whileltcapacity(void *ctx) {
  pharray *array = ctx;
  phint index, capacity = array->capacity;
  phint sum = 0;
  for (index = 0; index < capacity; ++index) {
    sumReduce(&sum, array->items[index]);
  }
}

void b_whilenext(void *ctx) {
  phlist *list = ctx;
  phlistnode *node = list->first;
  phint sum = 0;
  for (; node; node = node->next) {
    sumReduce(&sum, node->item);
  }
}

void b_arrayiterator(void *ctx) {
  pharrayiterator _arrayitr;
  phint sum = 0;
  phIterate(phArrayIterator(ctx, &_arrayitr), (phitrfn) sumReduce, &sum);
}

void b_listiterator(void *ctx) {
  phlistiterator _listitr;
  phint sum = 0;
  phIterate(phIterator(ctx, &_listitr), (phitrfn) sumReduce, &sum);
}

void b_fastarrayiterator(void *ctx) {
  pharrayiterator _arrayitr;
  phint sum = 0;
  phArrayIterator(ctx, &_arrayitr);
  phStaticIterate(
    (phbool (*)(phiterator *)) phArrayNext,
    (void *(*)(phiterator *)) phArrayDeref, (phiterator *) &_arrayitr, (phitrfn) sumReduce, &sum);
}

void b_fastlistiterator(void *ctx) {
  phlistiterator _listitr;
  phint sum = 0;
  phIterator(ctx, &_listitr);
  phStaticIterate((phbool (*)(phiterator *)) phListNext, (void *(*)(phiterator *)) phListDeref, (phiterator *) &_listitr, (phitrfn) sumReduce, &sum);
}

#define ITERATIONDATA \
  phint nums[] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10}; \
  pharray array = pharrayv( \
    nums, nums+1, nums+2, nums+3, nums+4, \
    nums+5, nums+6, nums+7, nums+8, nums+9 \
  ); \
  phlist list = phlistv( \
    nums, nums+1, nums+2, nums+3, nums+4, \
    nums+5, nums+6, nums+7, nums+8, nums+9 \
  )

void bench_iterator() {
  {
    note("phIterate 100");
    phint nums[100];
    phint *numRefs[100];
    pharray array = pharray(100, ((void **) numRefs));
    phlist list = phlist();
    for (phint i = 0; i < 100; ++i) {
      nums[i] = i + 1;
      numRefs[i] = &nums[i];
      phAppend(&list, numRefs[i]);
    }

    bench(
      "forloop", b_forloop, &array,
      "array while lt", b_whileltcapacity, &array,
      "list while loop", b_whilenext, &list,
      "array iterator", b_arrayiterator, &array,
      "list iterator", b_listiterator, &list,
      "fast array iterator", b_fastarrayiterator, &array,
      "fast list iterator", b_fastlistiterator, &list,
      NULL
    );

    phDump(&list, NULL);
  }

  {
    note("phIterate 100,000");
    phint nums[100000];
    phint *numRefs[100000];
    pharray array = pharray(100000, ((void **) numRefs));
    phlist list = phlist();
    for (phint i = 0; i < 100000; ++i) {
      nums[i] = i + 1;
      numRefs[i] = &nums[i];
      phAppend(&list, numRefs[i]);
    }

    bench(
      "forloop", b_forloop, &array,
      "array while lt", b_whileltcapacity, &array,
      "list while loop", b_whilenext, &list,
      "array iterator", b_arrayiterator, &array,
      "list iterator", b_listiterator, &list,
      "fast array iterator", b_fastarrayiterator, &array,
      "fast list iterator", b_fastlistiterator, &list,
      NULL
    );

    phDump(&list, NULL);
  }
}
