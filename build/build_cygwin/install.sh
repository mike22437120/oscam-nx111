#!/bin/sh

plat=i686-pc-cygwin
plat_dir=`basename \`dirname $0\``

curdir=`pwd`
cd `dirname $0`
builddir=`pwd`
outdir=`dirname $builddir`
if [ $# -gt 0 ]; then 
	if [ .$[1:0:1] = .\/ ]; then 
		outdir=$1;
	elif [ -d $curdir/$1 ]; then
		outdir=$curdir/$1
	fi
fi
srcdir=`dirname \`dirname \\\`pwd\\\`\``
${TOOLCHAIN_ROOT:=$srcdir/../toolchains} 2>/dev/null

rm -rf oscam  CMake* *.a Makefile cscrypt csctapi algo a.exe CopyOfCMakeCache.txt $outdir/oscam-${plat}-*.tar.gz oscam-${plat}-*.tar.gz *.cmake

make clean
export CMAKE_LEGACY_CYGWIN_WIN32=0
sed  "s:.*(CMAKE_RC_COMPILER.*::g" $srcdir/toolchains/toolchain-i386-cygwin.cmake > toolchain-i386-cygwin.cmake
PATH=$TOOLCHAIN_ROOT/i686-pc-cygwin/bin:$PATH windres=`which i686-pc-cygwin-windres`
echo "SET (CMAKE_RC_COMPILER $windres)" >>toolchain-i386-cygwin.cmake
PATH=$TOOLCHAIN_ROOT/i686-pc-cygwin/bin:$PATH \
cmake 	-DCMAKE_TOOLCHAIN_FILE=toolchain-i386-cygwin.cmake \
      	-DLIBUSBDIR=$TOOLCHAIN_ROOT/i686-pc-cygwin/i686-pc-cygwin \
	-DLIBRTDIR=$TOOLCHAIN_ROOT/i686-pc-cygwin/i686-pc-cygwin \
	-DOPTIONAL_INCLUDE_DIR=$TOOLCHAIN_ROOT/i686-pc-cygwin/i686-pc-cygwin/include \
	-DSTATIC_LIBUSB=1 \
      	$srcdir
make
export CMAKE_LEGACY_CYGWIN_WIN32=
cp oscam.exe image/oscam.exe

svnroot=$srcdir
csver=`grep "CS_VERSION" $svnroot/globals.h | sed -e "s/[^\"]*//" -e "s/\"//g" | cut -f1 -d-`
svnver=`(grep "CS_SVN_VERSION .*" $svnroot/.revision 2>/dev/null || (echo CS_SVN_VERSION "0">$svnroot/.revision;printf 0)) | cut -d" " -f2 | sed 's/[^0-9]*//;s/[^0-9]*$//'`
cd  $builddir/image && tar czf ${outdir}/oscam-${plat}-svn${svnver}-nx111-`date +%Y%m%d`.tar.gz cygwin1.dll oscam.exe oscam.conf oscam.server.default
cd $builddir && rm -rf CMake* *.a Makefile cscrypt csctapi *.cmake algo a.exe image/oscam.exe utils
cd $curdir
[ -f $builddir/oscam.exe ] || exit 1
echo "Compiled oscam files will be found in: $outdir"
