#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <assert.h>
#include "encode.h"

int main(int argc, char **argv)
{
  if (argc != 3) {
    fprintf(stderr, "usage: %s <input file> [output file]\n",argv[0]);
    exit(1);
  }

  encode(argv[1], argv[2]);

  return 0;
}
