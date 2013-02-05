#!/bin/sh

plat=powerpc
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

rm -f oscam oscam-nx111  $outdir/oscam-$plat-svn*.* oscam-$plat-svn*.*

make clean
PATH=$TOOLCHAIN_ROOT/powerpc-tuxbox-linux-gnu/bin:$PATH \
cmake 	-DCMAKE_TOOLCHAIN_FILE=$srcdir/toolchains/toolchain-powerpc-tuxbox.cmake \
      	-DLIBUSBDIR=$TOOLCHAIN_ROOT/powerpc-tuxbox-linux-gnu/powerpc-tuxbox-linux-gnu \
      	-DLIBRTDIR=$TOOLCHAIN_ROOT/powerpc-tuxbox-linux-gnu/powerpc-tuxbox-linux-gnu \
	-DOPTIONAL_INCLUDE_DIR=$TOOLCHAIN_ROOT/powerpc-tuxbox-linux-gnu/powerpc-tuxbox-linux-gnu/include \
      	$srcdir    
make

[ -d image/var/bin ] || mkdir -p image/var/bin
cp oscam image/var/bin/

svnroot=$srcdir
csver=`grep "CS_VERSION" $svnroot/globals.h | sed -e "s/[^\"]*//" -e "s/\"//g" | cut -f1 -d-`
svnver=`(grep "CS_SVN_VERSION .*" $svnroot/.revision 2>/dev/null || (echo CS_SVN_VERSION "0">$svnroot/.revision;printf 0)) | cut -d" " -f2 | sed 's/[^0-9]*//;s/[^0-9]*$//'`
cd $builddir/image
sed -i "s/Version:.*/Version: ${csver}-svn${svnver}/" DEBIAN/control
tar czf $outdir/oscam-${plat}-svn${svnver}-nx111-`date +%Y%m%d`.tar.gz var
cd $builddir 
dpkg -b image $outdir/oscam-${plat}-svn${svnver}-nx111-`date +%Y%m%d`.ipk
rm -rf CMake* *.a Makefile cscrypt csctapi *.cmake algo image/var/bin/oscam utils
cd $curdir
[ -f $builddir/oscam ] || exit 1
echo "Compiled oscam files will be found in: $outdir"

