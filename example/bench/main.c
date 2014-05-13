#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#if EMSCRIPTEN
#include <emscripten.h>
#endif

#include "ph.h"

#include "../simple/env.h"

clock_t start;
clock_t end;
time_t starttime;
time_t endtime;
phbool loop;
int frames = 0;
phdouble length = 10.0;

void main_loop();
void sigint(int);

int main(int argc, char **argv) {
  init();

  if (argc == 2) {
    if (strncmp(argv[1], "inf\0", 4) == 0) {
      length = INFINITY;
    } else {
      length = atof(argv[1]);
    }
  }

  // Clock based time is cpu clock. With threads, performance in comparison to
  // cpu clock will always be less than a single main thread that does not need
  // to deal with inter thread communication.
  start = end = clock();
  loop = 1;
  // TODO: Find a non-cpu-clock that more accurately reports time. Time is
  // limited to seconds.
  starttime = endtime = time(NULL);

  #if EMSCRIPTEN
  emscripten_set_main_loop(main_loop, 0, 0);
  #else
  signal(SIGINT, sigint);
  while (loop) {
    main_loop();
  }
  #endif
}

void main_loop() {
  for (int i = 0; i < 100; ++i) {
    step(1 / 60.0);
  }
  frames += 100;
  #if EMSCRIPTEN
  printf(".\n");
  #else
  printf(".");
  fflush(stdout);
  #endif

  // phdouble t = ((end = clock()) - (phdouble) start) / CLOCKS_PER_SEC;
  phdouble t = difftime(endtime = time(NULL), starttime);
  if (t >= length) {
    loop = 0;

    printf("\n");

    printf("%d frames in %g seconds\n", frames, t);
    printf("%g frames per second\n", frames / t);
    printf("%g milliseconds per frame\n", t / frames * 1000);

    #if EMSCRIPTEN
    exit(0);
    #endif
  }
}

void sigint(int sig) {
  length = 0;
}
