#include "Binder.h"

int main(int argc, char* argv[]) {
  bool debug = argc > 1;
  bool put_file = argc > 2;

  Binder binder;
  binder.execute();

  return 0;
}
