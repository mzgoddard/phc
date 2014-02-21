#include <math.h>
#include <stdlib.h>

#include "tap.h"
#include "ph.h"

void test_math();
void test_iterator();
void test_list();

int main() {
  ansicolor(getenv( "ANSICOLOR" ) != NULL);
  plan(37);

  test_math();
  test_iterator();
  test_list();

  done_testing();
}
