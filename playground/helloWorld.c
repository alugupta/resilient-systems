#include <stdio.h>
#include <stdlib.h>

int main()
{
    const int N = 10;
    int array[N];

    if (NULL == array) {
        printf("malloc failed \n");
        return -1;
    }

    int i;
    for (i = 0; i < N; i++) {
        array[i] = i;
    }

    int value;
    for (i = 0; i < N; i++) {
        value = array[i];
    }

    return 0;
}
