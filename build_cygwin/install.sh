#!/bin/sh

plat=i686-pc-cygwin
plat_dir=build_cygwin

if [ `dirname $0` != "." ]; then 
	echo "Must run it in current dir!" 
	exit -1
fi

${TOOLCHAIN_ROOT:=`pwd`/../../toolchains} 2>/dev/null

rm -rf oscam  CMake* *.a Makefile cscrypt csctapi algo a.exe CopyOfCMakeCache.txt oscam-${plat}-*.tar.gz *.cmake
make clean
export CMAKE_LEGACY_CYGWIN_WIN32=0
sed  "s:.*(CMAKE_RC_COMPILER.*::g" ../toolchains/toolchain-i386-cygwin.cmake > toolchain-i386-cygwin.cmake
PATH=$TOOLCHAIN_ROOT/i686-pc-cygwin/bin:$PATH windres=`which i686-pc-cygwin-windres`
echo "SET (CMAKE_RC_COMPILER $windres)" >>toolchain-i386-cygwin.cmake
PATH=$TOOLCHAIN_ROOT/i686-pc-cygwin/bin:$PATH \
cmake 	-DCMAKE_TOOLCHAIN_FILE=toolchain-i386-cygwin.cmake \
      	-DLIBUSBDIR=$TOOLCHAIN_ROOT/i686-pc-cygwin/i686-pc-cygwin \
	-DLIBRTDIR=$TOOLCHAIN_ROOT/i686-pc-cygwin/i686-pc-cygwin \
	-DOPTIONAL_INCLUDE_DIR=$TOOLCHAIN_ROOT/i686-pc-cygwin/i686-pc-cygwin/include \
      	..
make
export CMAKE_LEGACY_CYGWIN_WIN32=
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

rm -rf CMake* *.a Makefile cscrypt csctapi *.cmake algo a.exe image/oscam.exe utils
