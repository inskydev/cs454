#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

#include "util.h"
#include "Transporter.cpp"

int main(int argc, char *argv[]) {

  int argTypes = 0;
  void** args0 = NULL; //(void **)malloc(0 * sizeof(void *));
  int s4 = rpcCall("f0", &argTypes, NULL);
  cout << "f0:" << s4 << endl;

  {
    int* a1 = formatArgTypes(
        {
        ArgType(NOT_INPUT, OUTPUT, ARG_CHAR, 0),
        });

    char value = 'v';
    void** args = (void**)malloc(sizeof(void*) * 1);
    args[0] = &value;

    int s1 = rpcCall("f1", a1, args);
    cout << "c1: " << s1 << endl;
    cout << value << endl;
  }

  {
    int* a2 = formatArgTypes(
      {
        ArgType(INPUT, NOT_OUTPUT, ARG_CHAR, 0),
      });
    char value = 'v';
    void** args = (void**)malloc(sizeof(void*) * 1);
    args[0] = &value;

    int s = rpcCall("f2", a2, args);
    cout << "c1: " << s << endl;
    cout << value << endl;
  }

  {
    int* a3 = formatArgTypes(
      {
        ArgType(INPUT, OUTPUT, ARG_CHAR, 5),
      });
      char* value = new char[5];
      value[0] = 0;
      value[1] = 0;
      value[2] = 0;
      value[3] = 0;
      value[4] = 0;
      void** args = (void**)malloc(sizeof(void*) * 1);
      args[0] = value;
      int s = rpcCall("f3", a3, args);
      cout << "c3: " << s << endl;
      cout << value[0] << endl;
      cout << value[1] << endl;
      cout << value[2] << endl;
      cout << value[3] << endl;
      cout << value[4] << endl;
  }
  {
    int* a4 = formatArgTypes(
      {
        ArgType(INPUT, OUTPUT, ARG_INT, 0),
      });

    int test = 4;
    void** args = (void**)malloc(sizeof(void*) * 1);
    args[0] = &test;
    int s = rpcCall("f4", a4, args);
    cout << "c3: " << s << endl;
    cout << test << endl;
  }
  //int rc = rpcTerminate();
  //cout << rc << endl;

}
