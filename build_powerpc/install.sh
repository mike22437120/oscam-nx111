#!/bin/sh

plat=powerpc
plat_dir=build_powerpc

if [ `dirname $0` != "." ]; then 
	echo "Must run it in current dir!" 
	exit -1
fi

${TOOLCHAIN_ROOT:=`pwd`/../../toolchains} 2>/dev/null

rm -f oscam oscam-nx111  oscam-$plat-svn*.tar.gz oscam-$plat-svn*.ipk

make clean
PATH=$TOOLCHAIN_ROOT/powerpc-tuxbox-linux-gnu/bin:$PATH \
cmake 	-DCMAKE_TOOLCHAIN_FILE=../toolchains/toolchain-powerpc-tuxbox.cmake \
      	-DLIBUSBDIR=$TOOLCHAIN_ROOT/powerpc-tuxbox-linux-gnu/powerpc-tuxbox-linux-gnu \
      	-DLIBRTDIR=$TOOLCHAIN_ROOT/powerpc-tuxbox-linux-gnu/powerpc-tuxbox-linux-gnu \
	-DOPTIONAL_INCLUDE_DIR=$TOOLCHAIN_ROOT/powerpc-tuxbox-linux-gnu/powerpc-tuxbox-linux-gnu/include \
      	..    
make

[ -d image/var/bin ] || mkdir -p image/var/bin
cp oscam image/var/bin/

curdir=`pwd`
builddir=`dirname $0`
[ "$builddir" = "." ] && svnroot=".."
[ "$builddir" = "." ] || svnroot=`dirname $builddir`
csver=`grep "CS_VERSION" $svnroot/globals.h | sed -e "s/[^\"]*//" -e "s/\"//g" | cut -f1 -d-`
svnver=`svnversion  -c ${svnroot} | cut -f 2 -d: | sed -e "s/[^[:digit:]]//g"`
cd ${svnroot}/${plat_dir}/image
sed -i "s/Version:.*/Version: ${csver}-svn${svnver}/" DEBIAN/control
tar czf ../oscam-${plat}-svn${svnver}-nx111-`date +%Y%m%d`.tar.gz var
cd ../ 
dpkg -b image oscam-${plat}-svn${svnver}-nx111-`date +%Y%m%d`.ipk
rm -rf CMake* *.a Makefile cscrypt csctapi *.cmake algo image/var/bin/oscam utils
cd $curdir
