#include <stdlib.h>
#include <stdio.h>

int main()
{
  printf("Running test 1 to test a simple malloc and free\n");

  char * ptr = ( char * ) malloc ( 65535 );
  free( ptr ); 

  return 0;
}
