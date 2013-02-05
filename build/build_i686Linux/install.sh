#!/bin/sh
plat=i686-pc-linux
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

rm -f oscam $outdir/oscam-$plat-svn*.* oscam-$plat-svn*.*

make clean
echo "TOOLCHAIN_ROOT:$TOOLCHAIN_ROOT"
if [ -d $TOOLCHAIN_ROOT/i686-pc-linux-gnu/bin ]; then
	PATH=$TOOLCHAIN_ROOT/i686-pc-linux-gnu/bin:$PATH \
	cmake	-DCMAKE_TOOLCHAIN_FILE=$srcdir/toolchains/toolchain-i686-pc-linux.cmake \
		-DLIBUSBDIR=$TOOLCHAIN_ROOT/i686-pc-linux-gnu/i686-pc-linux-gnu/sysroot/usr \
		-DLIBRTDIR=$TOOLCHAIN_ROOT/i686-pc-linux-gnu/i686-pc-linux-gnu/sysroot/usr \
		-DOPTIONAL_INCLUDE_DIR=$TOOLCHAIN_ROOT/i686-pc-linux-gnu/i686-pc-linux-gnu/sysroot/usr/include \
		$srcdir   
else
	cmake	$srcdir
fi

make

[ -d image/usr/bin ] || mkdir -p image/usr/bin
cp oscam image/usr/bin/

svnroot=$srcdir
csver=`grep "CS_VERSION" $svnroot/globals.h | sed -e "s/[^\"]*//" -e "s/\"//g" | cut -f1 -d-`
svnver=`(grep "CS_SVN_VERSION .*" $svnroot/.revision 2>/dev/null || (echo CS_SVN_VERSION "0">$svnroot/.revision;printf 0)) | cut -d" " -f2 | sed 's/[^0-9]*//;s/[^0-9]*$//'`
cd $builddir/image
sed -i "s/Version:.*/Version: ${csver}-svn${svnver}/" DEBIAN/control
tar czf $outdir/oscam-${plat}-svn${svnver}-nx111-`date +%Y%m%d`.tar.gz usr
cd $builddir
dpkg -b image $outdir/oscam-${plat}-svn${svnver}-nx111-`date +%Y%m%d`.deb
rm -rf CMake* *.a Makefile cscrypt csctapi *.cmake algo image/usr/bin/oscam utils
cd $curdir
[ -f $builddir/oscam ] || exit 1
echo "Compiled oscam files will be found in: $outdir"

