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
  char* arg = (char*) args[0];
  *arg = 'a';
  return 2;
}

int f2(int *argTypes, void **args) {
  char* arg = (char*) args[0];
  *arg = 'a';
  return 2;
}

int f3(int *argTypes, void **args) {
  char* arg = (char*) args[0];
  arg[0] = '1';
  arg[1] = '2';
  arg[2] = '3';
  arg[3] = '4';
  arg[4] = '5';
  return 2;
}

int f4(int *argTypes, void **args) {
  int* arg = (int*) args[0];
  *arg = *arg + 42;
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
        ArgType(NOT_INPUT, OUTPUT, ARG_CHAR, 0),
      });
  rpcRegister("f1", a1, f1);

  int* a2 = formatArgTypes(
      {
        ArgType(INPUT, NOT_OUTPUT, ARG_CHAR, 0),
      });
  rpcRegister("f2", a2, f2);

  int* a3 = formatArgTypes(
      {
        ArgType(INPUT, OUTPUT, ARG_CHAR, 5),
      });
  rpcRegister("f3", a3, f3);

  int* a4 = formatArgTypes(
      {
        ArgType(INPUT, OUTPUT, ARG_INT, 0),
      });
  rpcRegister("f4", a4, f4);

  rpcExecute();

  return 0;
}
