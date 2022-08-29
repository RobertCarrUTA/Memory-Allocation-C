#include <stdlib.h>
#include <stdio.h>

int main()
{

  printf("Running test 3  to test coalesce\n");

  char * ptr1 = ( char * ) malloc ( 1200 );
  char * ptr2 = ( char * ) malloc ( 1200 );

  free( ptr1 );
  free( ptr2 );

  char * ptr3 = ( char * ) malloc ( 2048 );

  free( ptr3 );

  return 0;
}
