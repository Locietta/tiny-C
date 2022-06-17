gcc_lib_version=$1
my_stblib_path="mystdlib/libmystd.a"
output_name=$2
shift
shift

ld.lld -pie --eh-frame-hdr -m elf_x86_64 -dynamic-linker /lib64/ld-linux-x86-64.so.2\
 -o $output_name \
/usr/lib64/Scrt1.o /usr/lib64/crti.o /usr/lib64/gcc/x86_64-pc-linux-gnu/$gcc_lib_version/crtbeginS.o -L/usr/lib64/gcc/x86_64-pc-linux-gnu/$gcc_lib_version -L/usr/lib64 -L/lib64 -L/usr/lib -L/lib \
 $* $my_stblib_path \
 -lgcc --as-needed -lgcc_s --no-as-needed -lc -lgcc --as-needed -lgcc_s --no-as-needed /usr/lib64/gcc/x86_64-pc-linux-gnu/$gcc_lib_version/crtendS.o /usr/lib64/crtn.o