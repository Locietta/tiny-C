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

int gcd(int a, int b);

void output_int(int num);

typedef int aaa;

static int bar(int i, int j);

void nothing() {
    return;
}

int main(void) {
    int sum = 100;

    // for (typedef int bbb; sum == 100; sum = sum + 1) {
    //     bbb six = 6;
    //     sum = sum + six;
    // }

    int i = 0;
    while (i < 100) {
        sum = sum + i;
        i = i + 1;
        continue;
        i = i + 2;
    }
    // haha i = 1;

    for (aaa i = 0; i < 100; i = i + 1) {
        for (int j = 0; j < 100; j = j + 1) {
            sum = foo(sum, 1);
            continue;
        }
    }

    // foo(sum, sum);
    output_int(gcd(sum, 120));
    return 0;
}

int gcd(int a, int b) {
    if (b) {
        return gcd(b, a % b);
    } else {
        return a;
    }
}