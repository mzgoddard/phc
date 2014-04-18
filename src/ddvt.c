#include "./ph.h"

void _phDdvtAdd(phddvt *self, phparticle *particle, phbox *box);
void _phDdvtRemove(phddvt *self, phparticle *particle, phbox *box);
void _phDdvtWake(phddvt *self, phparticle *particle, phbox *box);
void _phDdvtUpdate(phddvt *self, phparticle *particle, phbox *old, phbox *new);

void phDdvtDump(phddvt *self) {
  if (self->tl) {
    phDdvtDump(self->tl);
    phFree(self->tl); self->tl = NULL;
    phDdvtDump(self->tr);
    phFree(self->tr); self->tr = NULL;
    phDdvtDump(self->bl);
    phFree(self->bl); self->bl = NULL;
    phDdvtDump(self->br);
    phFree(self->br); self->br = NULL;
  }
  // phDump(&self->particles, NULL);
  // phFree(self->_particleArray.items);
}

void _phDdvtAddLeaf(phddvt *self, phparticle *particle) {
  // phPrepend(&self->particles, particle);
  self->_particleArray.items[self->length] = particle;
  ++self->length;
  self->_particleArray.capacity = self->length;
  // self->_dirtyParticles++;
}

void _phDdvtRemoveLeaf(phddvt *self, phparticle *particle) {
  // phRemove(&self->particles, particle);
  // for (phint i = self->length - 1; i >= 0; --i) {
  for (phint i = 0, length = self->length; length - i; ++i) {
    if (self->_particleArray.items[i] == particle) {
      self->_particleArray.items[i] = self->_particleArray.items[length - 1];
      break;
    }
  }
  --self->length;
  self->_particleArray.capacity = self->length;
  // self->_dirtyParticles = self->length;
}

void _phDdvtUpdateLeaf(
  phddvt *self, phparticle *particle, phbox *old, phbox *new
) {
  phbool intersects = phIntersect(self->box, *new);
  phbool lastIntersect = phIntersect(self->box, *old);

  if (intersects && !lastIntersect) {
    phDdvtAdd(self, particle);
    return;
  }

  if (!intersects && lastIntersect) {
    _phDdvtRemoveLeaf(self, particle);
  }
}

void _phDdvtAddChild(phddvt *self, phparticle *particle, phbox *aabb) {
  phbool added = 0;
  #define INTERSECT_AND_ADD(corner) \
    if (phIntersect(self->corner->box, *aabb)) { \
      _phDdvtAdd(self->corner, particle, aabb); \
      added = 1; \
    }
  INTERSECT_AND_ADD(tl);
  INTERSECT_AND_ADD(tr);
  INTERSECT_AND_ADD(bl);
  INTERSECT_AND_ADD(br);
  #undef INTERSECT_AND_ADD

  if (added) {
    self->length++;
  }
}

phbool _phDdvtRemoveChild(phddvt *self, phparticle *particle, phbox *aabb) {
  phbool removed = 0;

  #define INTERSECT_AND_REMOVE(corner) \
    if (phIntersect(self->corner->box, *aabb)) { \
      _phDdvtRemove(self->corner, particle, aabb); \
      removed = 1; \
    }
  INTERSECT_AND_REMOVE(tl);
  INTERSECT_AND_REMOVE(tr);
  INTERSECT_AND_REMOVE(bl);
  INTERSECT_AND_REMOVE(br);
  #undef INTERSECT_AND_REMOVE

  if (removed) {
    self->length--;
  }

  return removed;
}

