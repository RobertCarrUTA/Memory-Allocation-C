#include <stdlib.h>
#include <stdio.h>

int main()
{
  printf("Running test 4 to test a block split and reuse\n");

  char * ptr1 = ( char * ) malloc( 2048 );

  free( ptr1 );

  char * ptr2 = ( char * ) malloc( 1024 );

  free( ptr2 );

  return 0;
}
