
int x = 1;
char z = '\n';
float y = 0.5;

int f(int n) {
    if (n == 0) {
        return 0;
    } else if (n == 1) {
        return 1;
    } else {
        return f(n - 1) + f(n - 2);
    }
}

int main(void) {
    if (x == 1) {
        x = 2;
    } else
        x = 3;

    char ch = 'c';

    return 0;
}
