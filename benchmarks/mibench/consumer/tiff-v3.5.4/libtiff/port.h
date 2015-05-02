/*
 * Warning, this file was automatically created by the TIFF configure script
 * VERSION:	 v3.5.4
 * DATE:	 Sat May 2 17:47:25 EDT 2015
 * TARGET:	 i686-unknown-linux
 * CCOMPILER:	 /usr/bin/gcc-4.6.3-1ubuntu5' --with-bugurl=file:///usr/share/doc/gcc-4.6/README.Bugs --enable-languages=c,c++,fortran,objc,obj-c++ --prefix=/usr --program-suffix=-4.6 --enable-shared --enable-linker-build-id --with-system-zlib --libexecdir=/usr/lib --without-included-gettext --enable-threads=posix --with-gxx-include-dir=/usr/include/c++/4.6 --libdir=/usr/lib --enable-nls --with-sysroot=/ --enable-clocale=gnu --enable-libstdcxx-debug --enable-libstdcxx-time=yes --enable-gnu-unique-object --enable-plugin --enable-objc-gc --enable-targets=all --disable-werror --with-arch-32=i686 --with-tune=generic --enable-checking=release --build=i686-linux-gnu --host=i686-linux-gnu --target=i686-linux-gnu 4.6.3-1ubuntu5)
 */
#ifndef _PORT_
#define _PORT_ 1
#ifdef __cplusplus
extern "C" {
#endif
#include <sys/types.h>
#define HOST_FILLORDER FILLORDER_MSB2LSB
#define HOST_BIGENDIAN	1
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
typedef double dblparam_t;
#ifdef __STRICT_ANSI__
#define	INLINE	__inline__
#else
#define	INLINE	inline
#endif
#define GLOBALDATA(TYPE,NAME)	extern TYPE NAME
#ifdef __cplusplus
}
#endif
#endif
