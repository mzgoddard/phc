#include "ph.h"

void * phFlowCopy(void *_flow, phlist *dst, phlist *src) {
  // TODO
  return NULL;
}

void phFlowCollide(phparticle *a, phparticle *b, phcollision *col) {
  phflow *flow = a->data;
  b->acceleration = phAdd(phScale(flow->force, 1 / b->mass), b->acceleration);
}

phbool phFlowBeforeStep(void *_flow) {
  phflow *flow = _flow;
  flow->particle->data = flow;
  flow->particle->isTrigger = 1;
  flow->particle->isStatic = 1;
  flow->particle->collide = phFlowCollide;

  return 0;
}

void phFlowUpdate(void *_flow, phworld *world) {}

phconstraintid phflowid = "phflow";
phconstrainttype *phflowtype = &(phconstrainttype(
  &phflowid, phFlowCopy, phFlowBeforeStep, phFlowUpdate
));