void _phDdvtUpdateChild(
  phddvt *self, phparticle *particle, phbox *old, phbox *new
) {
  phbool updated = 0;
  phbool added = 0;
  phbool removed = 0;

  #define INTERSECT_AND_UPDATE(corner) \
    if (corner ## Intersect && corner ## LastIntersect) { \
      if (self->corner->tl) { \
        _phDdvtUpdate(self->corner, particle, old, new); \
      } \
      updated = 1; \
    } else if (corner ## Intersect) { \
      _phDdvtAdd(self->corner, particle, new); \
      added = 1; \
    } else if (corner ## LastIntersect) { \
      _phDdvtRemove(self->corner, particle, old); \
      removed = 1; \
    }

  phbox *tlBox = &self->tl->box;

  phint centerX = tlBox->right;
  phint centerY = tlBox->bottom;

  phint leftIntersect = centerX >= new->left;
  phint topIntersect = centerY <= new->top;
  phint rightIntersect = centerX <= new->right;
  phint bottomIntersect = centerY >= new->bottom;
  phint leftLastIntersect = centerX >= old->left;
  phint topLastIntersect = centerY <= old->top;
  phint rightLastIntersect = centerX <= old->right;
  phint bottomLastIntersect = centerY >= old->bottom;

  phint tlIntersect = topIntersect && leftIntersect;
  phint tlLastIntersect = topLastIntersect && leftLastIntersect;
  phint trIntersect = topIntersect && rightIntersect;
  phint trLastIntersect = topLastIntersect && rightLastIntersect;
  phint blIntersect = bottomIntersect && leftIntersect;
  phint blLastIntersect = bottomLastIntersect && leftLastIntersect;
  phint brIntersect = bottomIntersect && rightIntersect;
  phint brLastIntersect = bottomLastIntersect && rightLastIntersect;
  INTERSECT_AND_UPDATE(tl)
  INTERSECT_AND_UPDATE(tr)
  INTERSECT_AND_UPDATE(bl)
  INTERSECT_AND_UPDATE(br)

  #undef INTERSECT_AND_UPDATE

  if (!updated) {
    if (added && !removed) {
      self->length++;
    }
    if (removed && !added) {
      self->length--;
    }
  }
}

void _phDdvtTestAndAdd(phddvt *self, phparticle *particle) {
  phbox box = particle->_worldData.oldBox;
  #define TEST_AND_ADD(corner) \
    if (phIntersect(self->corner->box, box)) { \
      _phDdvtAdd(self->corner, particle, &box); \
    }
  TEST_AND_ADD(tl);
  TEST_AND_ADD(tr);
  TEST_AND_ADD(bl);
  TEST_AND_ADD(br);
  #undef TEST_AND_ADD
}

void _phDdvtToChildren(phddvt *self) {
  #define ALLOC_AND_INIT(corner, CORNER) \
    self->corner = phAlloc(phddvt); \
    *self->corner = phddvt( \
      self, \
      ph ## CORNER(self->box), \
      PH_DDVT_PARTICLES_JOIN, \
      PH_DDVT_PARTICLES_SUBDIVIDE \
    )
  ALLOC_AND_INIT(tl, TL);
  ALLOC_AND_INIT(tr, TR);
  ALLOC_AND_INIT(bl, BL);
  ALLOC_AND_INIT(br, BR);
  #undef ALLOC_AND_INIT

  // phlistiterator _listitr;
  // phiterator *itr = phIterator(&self->particles, &_listitr);
  pharrayiterator _arrayitr;
  phiterator *itr = phArrayIterator(&self->_particleArray, &_arrayitr);
  phIterate(itr, (phitrfn) _phDdvtTestAndAdd, self);
  // phClean(&self->particles, NULL);
  self->_particleArray.capacity = 0;
}

void _phDdvtAddIfUnique(phddvt *self, phparticle *particle) {
  // phlistiterator _listitr;
  pharrayiterator _arrayitr;
  if (!phContains(phArrayIterator(&self->_particleArray, &_arrayitr), particle)) {
    if (
      phBoxContain(self->box, particle->_worldData.oldBox) ||
        !self->parent
    ) {
      particle->_worldData.topDdvt = self;
    }
    _phDdvtAddLeaf(self, particle);
  }
}

