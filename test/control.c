

extern void output_int(int num);
extern void output_fp(double num);

int main() {
    int count = 0;

    for (int i = 0; i < 100; i = i + 1) {
        for (int j = 0; j < 100; j = j + 1) {
            count = count + 1;
            break;
            count = count + 1;
        }
    }
    output_int(count); // 100

    count = 0;

    for (int i = 0; i < 100; i = i + 1) {
        for (int j = 0; j < 100; j = j + 1) {
            count = count + 1;
            continue;
            count = count + 1;
        }
    }

    output_int(count); // 10000

    return 0;
}