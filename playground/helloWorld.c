int main()
{
    const int N = 1000 * 4;
    int array[N];

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
