#include <stdio.h>

int x = 3, y = 4;

// int foo(int a, int b) {
//     int sum = 0;
//     for (int i = 0; i <= 10; i = i + 1) {
//         sum = sum + i * a - b;
//     }
//     return sum;
// }

int main(void) {
    int sum = 0, a = 1;
    // for (int i = 0; i <= 10; i = i + 1) {
    //     sum = sum + 1;
    // }
    for (;;) {
        sum = sum + 1;
        return sum;
    }
    sum = sum * sum;

    return sum;
    // ;
    // for (;;) return 1;
    // while (0) {
    //     return 2;
    // }
    // return 1;
    // int sum = 0;

    // for (int i = 0; i <= 10; i = i + 1) {
    //     for (int j = 0; j <= 10; j = j + 1) {
    //         sum = sum + i * j;
    //     }
    // }

    // int i = 0;
    // for (; i <= 100; i = i + 1) {
    //     if (i == 73) break;
    // }
    // if (i == 73) return 233;

    // printf("%d\n", sum);
    // if (sum == 3025) return 114;
    return 0;
}
