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

int f1(int *argTypes, void **args) {
  return 2;
}



int main(int argc, char* argv[]) {

  /* create sockets and connect to the binder */
  if (rpcInit()) {
    cout << "init failed." << endl;
    return -1;
  }
  cout << "init done." << endl;


  // (Takes nothing, return nothing), no funciton implemented
  int a = 0;
  rpcRegister("f0", &a, f0);

  int* a1 = formatArgTypes(
      {
        ArgType(INPUT, OUTPUT, ARG_CHAR, 0),
      });
  rpcRegister("f1", a1, f1);

  rpcExecute();

  return 0;
}