void _phDdvtFromChildren(phddvt *self) {
  self->length = 0;
  // phlistiterator _listitr;
  // phiterator *itr;
  pharrayiterator _arrayitr;
  phiterator *itr;
  #define ITR_ADD_UNIQUE(corner) \
    itr = phArrayIterator(&self->corner->_particleArray, &_arrayitr); \
    phIterate(itr, (phitrfn) _phDdvtAddIfUnique, self); \
    phDdvtDump(self->corner); phFree(self->corner); self->corner = NULL
  ITR_ADD_UNIQUE(tl);
  ITR_ADD_UNIQUE(tr);
  ITR_ADD_UNIQUE(bl);
  ITR_ADD_UNIQUE(br);
  #undef ITR_ADD_UNIQUE
}

void _phDdvtAdd(phddvt *self, phparticle *particle, phbox *box) {
  self->isSleeping = 0;

  if (phBoxContain(self->box, *box) || !self->parent) {
    particle->_worldData.topDdvt = self;
  }

  if (!self->tl && self->length < PH_DDVT_PARTICLES_SUBDIVIDE) {
    if (!self->_particleArray.items) {
      self->_particleArray.items = (void**) &self->_particleItems;
    }
    _phDdvtAddLeaf(self, particle);
  } else if (
    !self->tl && self->length >= PH_DDVT_PARTICLES_SUBDIVIDE &&
    self->parent && self->parent->length < PH_MAX_DDVT_PARTICLES - 2
  ) {
    _phDdvtAddLeaf(self, particle);
  } else {
    if (!self->tl) {
      _phDdvtToChildren(self);
    }

    _phDdvtAddChild(self, particle, box);
  }
}

void _phDdvtRemove(phddvt *self, phparticle *particle, phbox *box) {
  if (!self->tl) {
    _phDdvtRemoveLeaf(self, particle);
  } else {
    _phDdvtRemoveChild(self, particle, box);

    if (self->length <= PH_DDVT_PARTICLES_JOIN) {
      _phDdvtFromChildren(self);
    }
  }
}

void _phDdvtWake(phddvt *self, phparticle *particle, phbox *box) {
  if (!self->tl) {
    self->isSleeping = 0;
  } else if (self->isSleeping) {
    int contained = 0;
    #define CHECK_AND_WAKE(corner) \
      if (phIntersect(self->corner->box, *box)) { \
        _phDdvtWake(self->corner, particle, box); \
        contained = 1; \
      }
    CHECK_AND_WAKE(tl);
    CHECK_AND_WAKE(tr);
    CHECK_AND_WAKE(bl);
    CHECK_AND_WAKE(br);
    #undef CHECK_AND_WAKE

    if (contained) {
      self->isSleeping = 0;
    }
  }
}

void _phDdvtUpdate(phddvt *self, phparticle *particle, phbox *old, phbox *new) {
  _phDdvtUpdateChild(self, particle, old, new);

  if (self->length <= PH_DDVT_PARTICLES_JOIN) {
    _phDdvtFromChildren(self);
  }
}

void phDdvtAdd(phddvt *self, phparticle *particle) {
  phbox box = particle->_worldData.oldBox;
  _phDdvtAdd(self, particle, &box);
}

void phDdvtRemove(phddvt *self, phparticle *particle, phbox box) {
  _phDdvtRemove(self, particle, &box);
}

void phDdvtWake(phddvt *self, phparticle *particle) {
  phbox box = particle->_worldData.oldBox;
  _phDdvtWake(self, particle, &box);
}

void phDdvtUpdate(phddvt *self, phparticle *particle, phbox old, phbox new) {
  phddvt *top = particle->_worldData.topDdvt;
  phddvt *update = top;
  if (phBoxContain(top->box, new)) {
    while (top->tl) {
      phdouble centerY = top->tl->box.bottom;
      phint topContain = centerY <= new.bottom;
      if (topContain) {
        phdouble centerX = top->tl->box.right;
        phint leftContain = centerX >= new.right;
        if (leftContain) {
          top = top->tl;
          continue;
        }
        phint rightContain = centerX <= new.left;
        if (rightContain) {
          top = top->tr;
          continue;
        }
        // Contained in the top, between left and right. Not possible to be in
        // bottom so go ahead and break.
        break;
      }
      phint bottomContain = centerY >= new.top;
      if (bottomContain) {
        phdouble centerX = top->tl->box.right;
        phint leftContain = centerX >= new.right;
        if (leftContain) {
          top = top->bl;
          continue;
        }
        phint rightContain = centerX <= new.left;
        if (rightContain) {
          top = top->br;
          continue;
        }
      }
      break;
    }
  } else {
    while (top->parent) {
      top = top->parent;
      if (phBoxContain(top->box, new)) {
        break;
      }
    }
    update = top;
  }
  particle->_worldData.topDdvt = top;

  if (update->tl) {
    _phDdvtUpdate(update, particle, &old, &new);
  }
}

