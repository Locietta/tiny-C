// #include <stdio.h>

int x = 3, y = 4;

int foo(int a, int b) {
    int sum;
    sum = a + b;
    return sum;
}

int fib(int n) {
    if (n <= 1)
        return 1;
    else
        return fib(n - 1) + fib(n - 2);
}

int main(void) {
    int sum = 100;
    // int i = 0;
    // while (i < 100) {
    //     sum = sum + i;
    //     i = i + 1;
    //     break;
    //     i = i + 2;
    // }

    for (int i = 0; i < 100; i = i + 1) {
        for (int j = 0; j < 100; j = j + 1) {
            sum = foo(sum, 1);
            continue;
        }
    }

    // foo(sum, sum);
    return sum;
}
