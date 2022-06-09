cd mystdlib
clang -c -O3 *.c
ar crv libmystd.a *.o
rm -f *.o