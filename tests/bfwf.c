#include <stdlib.h>
#include <stdio.h>
#include <string.h>

int main()
{
  char * ptr1 = ( char * ) malloc ( 65535 );
  char * buffer1 = ( char * ) malloc( 1 );
  char * ptr4 = ( char * ) malloc ( 65 );
  char * buffer2 = ( char * ) malloc( 1 );
  char * ptr2 = ( char * ) malloc ( 6000 );

  printf("Worst fit should pick this one: %p\n", ptr1 );
  printf("Best fit should pick this one: %p\n", ptr2 );

  free( ptr1 ); 
  free( ptr2 ); 

  buffer1 = buffer1;
  buffer2 = buffer2;
  ptr4 = ptr4;

  char * ptr3 = ( char * ) malloc ( 1000 );
  printf("Chosen address: %p\n", ptr3 );

  return 0;
}

