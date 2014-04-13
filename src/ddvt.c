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
  phDump(&self->particles, NULL);
}

void _phDdvtAddLeaf(phddvt *self, phparticle *particle) {
  phAppend(&self->particles, particle);
  self->length++;
}

void _phDdvtRemoveLeaf(phddvt *self, phparticle *particle) {
  phRemove(&self->particles, particle);
  self->length--;
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
  // aqaabb aabb = AQParticle_aabb( particle );
  // aqaabb lastAabb = AQParticle_lastAabb( particle );

  phbool updated = 0;
  phbool added = 0;
  phbool removed = 0;

  #define INTERSECT_AND_UPDATE(corner) \
    phint corner ## Intersect = phIntersect(self->corner->box, *new); \
    phint corner ## LastIntersect = phIntersect(self->corner->box, *old); \
    \
    if (corner ## Intersect && corner ## LastIntersect) { \
      _phDdvtUpdate(self->corner, particle, old, new); \
      updated = 1; \
    } else if (corner ## Intersect) { \
      _phDdvtAdd(self->corner, particle, new); \
      added = 1; \
    } else if (corner ## LastIntersect) { \
      _phDdvtRemove(self->corner, particle, old); \
      removed = 1; \
    }

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
  phbox box = particle->_worldData->oldBox;
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
      self->minParticles, \
      self->maxParticles \
    )
  ALLOC_AND_INIT(tl, TL);
  ALLOC_AND_INIT(tr, TR);
  ALLOC_AND_INIT(bl, BL);
  ALLOC_AND_INIT(br, BR);
  #undef ALLOC_AND_INIT

  phlistiterator _listitr;
  phiterator *itr = phIterator(&self->particles, &_listitr);
  phIterate(itr, (phitrfn) _phDdvtTestAndAdd, self);
  phClean(&self->particles, NULL);
}

void _phDdvtAddIfUnique(phddvt *self, phparticle *particle) {
  phlistiterator _listitr;
  if (!phContains(phIterator(&self->particles, &_listitr), particle)) {
    _phDdvtAddLeaf(self, particle);
  }
}

void _phDdvtFromChildren(phddvt *self) {
  self->length = 0;
  phlistiterator _listitr;
  phiterator *itr;
  #define ITR_ADD_UNIQUE(corner) \
    itr = phIterator(&self->corner->particles, &_listitr); \
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

  if (!self->tl && self->length < self->maxParticles) {
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

    if (self->length <= self->minParticles) {
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
  if (!self->tl) {
    _phDdvtUpdateLeaf(self, particle, old, new);
  } else {
    _phDdvtUpdateChild(self, particle, old, new);

    if (self->length <= self->minParticles) {
      _phDdvtFromChildren(self);
    }
  }
}

void phDdvtAdd(phddvt *self, phparticle *particle) {
  phbox box = particle->_worldData->oldBox;
  _phDdvtAdd(self, particle, &box);
}

void phDdvtRemove(phddvt *self, phparticle *particle, phbox box) {
  _phDdvtRemove(self, particle, &box);
}

void phDdvtWake(phddvt *self, phparticle *particle) {
  phbox box = particle->_worldData->oldBox;
  _phDdvtWake(self, particle, &box);
}

void phDdvtUpdate(phddvt *self, phparticle *particle, phbox old, phbox new) {
  _phDdvtUpdate(self, particle, &old, &new);
}

void _phDdvtNextChild(phddvtiterator *self) {
  if (self->leafItr.list == NULL) {
    if (self->ddvt->parent && self->ddvt == self->ddvt->parent->tl) {
      self->ddvt = self->ddvt->parent->tr;
      while (self->ddvt->tl) {
        self->ddvt = self->ddvt->tl;
      }
    } else if (self->ddvt->parent && self->ddvt == self->ddvt->parent->tr) {
      self->ddvt = self->ddvt->parent->bl;
      while (self->ddvt->tl) {
        self->ddvt = self->ddvt->tl;
      }
    } else if (self->ddvt->parent && self->ddvt == self->ddvt->parent->bl) {
      self->ddvt = self->ddvt->parent->br;
      while (self->ddvt->tl) {
        self->ddvt = self->ddvt->tl;
      }
    } else {
      self->ddvt = self->ddvt->parent;
    }
  }
}

phbool phDdvtNext(phddvtiterator *self) {
  phbool next = 0;
  while (!next && self->ddvt != self->topDdvt) {
    // if parent, iterate over tl, then tr, bl, br
    _phDdvtNextChild((phddvtiterator *) self);
    // if leaf, iterate over particles
    if (self->ddvt && !self->ddvt->tl) {
      if (self->leafItr.list != &self->ddvt->particles) {
        phIterator(&self->ddvt->particles, &self->leafItr);
      }
      next = phListNext(&self->leafItr);
      if (!next) {
        self->leafItr.list = NULL;
      }
    }
  }
  return next;
}

void * phDdvtDeref(phddvtiterator *self) {
  if (self->leafItr.list) {
    return phListDeref(&self->leafItr);
  }
  return NULL;
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
  phIterator(&itr->ddvt->particles, &itr->leafItr);
  return (phiterator *) itr;
}

phbool phDdvtPairNext(phddvtpairiterator *self) {
  phbool next = 0;
  while (!next && self->ddvt != self->topDdvt) {
    // if parent, iterate over tl, then tr, bl, br
    _phDdvtNextChild((phddvtiterator *) self);
    // if leaf, iterate over particles
    if (self->ddvt && !self->ddvt->tl) {
      // init the iterators, i = 0, j = i + 1
      if (self->leafItr1.list != &self->ddvt->particles) {
        phIterator(&self->ddvt->particles, &self->leafItr1);
        phListNext(&self->leafItr1);
        self->leafItr2 = self->leafItr1;
      }
      // step j
      next = phListNext(&self->leafItr2);
      if (!next) {
        // step i
        next = phListNext(&self->leafItr1);
        // if there is still more, set j = i + 1
        if (next) {
          self->leafItr2 = self->leafItr1;
          next = phListNext(&self->leafItr2);
        }
      }
      // reached the end of this volume, set i = NULL
      if (!next) {
        self->leafItr1.list = NULL;
      }
    }
  }
  return next;
}

void * phDdvtPairDeref(phddvtpairiterator *self) {
  if (self->leafItr1.list) {
    self->pair.a = phListDeref(&self->leafItr1);
    self->pair.b = phListDeref(&self->leafItr2);
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
  phIterator(&itr->ddvt->particles, &itr->leafItr1);
  phListNext(&itr->leafItr1);
  itr->leafItr2 = itr->leafItr1;
  return (phiterator *) itr;
}
