#include "Binder.h"

int main(int argc, char* argv[]) {

  // Create a binder server
  Server* binder = new Binder();
  // listen for connections
  binder->execute();

  delete binder;

  return 0;
}
