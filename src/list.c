#include "ph.h"

#define phlistnode(next, prev, item) ((phlistnode) { next, prev, item })

typedef struct _phlistiterator {
  phiterator iterator;
  phlist *list;
  phlistnode *node;
} _phlistiterator;

#define phlistiterator(list, node) ((phlistiterator) { \
  (phiteratornext) phListNext, (phiteratorderef) phListDeref, list, node \
})

phbool phListNext(_phlistiterator *self) {
  if (!self->node) {
    self->node = self->list->first;
    return self->node != NULL;
  } else if (self->node->next) {
    self->node = self->node->next;
    return 1;
  }
  return 0;
}

void * phListDeref(_phlistiterator *self) {
  return self->node != NULL ? self->node->item : NULL;
}

void phClean(phlist *self, void (*freeItem)(void *)) {
  phlistnode *node = self->first;

  while (node != NULL) {
    phlistnode *next = node->next;
    if (freeItem != NULL) {
      freeItem(node->item);
    }

    node->next = self->freeList;
    self->freeList = node;

    node = next;
  }

  self->length = 0;
  self->first = NULL;
  self->last = NULL;
}

phiterator * phIterator(phlist *self, phlistiterator *itr) {
  if (itr == NULL) {
    itr = phAlloc(phlistiterator);
  }
  *itr = phlistiterator(self, NULL);
  return (phiterator *) itr;
}

// void phIterate(phlist *self, void (*itr)(void *, void *), void *ctx) {
//   phlistnode *node = self->first;
// 
//   while (node != NULL) {
//     itr(ctx, node->item);
//     node = node->next;
//   }
// }

void phDump(phlist *self, void (*freeItem)(void *)) {
  phlistnode *node = self->first;

  while (node != NULL) {
    phlistnode *next = node->next;
    if (freeItem != NULL) {
      freeItem(node->item);
    }
    phFree(node);
    node = next;
  }

  node = self->freeList;
  while(node != NULL) {
    phlistnode *next = node->next;
    phFree(node);
    node = next;
  }

  self->length = 0;
  self->first = NULL;
  self->last = NULL;
  self->freeList = NULL;
}

phlist * phAppend(phlist *self, void *item) {
  phlistnode *node;
  if (self->freeList) {
    node = self->freeList;
    self->freeList = self->freeList->next;
  } else {
    node = phAlloc(phlistnode);
  }

  *node = phlistnode(NULL, self->last, item);

  if (self->first == NULL) {
    self->first = node;
  }
  if (self->last != NULL) {
    self->last->next = node;
  }
  self->last = node;

  self->length++;
  return self;
}

phlist * phInsert(phlist *self, int index, void *item) {
  if (index >= self->length) {
    return phAppend(self, item);
  }

  phlistnode *node = self->first;
  int i = 0;

  while (node != NULL && i < index) {
    node = node->next;
    i++;
  }

  if (node != NULL) {
    phlistnode *newNode;
    if (self->freeList) {
      newNode = self->freeList;
      self->freeList = self->freeList->next;
    } else {
      newNode = phAlloc(phlistnode);
    }

    *newNode = phlistnode(node, node->prev, item);

    if (node->prev != NULL) {
      node->prev->next = newNode;
    }
    node->prev = newNode;

    if (self->first == node) {
      self->first = newNode;
    }

    self->length++;
  }

  return self;
}

phlist * phRemove(phlist *self, void *item) {
  phlistnode *node = self->first;

  while (node != NULL && node->item != item) {
    node = node->next;
  }

  if (node != NULL) {
    if (node->prev != NULL) {
      node->prev->next = node->next;
    }
    if (node->next != NULL) {
      node->next->prev = node->prev;
    }

    if (self->first == node) {
      self->first = node->next;
    }
    if (self->last == node) {
      self->last = node->prev;
    }

    node->next = self->freeList;
    self->freeList = node;

    self->length--;
  }

  return self;
}
