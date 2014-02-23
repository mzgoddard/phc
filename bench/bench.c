#include <stdarg.h>
#include <time.h>

#include "tap.h"
#include "ph.h"
#include "bench.h"

static struct {
  char *clear;
  char *red;
  char *green;
  char *yellow;
  char *cyan;
  char *gray;
} ansicodes = {
  "\x1b[0m",
  "\x1b[31m",
  "\x1b[32m",
  "\x1b[33m",
  "\x1b[36m",
  "\x1b[30;1m"
};

static phbool bench_ansicolor = 0;

void bench_math();
void bench_iterator();

#define ANSI(_code_) (char *) (bench_ansicolor ? (ansicodes._code_) : "")

phdouble runiterations(phint n, benchfn fn, void *ctx) {
  clock_t start = clock();
  for (phint i = 0; i < n; ++i) {fn(ctx);}
  return (clock() - start) / (phdouble) CLOCKS_PER_SEC;
}

void bench(char *name, benchfn fn, void *ctx, ...) {
  phdouble t = 0;
  phint n = 0;
  va_list args;
  va_start(args, ctx);
  while (name != NULL) {
    while (t < 0.1) {
      if (n == 0) {
        n = 1;
      } else {
        n *= 10;
      }
      printf("\r%s%s%s: run %s%d%s iterations",
        ANSI(green), name, ANSI(clear),
        ANSI(cyan), n, ANSI(clear)
      );
      fflush(stdout);
      t = runiterations(n, fn, ctx);
    }
    printf("\r%s%s%s: %s%.3g%s iterations in %s%.3g%s second.\n",
      ANSI(green), name, ANSI(clear),
      ANSI(cyan), n / t, ANSI(clear),
      ANSI(cyan), t / t, ANSI(clear)
    );
    fflush(stdout);

    n = 0;
    t = 0;
    name = va_arg(args, char *);
    if (name == NULL) {break;}
    fn = va_arg(args, benchfn);
    if (fn == NULL) {break;}
    ctx = va_arg(args, void *);
  }
  va_end(args);
}

int main() {
  bench_ansicolor = getenv("ANSICOLOR") != NULL;
  ansicolor(bench_ansicolor);
  plan(2);

  bench_math();
  bench_iterator();

  done_testing();
}
