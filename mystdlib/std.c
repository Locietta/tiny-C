#include <stdio.h>

/**
 * simple std lib for tinycc
 */

int input_int() {
    int ret;
    scanf("%d", &ret);
    return ret;
}

void output_int(int num) {
    printf("%d\n", num);
}

void output_char(char ch) {
    printf("%c\n", ch);
}