static void _phDdvtNextChild(phddvtiterator *self) {
  phddvt *ddvt = self->ddvt;
  phddvt *parent = ddvt ? ddvt->parent : NULL;
  if (parent) {
    for (
      phddvt **child = parent->children,
        **end = parent->children + 3;
      end - child;
      ++child
    ) {
      if (ddvt == *child) {
        ddvt = child[1];
        while (ddvt->tl) {
          ddvt = ddvt->tl;
        }
        self->ddvt = ddvt;
        return;
      }
    }
  }
  // No sibling ddvt, bump up a level.
  self->ddvt = parent;
}

static void _phDdvtCheckParticleArray(phddvt *self) {
  if (self->_dirtyParticles) {
    if (!self->_particleArray.items) {
      self->_particleArray = pharray(self->length, calloc(PH_MAX_DDVT_PARTICLES, sizeof(phparticle *)));
      self->_dirtyParticles = self->length;
    }
    phint newParticles = self->_dirtyParticles;
    // phint newParticles = self->length;
    // void **items = self->_particleArray.items + self->length - newParticles;
    void **items = self->_particleArray.items;
    for (
      phint i = self->_particleArray.capacity - 1, j = self->length - 1;
      j >= newParticles;
      --i, --j
    ) {
      items[j] = items[i];
    }
    // phlistnode *node = self->particles.first;
    // for (phint i = newParticles - 1; node && i; --i, node = node->prev) {}
    // for (phint i = self->length - newParticles; node && i; --i, node = node->next) {}
    // for (; node && newParticles >= 0; node = node->next, ++items, --newParticles) {
    //   *items = node->item;
    // }
    // self->_particleArray.capacity = self->length;
    self->_dirtyParticles = 0;
  }
}

phbool phDdvtNext(phddvtiterator *self) {
  if (self->ddvt == self->topDdvt) {
    return 0;
  }
  phbool next = 0;
  while (!next) {
    // if (self->leafItr.list) {
    if (self->arrayItr.test) {
      // next = phListNext(&self->leafItr);
      next = phArrayNext(&self->arrayItr);
      // if (!next) {
      //   self->leafItr.list = NULL;
      // }
    } else {
      // if parent, iterate over tl, then tr, bl, br
      _phDdvtNextChild((phddvtiterator *) self);
      if (self->ddvt == self->topDdvt) {
        break;
      }
      // if leaf, iterate over particles
      if (self->ddvt && !self->ddvt->tl) {
        // if (self->leafItr.list != &self->ddvt->particles) {
        // phIterator(&self->ddvt->particles, &self->leafItr);
        phArrayIterator(&self->ddvt->_particleArray, &self->arrayItr);
        self->arrayItr.test = 1;
        // }
      }
    }
  }
  return next;
}

void * phDdvtDeref(phddvtiterator *self) {
  // if (self->leafItr.list) {
  //   return phListDeref(&self->leafItr);
  // }
  // return NULL;
  return phArrayDeref(&self->arrayItr);
}

phiterator * phDdvtIterator(phddvt *self, phddvtiterator *itr) {
  if (itr == NULL) {
    itr = phAlloc(phddvtiterator);
  }
  *itr = phddvtiterator(self);
  // Dive until the first leaf.
  while (itr->ddvt->tl) {
    itr->ddvt = itr->ddvt->tl;
  }
  // phIterator(&itr->ddvt->particles, &itr->leafItr);
  phArrayIterator(&itr->ddvt->_particleArray, &itr->arrayItr);
  itr->arrayItr.test = 1;
  return (phiterator *) itr;
}

