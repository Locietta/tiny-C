first=$1
my_stblib_path="mystdlib/libmystd.a"
shift

ld.lld -pie --eh-frame-hdr -m elf_x86_64 -dynamic-linker /lib64/ld-linux-x86-64.so.2\
 -o $first \
/usr/sbin/../lib64/gcc/x86_64-pc-linux-gnu/12.1.0/../../../../lib64/Scrt1.o /usr/sbin/../lib64/gcc/x86_64-pc-linux-gnu/12.1.0/../../../../lib64/crti.o /usr/sbin/../lib64/gcc/x86_64-pc-linux-gnu/12.1.0/crtbeginS.o -L/usr/sbin/../lib64/gcc/x86_64-pc-linux-gnu/12.1.0 -L/usr/sbin/../lib64/gcc/x86_64-pc-linux-gnu/12.1.0/../../../../lib64 -L/lib/../lib64 -L/usr/lib/../lib64 -L/usr/bin/../lib -L/lib -L/usr/lib \
 $* $my_stblib_path \
 -lgcc --as-needed -lgcc_s --no-as-needed -lc -lgcc --as-needed -lgcc_s --no-as-needed /usr/sbin/../lib64/gcc/x86_64-pc-linux-gnu/12.1.0/crtendS.o /usr/sbin/../lib64/gcc/x86_64-pc-linux-gnu/12.1.0/../../../../lib64/crtn.o