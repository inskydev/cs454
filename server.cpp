#include <algorithm>
#include <list>
#include <string>
#include <cstring>
#include <cstdio>
#include <cstdlib>

#include "util.h"

using namespace std;

int f0(int *argTypes, void **args) {
  return 3;
}

int main(int argc, char* argv[]) {

  /* create sockets and connect to the binder */
  if (rpcInit()) {
    cout << "init failed." << endl;
    return -1;
  }
  cout << "init done." << endl;


  //int* argTypes0 = formatArgTypes(
  //    {
  //      ArgType(INPUT, OUTPUT, ARG_CHAR, 0),
  //      ArgType(INPUT, NOT_OUTPUT, ARG_INT, 2)
  //    });
  // rpcRegister("f0", argTypes0, NULL);

  // (Takes nothing, return nothing), no funciton implemented
  int a = 0;
  rpcRegister("f0", &a, f0);

  rpcExecute();

  return 0;
}