phbool phDdvtPairNext(phddvtpairiterator *self) {
  if (self->ddvt == self->topDdvt) {
    return 0;
  }
  phbool next = 0;
  pharrayiterator *arrayItr1 = &self->arrayItr1;
  pharrayiterator *arrayItr2 = &self->arrayItr2;
  while (!next) {
    // step j and i if possible
    if (arrayItr1->test) {
      // step j
      next = phArrayNext(arrayItr2);
      if (!next) {
        // step i
        next = phArrayNext(arrayItr1);
        // if there is still more, set j = i + 1
        if (next) {
          arrayItr2->items = arrayItr1->items;
          next = phArrayNext(arrayItr2);
        }
        // reached the end of this volume, set i = NULL
        if (!next) {
          arrayItr1->test = 0;
        }
      }
    } else {
      // if parent, iterate over tl, then tr, bl, br
      _phDdvtNextChild((phddvtiterator *) self);
      phddvt *ddvt = self->ddvt;
      if (ddvt == self->topDdvt) {
        break;
      }
      // if leaf, iterate over particles
      if (ddvt && !ddvt->tl) {
        // init the iterators, i = 0, j = i + 1
        pharray *particles = &self->particles;
        // particles->capacity = PH_MAX_DDVT_PARTICLES;
        // phIterator(&ddvt->particles, &self->leafItr1);
        // phUnsafeToArray(&self->leafItr1.iterator, particles);
        // phlistnode *node = ddvt->particles.first;
        // void **items = particles->items;
        // for (; node; node = node->next, ++items) {
        //   *items = node->item;
        // }
        // particles->capacity = ddvt->particles.length;
        // _phDdvtCheckParticleArray(ddvt);
        *particles = ddvt->_particleArray;
        arrayItr1->items = particles->items;
        arrayItr1->end = particles->items + particles->capacity;
        arrayItr1->test = 1;
        arrayItr2->items = arrayItr1->items;
        arrayItr2->end = arrayItr1->end;
        arrayItr2->test = 1;
        next = phArrayNext(arrayItr2);
        // }
      }
    }
  }
  return next;
}

void * phDdvtPairDeref(phddvtpairiterator *self) {
  // if (self->leafItr1.list) {
  if (self->arrayItr1.test) {
    // self->pair.a = phListDeref(&self->leafItr1);
    // self->pair.b = phListDeref(&self->leafItr2);
    self->pair.a = phArrayDeref(&self->arrayItr1);
    self->pair.b = phArrayDeref(&self->arrayItr2);
    return &self->pair;
  }
  return NULL;
}

phiterator * phDdvtPairIterator(phddvt *self, phddvtpairiterator *itr) {
  if (itr == NULL) {
    itr = phAlloc(phddvtpairiterator);
  }
  *itr = phddvtpairiterator(self);
  // Dive until the first leaf.
  while (itr->ddvt->tl) {
    itr->ddvt = itr->ddvt->tl;
  }
  // itr->particles = pharray(PH_MAX_DDVT_PARTICLES, (void **) &itr->_particles._items);
  // phIterator(&itr->ddvt->particles, &itr->leafItr1);
  // phUnsafeToArray(&itr->leafItr1.iterator, &itr->particles);
  // phlistnode *node = itr->ddvt->particles.first;
  // void **items = itr->particles.items;
  // for (; node; node = node->next, ++items) {
  //   *items = node->item;
  // }
  // itr->particles.capacity = itr->ddvt->particles.length;
  // _phDdvtCheckParticleArray(itr->ddvt);
  itr->particles = itr->ddvt->_particleArray;
  phArrayIterator(&itr->particles, &itr->arrayItr1);
  phArrayNext(&itr->arrayItr1);
  // phListNext(&itr->leafItr1);
  // itr->leafItr2 = itr->leafItr1;
  itr->arrayItr2 = itr->arrayItr1;
  return (phiterator *) itr;
}
