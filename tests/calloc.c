#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

int main()
{
    int* array;

    array = (int*)calloc(5, sizeof(int));

    assert( array[0] == 0 );
    assert( array[1] == 0 );
    assert( array[2] == 0 );
    assert( array[3] == 0 );
    assert( array[4] == 0 );

    printf("calloc test PASSED\n");

    free(array);

    return (0);
}
