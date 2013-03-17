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
  int* arg1 = (int*) args[0];
  *arg1 = *arg1 + 42;
  double* arg2 = (double*) args[1];
  *arg2 += 6.66;
  return 2;
}

int f5(int *argTypes, void **args) {
  int* arg1 = (int*) args[0];
  *arg1 = *arg1 + 42;
  double* arg2 = (double*) args[1];
  *arg2 += 6.66;
  short* arg3 = (short*) args[2];
  short* arg4 = (short*) args[3];
  arg4[0] = arg3[0];
  arg4[1] = arg3[1];
  arg4[2] = arg3[2];

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
        ArgType(INPUT, OUTPUT, ARG_DOUBLE, 0),
      });
  rpcRegister("f4", a4, f4);

  int* a5 = formatArgTypes(
      {
        ArgType(INPUT, OUTPUT, ARG_INT, 0),
        ArgType(INPUT, OUTPUT, ARG_DOUBLE, 0),
        ArgType(INPUT, NOT_OUTPUT, ARG_SHORT, 7),
        ArgType(NOT_INPUT, OUTPUT, ARG_SHORT, 7),
      });
  rpcRegister("f5", a5, f5);

  rpcExecute();

  return 0;
}
