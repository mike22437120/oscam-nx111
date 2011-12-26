#!/bin/sh

plat=i686-pc-cygwin
plat_dir=build_cygwin

rm -rf oscam  CMake* *.a Makefile cscrypt csctapi algo a.exe CopyOfCMakeCache.txt oscam-${plat}-*.tar.gz *.cmake
export OLDPATH=$PATH
if ! echo $PATH | grep  i686-pc-cygwin >/dev/null; then 
	export PATH=../../toolchains/i686-pc-cygwin/bin:$PATH     # 指定编译源码时要用的环境下的GCC和C++编译器路径
fi
make clean
export CMAKE_LEGACY_CYGWIN_WIN32=0
sed  "s:.*(CMAKE_RC_COMPILER.*::g" ../toolchains/toolchain-i386-cygwin.cmake > toolchain-i386-cygwin.cmake
windres=`which i686-pc-cygwin-windres`
echo "SET (CMAKE_RC_COMPILER `pwd`/$windres)" >>toolchain-i386-cygwin.cmake
cmake -DCMAKE_TOOLCHAIN_FILE=toolchain-i386-cygwin.cmake  ..    #用cmake命令对源码进行交叉编译
make
export CMAKE_LEGACY_CYGWIN_WIN32=
export PATH=$OLDPATH
cp oscam.exe image/oscam.exe

curdir=`pwd`
builddir=`dirname $0`
[ "$builddir" = "." ] && svnroot=".."
[ "$builddir" = "." ] || svnroot=`dirname $builddir`
csver=`grep "CS_VERSION" $svnroot/globals.h | sed -e "s/[^\"]*//" -e "s/\"//g" | cut -f1 -d-`
svnver=`svnversion  -c ${svnroot} | cut -f 2 -d: | sed -e "s/[^[:digit:]]//g"`
cd  ${svnroot}/${plat_dir}/image
tar czf ${curdir}/oscam-${plat}-svn${svnver}-nx111-`date +%Y%m%d`.tar.gz cygwin1.dll oscam.exe oscam.conf oscam.server.default
cd $curdir

rm -rf CMake* *.a Makefile cscrypt csctapi *.cmake algo a.exe
