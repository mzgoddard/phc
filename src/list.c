#include "ph.h"

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

phlist * phPrepend(phlist *self, void *item) {
  phlistnode *node = self->freeList;
  if (node) {
    self->freeList = self->freeList->next;
  } else {
    node = phAlloc(phlistnode);
  }

  *node = phlistnode(self->first, NULL, item);

  if (!self->last) {
    self->last = node;
  } else {
    self->first->prev = node;
  }
  self->first = node;

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

  if (!node) {
    return self;
  }
  while (node->item != item && (node = node->next)) {}

  if (node) {
    if (node->prev) {
      node->prev->next = node->next;
    } else {
      self->first = node->next;
    }

    if (node->next) {
      node->next->prev = node->prev;
    } else {
      self->last = node->prev;
    }

    node->next = self->freeList;
    self->freeList = node;

    self->length--;
  }

  return self;
}

phlist * phRemoveLast(phlist *self, void *item) {
  phlistnode *node = self->last;

  if (!node) {
    return self;
  }
  while (node->item != item && (node = node->prev)) {}

  if (node) {
    if (node->prev) {
      node->prev->next = node->next;
    } else {
      self->first = node->next;
    }

    if (node->next) {
      node->next->prev = node->prev;
    } else {
      self->last = node->prev;
    }

    node->next = self->freeList;
    self->freeList = node;

    self->length--;
  }

  return self;
}
