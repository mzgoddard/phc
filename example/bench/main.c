#include <stdio.h>
#include <time.h>

#include "ph.h"

#include "../simple/env.h"

int main(int argc, char **argv) {
  init();
  int frames = 0;

  clock_t start = clock();
  clock_t end;

  while (((end = clock()) - start) / CLOCKS_PER_SEC < 10) {
    for (int i = 0; i < 100; ++i) {
      step(1 / 60.0);
    }
    frames += 100;
    printf(".");
    fflush(stdout);
  }

  printf("\n");

  phdouble t = (end - (phdouble) start) / CLOCKS_PER_SEC;
  printf("%d frames in %g seconds\n", frames, t);
  printf("%g frames per second\n", frames / t);
}
