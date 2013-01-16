#!/bin/sh
plat=i686-pc-linux
plat_dir=build_i686Linux
if [ `dirname $0` != "." ]; then 
	echo "Must run it in current dir!" 
	exit -1
fi

rm -f oscam oscam-$plat-svn*.tar.gz oscam-$plat-svn*.deb

${TOOLCHAIN_ROOT:=`pwd`/../../toolchains} 2>/dev/null

make clean
echo "TOOLCHAIN_ROOT:$TOOLCHAIN_ROOT"
if [ -d $TOOLCHAIN_ROOT/i686-pc-linux-gnu/bin ]; then
	PATH=$TOOLCHAIN_ROOT/i686-pc-linux-gnu/bin:$PATH \
	cmake	-DCMAKE_TOOLCHAIN_FILE=../toolchains/toolchain-i686-pc-linux.cmake \
		-DLIBUSBDIR=$TOOLCHAIN_ROOT/i686-pc-linux-gnu/i686-pc-linux-gnu/sysroot/usr \
		-DLIBRTDIR=$TOOLCHAIN_ROOT/i686-pc-linux-gnu/i686-pc-linux-gnu/sysroot/usr \
		-DOPTIONAL_INCLUDE_DIR=$TOOLCHAIN_ROOT/i686-pc-linux-gnu/i686-pc-linux-gnu/sysroot/usr/include \
		..   
else
	cmake	..
fi

make

[ -d image/usr/bin ] || mkdir -p image/usr/bin
cp oscam image/usr/bin/

curdir=`pwd`
builddir=`dirname $0`
[ "$builddir" = "." ] && svnroot=".."
[ "$builddir" = "." ] || svnroot=`dirname $builddir`
csver=`grep "CS_VERSION" $svnroot/globals.h | sed -e "s/[^\"]*//" -e "s/\"//g" | cut -f1 -d-`
svnver=`(grep "CS_SVN_VERSION .*" $svnroot/.revision 2>/dev/null || (echo CS_SVN_VERSION "0">.revision;printf 0)) | cut -d" " -f2 | sed 's/[^0-9]*//;s/[^0-9]*$//'`
cd ${svnroot}/${plat_dir}/image
sed -i "s/Version:.*/Version: ${csver}-svn${svnver}/" DEBIAN/control
tar czf ../oscam-${plat}-svn${svnver}-nx111-`date +%Y%m%d`.tar.gz usr
cd ../ 
dpkg -b image oscam-${plat}-svn${svnver}-nx111-`date +%Y%m%d`.deb
rm -rf CMake* *.a Makefile cscrypt csctapi *.cmake algo image/usr/bin/oscam utils
cd $curdir
