########################## grep ##########################
wget https://ftp.gnu.org/gnu/grep/grep-3.6.tar.xz
tar -xf grep-3.6.tar.xz
cd grep-3.6

mkdir obj-llvm
cd obj-llvm
CC=wllvm CFLAGS="-g -O1 -Xclang -disable-llvm-passes -D__NO_STRING_INLINES -D_FORTIFY_SOURCE=0 -U__OPTIMIZE__" ../configure --disable-largefile --disable-threads --disable-nls --disable-perl-regexp --with-included-regex
make
cd src
find . -executable -type f | xargs -I '{}' extract-bc '{}'
cd ../..

mkdir obj-ubsan
cd obj-ubsan
CC=wllvm CFLAGS="-g -O1 -Xclang -disable-llvm-passes -D__NO_STRING_INLINES -D_FORTIFY_SOURCE=0 -U__OPTIMIZE__ -fsanitize=signed-integer-overflow -fsanitize=unsigned-integer-overflow -fsanitize=shift -fsanitize=bounds -fsanitize=pointer-overflow -fsanitize=null" ../configure --disable-largefile --disable-threads --disable-nls --disable-perl-regexp --with-included-regex
make
cd src
find . -executable -type f | xargs -I '{}' extract-bc '{}'
cd ../..


########################## gawk ##########################
wget https://ftp.gnu.org/gnu/gawk/gawk-5.1.0.tar.xz
tar -xf gawk-5.1.0.tar.xz
cd gawk-5.1.0

mkdir obj-llvm
cd obj-llvm
CC=wllvm CFLAGS="-g -O1 -Xclang -disable-llvm-passes -D__NO_STRING_INLINES -D_FORTIFY_SOURCE=0 -U__OPTIMIZE__" ../configure --disable-mpfr --disable-largefile --disable-nls
make
extract-bc gawk
cd ..

mkdir obj-ubsan
cd obj-ubsan
CC=wllvm CFLAGS="-g -O1 -Xclang -disable-llvm-passes -D__NO_STRING_INLINES -D_FORTIFY_SOURCE=0 -U__OPTIMIZE__ -fsanitize=signed-integer-overflow -fsanitize=unsigned-integer-overflow -fsanitize=shift -fsanitize=bounds -fsanitize=pointer-overflow -fsanitize=null" ../configure --disable-mpfr --disable-largefile --disable-nls
make
extract-bc gawk
cd ..


########################## binutils (objcopy and readelf) ##########################
wget https://ftp.gnu.org/gnu/binutils/binutils-2.36.tar.xz
tar -xf binutils-2.36.tar.xz
cd binutils-2.36

mkdir obj-llvm
cd obj-llvm
CC=wllvm CFLAGS="-g -O1 -Xclang -disable-llvm-passes -D__NO_STRING_INLINES -D_FORTIFY_SOURCE=0 -U__OPTIMIZE__" ../configure --disable-nls --disable-largefile --disable-gdb --disable-sim --disable-readline --disable-libdecnumber --disable-libquadmath --disable-libstdcxx --disable-ld --disable-gprof --disable-gas --disable-intl --disable-etc
make
cd binutils
find . -executable -type f | xargs -I '{}' extract-bc '{}'
cd ../..

mkdir obj-ubsan
cd obj-ubsan
CC=wllvm CFLAGS="-g -O1 -Xclang -disable-llvm-passes -D__NO_STRING_INLINES -D_FORTIFY_SOURCE=0 -U__OPTIMIZE__ -fsanitize=signed-integer-overflow -fsanitize=unsigned-integer-overflow -fsanitize=shift -fsanitize=bounds -fsanitize=pointer-overflow -fsanitize=null" ../configure --disable-nls --disable-largefile --disable-gdb --disable-sim --disable-readline --disable-libdecnumber --disable-libquadmath --disable-libstdcxx --disable-ld --disable-gprof --disable-gas --disable-intl --disable-etc
make
cd binutils
find . -executable -type f | xargs -I '{}' extract-bc '{}'
cd ../..


########################## find ##########################
wget https://ftp.gnu.org/gnu/findutils/findutils-4.7.0.tar.xz
tar -xf findutils-4.7.0.tar.xz
cd findutils-4.7.0

mkdir obj-llvm
cd obj-llvm
CC=wllvm CFLAGS="-g -O1 -Xclang -disable-llvm-passes -D__NO_STRING_INLINES -D_FORTIFY_SOURCE=0 -U__OPTIMIZE__" ../configure --disable-nls --disable-largefile --disable-threads --without-selinux
make
cd find
extract-bc find
cd ../..

mkdir obj-ubsan
cd obj-ubsan
CC=wllvm CFLAGS="-g -O1 -Xclang -disable-llvm-passes -D__NO_STRING_INLINES -D_FORTIFY_SOURCE=0 -U__OPTIMIZE__ -fsanitize=signed-integer-overflow -fsanitize=unsigned-integer-overflow -fsanitize=shift -fsanitize=bounds -fsanitize=pointer-overflow -fsanitize=null" ../configure --disable-nls --disable-largefile --disable-threads --without-selinux
make
cd find
extract-bc find
cd ../..


