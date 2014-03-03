#include "ph.h"
#include "tap.h"

struct iterateTestData {
  int index;
  char *data[ 4 ];
};

void iterateTestItr(void *ctx, void *item) {
  struct iterateTestData *data = ctx;
  ok(
    item == data->data[ data->index ],
    "Item %d is %s, should be %s.",
    data->index,
    data->data[ data->index ],
    item
  );
  data->index++;
}

void test_list() {
  note("\n** list **");

  {
    note("phlist");
    phlist list = phlist();
    ok(list.length == 0, "List length is 0.");
  }

  {
    note("phAppend");
    phlist a = phlist();
    char *str = "arbitrary";
    phAppend(&a, str);
    ok(a.length == 1, "List length is 1.");
    ok(a.last->item == str, "Last node holds appended item.");
    char *anotherStr = "different";
    phAppend(&a, anotherStr);
    ok(a.length == 2, "List length is 2.");
    ok(a.last->item == anotherStr, "Last node holds second appended item.");
    phDump(&a, NULL);
    ok(a.length == 0, "List length is 0.");
  }

  {
    note("phClean");
    phlist list = phlist();
    char *str = "data";
    phAppend(&list, str);
    ok(list.length == 1, "List length is 1.");
    phClean(&list, NULL);
    ok(list.length == 0, "List length is 0.");
    ok(list.freeList != NULL, "Node placed in free list.");
    phAppend(&list, str);
    ok(list.length == 1, "List length is 1.");
    ok(list.freeList == NULL, "Node from freeList used.");
    phDump(&list, NULL);
  }

  {
    note("phInsert");
    phlist list = phlist();
    char *a = "data1";
    phInsert(&list, 0, a);
    ok(list.length == 1, "Length is 1.");
    ok(list.first->item == a, "First item just inserted.");
    ok(list.last->item == a, "Last item just inserted.");
    char *b = "data2";
    phInsert(&list, 0, b);
    ok(list.length == 2, "Length is 2.");
    ok(list.first->item == b, "First item just inserted.");
    ok(list.last->item == a, "Last item is first inserted.");
    char *c = "data3";
    phInsert(&list, 1, c);
    ok(list.length == 3, "Length is 3.");
    ok(list.last->item == a, "Last item is the first inserted.");
    ok(list.first->item == b, "First item is the second inserted.");
    ok(list.first->next->item == c, "Middle item is last inserted.");
    ok(
      list.last->prev->item == c,
      "Reverse lookup middle item, last inserted."
    );
    ok(
      list.last->prev->prev->item == b,
      "Reverse lookup first item, second inserted."
    );
    phDump(&list, NULL);
  }

  {
    note("phRemove");
    phlist list = phlist();
    char *a = "data1";
    char *b = "data2";

    phAppend(&list, a);
    ok(list.length == 1, "Length is 1.");
    phRemove(&list, a);
    ok(list.length == 0, "Length is 0.");
    ok(list.first == NULL, "First is NULL.");
    ok(list.last == NULL, "Last is NULL.");
    ok(list.freeList != NULL, "freeList has node.");

    phAppend(&list, a);
    phAppend(&list, b);
    ok(list.length == 2, "Length is 2.");
    ok(list.first->item == a, "First item is first appended.");
    ok(list.first->next->item == b, "Second item is second appended.");
    phRemove(&list, b);
    ok(list.length == 1, "Length is 1.");
    phRemove(&list, a);
    ok(list.length == 0, "Length is 0.");

    phDump(&list, NULL);
  }

  {
    note("phIterate");
    phlist list = phlist();
    char *a = "data1";
    char *b = "data2";
    char *c = "data3";

    phAppend(&list, a);
    phAppend(&list, b);
    phAppend(&list, c);
    struct iterateTestData testData = {
      0,
      { a, b, c, NULL }
    };
    phlistiterator _itr;
    phiterator *itr = phIterator(&list, &_itr);
    phIterate(itr, iterateTestItr, &testData);
    phDump(&list, NULL);
  }

  {
    note("concat iteration");
    phlist list1 = phlist();
    phlist list2 = phlist();
    char *a = "data1";
    char *b = "data2";
    char *c = "data3";

    phAppend(&list1, a);
    phAppend(&list2, b);
    phAppend(&list2, c);
    phlistiterator _itr;
    phIterate(phIterator(&list2, &_itr), (phitrfn) phAppend, &list1);

    struct iterateTestData testData = {
      0,
      { a, b, c, NULL }
    };
    phIterate(phIterator(&list1, &_itr), iterateTestItr, &testData);
    phDump(&list1, NULL);
    phDump(&list2, NULL);
  }

  {
    note("phlistv");
    char *a = "data1";
    char *b = "data2";
    char *c = "data3";
    phlist list = phlistv(a, b, c);

    struct iterateTestData testData = {
      0,
      { a, b, c, NULL }
    };
    phlistiterator _itr;
    phIterate(phIterator(&list, &_itr), iterateTestItr, &testData);
    phDump(&list, NULL);
  }

  {
    note("phIterate w/o removed values");
    char *a = "data1";
    char *b = "data2";
    char *c = "data3";
    phlist list = phlistv(a, b, c);

    phRemove(&list, b);

    struct iterateTestData testData = {
      0,
      { a, c, NULL }
    };
    phlistiterator _itr;
    phIterate(phIterator(&list, &_itr), iterateTestItr, &testData);
    phDump(&list, NULL);
  }
}
