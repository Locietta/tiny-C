// #include <stdio.h>

int x = 3, y = 4;

int foo(int a, int b) {
    int sum;
    sum = a + b;
    return sum;
}

// int add(int a, int b) {
//     if (a != 0) {
//         return add(a - 1, b + 1);
//     }
//     return b;
// }

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