########################## make ##########################
wget https://ftp.gnu.org/gnu/make/make-4.3.tar.gz
tar -xf make-4.3.tar.gz
cd make-4.3

mkdir obj-llvm
cd obj-llvm
CC=wllvm CFLAGS="-g -O1 -Xclang -disable-llvm-passes -D__NO_STRING_INLINES -D_FORTIFY_SOURCE=0 -U__OPTIMIZE__" ../configure --disable-nls --disable-largefile --disable-job-server --disable-load
make
extract-bc make
cd ..

mkdir obj-ubsan
cd obj-ubsan
CC=wllvm CFLAGS="-g -O1 -Xclang -disable-llvm-passes -D__NO_STRING_INLINES -D_FORTIFY_SOURCE=0 -U__OPTIMIZE__ -fsanitize=signed-integer-overflow -fsanitize=unsigned-integer-overflow -fsanitize=shift -fsanitize=bounds -fsanitize=pointer-overflow -fsanitize=null" ../configure --disable-nls --disable-largefile --disable-job-server --disable-load
make
extract-bc make
cd ..


########################## sqlite3 ##########################
mkdir sqlite3
cd sqlite3
wget https://www.sqlite.org/2022/sqlite-autoconf-3390400.tar.gz
tar -xf sqlite-autoconf-3390400.tar.gz sqlite
mv sqlite-autoconf-3390400 sqlite3
cd sqlite3

mkdir obj-llvm
cd obj-llvm
wllvm -g -O1 -Xclang -disable-llvm-passes -D__NO_STRING_INLINES -D_FORTIFY_SOURCE=0 -U__OPTIMIZE__ -DSQLITE_THREADSAFE=0 -DSQLITE_OMIT_LOAD_EXTENSION -DSQLITE_DEFAULT_MEMSTATUS=0 -DSQLITE_MAX_EXPR_DEPTH=0 -DSQLITE_OMIT_DECLTYPE -DSQLITE_OMIT_DEPRECATED -DSQLITE_DEFAULT_PAGE_SIZE=512 -DSQLITE_DEFAULT_CACHE_SIZE=10 -DSQLITE_DISABLE_INTRINSIC -DSQLITE_DISABLE_LFS -DYYSTACKDEPTH=20 -DSQLITE_OMIT_LOOKASIDE -DSQLITE_OMIT_WAL -DSQLITE_OMIT_PROGRESS_CALLBACK -DSQLITE_DEFAULT_LOOKASIDE='64,5' -DSQLITE_OMIT_PROGRESS_CALLBACK -DSQLITE_OMIT_SHARED_CACHE -I. ../shell.c ../sqlite3.c -o sqlite3
find . -executable -type f | xargs -I '{}' extract-bc '{}'
cd ../..

mkdir obj-ubsan
cd obj-ubsan
wllvm -g -O1 -Xclang -disable-llvm-passes -D__NO_STRING_INLINES -D_FORTIFY_SOURCE=0 -U__OPTIMIZE__ -fsanitize=signed-integer-overflow -fsanitize=unsigned-integer-overflow -fsanitize=shift -fsanitize=bounds -fsanitize=pointer-overflow -fsanitize=null -DSQLITE_THREADSAFE=0 -DSQLITE_OMIT_LOAD_EXTENSION -DSQLITE_DEFAULT_MEMSTATUS=0 -DSQLITE_MAX_EXPR_DEPTH=0 -DSQLITE_OMIT_DECLTYPE -DSQLITE_OMIT_DEPRECATED -DSQLITE_DEFAULT_PAGE_SIZE=512 -DSQLITE_DEFAULT_CACHE_SIZE=10 -DSQLITE_DISABLE_INTRINSIC -DSQLITE_DISABLE_LFS -DYYSTACKDEPTH=20 -DSQLITE_OMIT_LOOKASIDE -DSQLITE_OMIT_WAL -DSQLITE_OMIT_PROGRESS_CALLBACK -DSQLITE_DEFAULT_LOOKASIDE='64,5' -DSQLITE_OMIT_PROGRESS_CALLBACK -DSQLITE_OMIT_SHARED_CACHE -I. ../shell.c ../sqlite3.c -o sqlite3
find . -executable -type f | xargs -I '{}' extract-bc '{}'
cd ../..


########################## sed ##########################
wget https://ftp.gnu.org/gnu/sed/sed-4.8.tar.gz
tar -xf sed-4.8.tar.gz
cd sed-4.8

mkdir obj-llvm
cd obj-llvm
CC=wllvm CFLAGS="-g -O1 -Xclang -disable-llvm-passes -D__NO_STRING_INLINES -D_FORTIFY_SOURCE=0 -U__OPTIMIZE__" ../configure --disable-nls --disable-largefile
make
cd sed
extract-bc sed
cd ..

