#include "tap.h"
#include "ph.h"

typedef void (*benchfn)(void *);

void bench(char *, benchfn, void *, ...);
