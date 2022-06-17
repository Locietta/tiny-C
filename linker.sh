first=$1
my_stblib_path="mystdlib/libmystd.a"
shift

ld.lld -pie --eh-frame-hdr -m elf_x86_64 -dynamic-linker /lib64/ld-linux-x86-64.so.2\
 -o $first \
/usr/lib64/Scrt1.o /usr/lib64/crti.o /usr/lib64/gcc/x86_64-pc-linux-gnu/12.1.0/crtbeginS.o -L/usr/lib64/gcc/x86_64-pc-linux-gnu/12.1.0 -L/usr/lib64 -L/lib64 -L/usr/lib -L/lib \
 $* $my_stblib_path \
 -lgcc --as-needed -lgcc_s --no-as-needed -lc -lgcc --as-needed -lgcc_s --no-as-needed /usr/lib64/gcc/x86_64-pc-linux-gnu/12.1.0/crtendS.o /usr/lib64/crtn.o