mkdir obj-ubsan
cd obj-ubsan
CC=wllvm CFLAGS="-g -O1 -Xclang -disable-llvm-passes -D__NO_STRING_INLINES -D_FORTIFY_SOURCE=0 -U__OPTIMIZE__ -fsanitize=signed-integer-overflow -fsanitize=unsigned-integer-overflow -fsanitize=shift -fsanitize=bounds -fsanitize=pointer-overflow -fsanitize=null" ../configure --disable-nls --disable-largefile
make
cd sed
extract-bc sed
cd ..


########################## nasm ##########################
wget https://www.nasm.us/pub/nasm/releasebuilds/2.15.05/nasm-2.15.05.tar.xz
tar -xf nasm-2.15.058.tar.xz
cd nasm-2.15.05

mkdir obj-llvm
cd obj-llvm
CC=wllvm CFLAGS="-g -O1 -Xclang -disable-llvm-passes -D__NO_STRING_INLINES -D_FORTIFY_SOURCE=0 -U__OPTIMIZE__" ../configure --disable-largefile
make
extract-bc nasm
cd ..

mkdir obj-ubsan
cd obj-ubsan
CC=wllvm CFLAGS="-g -O1 -Xclang -disable-llvm-passes -D__NO_STRING_INLINES -D_FORTIFY_SOURCE=0 -U__OPTIMIZE__ -fsanitize=signed-integer-overflow -fsanitize=unsigned-integer-overflow -fsanitize=shift -fsanitize=bounds -fsanitize=pointer-overflow -fsanitize=null" ../configure --disable-largefile
make
extract-bc nasm
cd ..


########################## expat ##########################
wget https://github.com/libexpat/libexpat/releases/download/R_2_5_0/expat-2.5.0.tar.gz
tar -xf expat-2.5.0.tar.gz
cd expat-2.5.0

mkdir obj-llvm
cd obj-llvm
CC=wllvm CFLAGS="-g -O1 -Xclang -disable-llvm-passes -D__NO_STRING_INLINES -D_FORTIFY_SOURCE=0 -U__OPTIMIZE__" ../configure
make
mkdir harness
cd harness
wllvm -g -O1 -o expat -L../lib ../lib/*.o -lexpat -lz -lm ../../examples/elements.c
extract-bc expat
cd ../..

mkdir obj-ubsan
cd obj-ubsan
CC=wllvm CFLAGS="-g -O1 -Xclang -disable-llvm-passes -D__NO_STRING_INLINES -D_FORTIFY_SOURCE=0 -U__OPTIMIZE__ -fsanitize=signed-integer-overflow -fsanitize=unsigned-integer-overflow -fsanitize=shift -fsanitize=bounds -fsanitize=pointer-overflow -fsanitize=null" ../configure
make
mkdir harness
cd harness
wllvm -g -O1 -o expat -L../lib ../lib/*.o -lexpat -lz -lm -fsanitize=signed-integer-overflow -fsanitize=unsigned-integer-overflow -fsanitize=shift -fsanitize=bounds -fsanitize=pointer-overflow -fsanitize=null ../../examples/elements.c
extract-bc expat
cd ../..


########################## libxml2 ##########################
wget https://codeload.github.com/GNOME/libxml2/tar.gz/refs/tags/v2.10.3
tar -xf v2.10.3
mv v2.10/libxml2-2.10.3 .
rm -r v2.10
cd libxml2-2.10.3
./autogen.sh
make distclean

mkdir obj-llvm
cd obj-llvm
CC=wllvm CFLAGS="-g -O1 -Xclang -disable-llvm-passes -D__NO_STRING_INLINES -D_FORTIFY_SOURCE=0 -U__OPTIMIZE__" ../configure --without-python --with-threads=no --with-zlib=no --with-lzma=no 
make
mkdir harness
cd harness
wllvm -g -O1 -o xmlparse -I/usr/include/libxml2/ -L../.libs ../.libs/*.o -lxml2 -lz -lm -ldl xmlparse.c
extract-bc xmlparse
cd ../..

mkdir obj-ubsan
cd obj-ubsan
CC=wllvm CFLAGS="-g -O1 -Xclang -disable-llvm-passes -D__NO_STRING_INLINES -D_FORTIFY_SOURCE=0 -U__OPTIMIZE__ -fsanitize=signed-integer-overflow -fsanitize=unsigned-integer-overflow -fsanitize=shift -fsanitize=bounds -fsanitize=pointer-overflow -fsanitize=null" ../configure --without-python --with-threads=no --with-zlib=no --with-lzma=no 
make
mkdir harness
cd harness
wllvm -g -O1 -o xmlparse -I/usr/include/libxml2/ -L../.libs ../.libs/*.o -lxml2 -lz -lm -ldl -fsanitize=signed-integer-overflow -fsanitize=unsigned-integer-overflow -fsanitize=shift -fsanitize=bounds -fsanitize=pointer-overflow -fsanitize=null xmlparse.c
extract-bc xmlparse
cd ../..
