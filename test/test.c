#include "tap.h"
#include "ph.h"

void test_math();
void test_iterator();
void test_list();
void test_particle();
void test_ddvt();

int main() {
  ansicolor(getenv( "ANSICOLOR" ) != NULL);
  plan(37);

  test_math();
  test_iterator();
  test_list();
  test_particle();
  test_ddvt();

  done_testing();
}
