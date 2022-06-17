

typedef int a;

a aa = 1;

extern void output_int(int num);
extern void output_fp(double num);

int main() {
    typedef double a;
    a f = 1.2; // double
    output_fp(f);
    output_int(aa);
    return 0;
}