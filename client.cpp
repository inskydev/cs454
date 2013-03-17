#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

#include "util.h"
#include "Transporter.cpp"

int main(int argc, char *argv[]) {
  int argTypes = 0;
  rpcCacheCall("f0", &argTypes, NULL);
  rpcCacheCall("f0", &argTypes, NULL);
  rpcCacheCall("f0", &argTypes, NULL);

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
        ArgType(INPUT, OUTPUT, ARG_DOUBLE, 0),
      });

    int v1 = 4;
    double v2 = 4.0;
    void** args = (void**)malloc(sizeof(void*) * 2);
    args[0] = &v1;
    args[1] = &v2;
    int s = rpcCall("f4", a4, args);
    cout << "c3: " << s << endl;
    cout << v1 << endl;
    cout << v2 << endl;
  }
  {
      int* a5 = formatArgTypes(
      {
        ArgType(INPUT, OUTPUT, ARG_CHAR, 12),
        ArgType(INPUT, OUTPUT, ARG_DOUBLE, 0),
        ArgType(INPUT, NOT_OUTPUT, ARG_SHORT, 3),
        ArgType(NOT_INPUT, OUTPUT, ARG_SHORT, 3),
      });

    char v1[] = "#1;2#;@;aaaa";
    double v2 = 4.0;
    short v3[]  = {1, 2, 3};
    short v4[]  = {0, 0, 0};
    void** args = (void**)malloc(sizeof(void*) * 4);
    args[0] = &v1;
    args[1] = &v2;
    args[2] = &v3;
    args[3] = &v4;
    int s = rpcCall("f5", a5, args);
    cout << "c5: " << s << endl;
    cout << string(v1) << endl;
    cout << v2 << endl;
    cout << v3[0] << v3[1] << v3[2] << endl;
    cout << v4[0] << v4[1] << v4[2] << endl;
  }

  //int rc = rpcTerminate();
  //cout << rc << endl;

}
