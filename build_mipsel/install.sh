#!/bin/sh

plat=mipsel
plat_dir=build_mipsel

if [ `dirname $0` != "." ]; then 
	echo "Must run it in current dir!" 
	exit -1
fi

rm -f oscam oscam-nx111  oscam-$plat-svn*.tar.gz oscam-$plat-svn*.ipk

${TOOLCHAIN_ROOT:=`pwd`/../../toolchains} 2>/dev/null

make clean
PATH=$TOOLCHAIN_ROOT/mipsel-unknown-linux-gnu/bin:$PATH \
cmake 	-DCMAKE_TOOLCHAIN_FILE=../toolchains/toolchain-mips-tuxbox.cmake \
      	-DLIBUSBDIR=$TOOLCHAIN_ROOT/mipsel-unknown-linux-gnu/mipsel-unknown-linux-gnu/sys-root/usr \
      	-DLIBRTDIR=$TOOLCHAIN_ROOT/mipsel-unknown-linux-gnu/mipsel-unknown-linux-gnu/sys-root/usr \
	-DOPTIONAL_INCLUDE_DIR=$TOOLCHAIN_ROOT/mipsel-unknown-linux-gnu/mipsel-unknown-linux-gnu/sys-root/usr/include \
      	.. 
make
cp oscam oscam-release

[ -d image/usr/bin ] || mkdir -p image/usr/bin
cp oscam image/usr/bin/

curdir=`pwd`
builddir=`dirname $0`
[ "$builddir" = "." ] && svnroot=".."
[ "$builddir" = "." ] || svnroot=`dirname $builddir`
csver=`grep "CS_VERSION" $svnroot/globals.h | sed -e "s/[^\"]*//" -e "s/\"//g" | cut -f1 -d-`
svnver=`(grep "CS_SVN_VERSION .*" $svnroot/.revision 2>/dev/null || (echo CS_SVN_VERSION "0">.revision;printf 0)) | cut -d" " -f2 | sed 's/[^0-9]*//;s/[^0-9]*$//'`
cd ${svnroot}/${plat_dir}/image
sed -i "s/oscam_version=.*/oscam_version=${csver}-svn${svnver}/" etc/init.d/softcam.oscam
sed -i "s/Version:.*/Version: ${csver}-svn${svnver}/" DEBIAN/control
tar czf ../oscam-${plat}-svn${svnver}-nx111-`date +%Y%m%d`.tar.gz usr etc var
cd ../ 
dpkg -b image oscam-${plat}-svn${svnver}-nx111-`date +%Y%m%d`.ipk
rm -rf CMake* *.a Makefile cscrypt csctapi *.cmake algo image/usr/bin/oscam utils
cd $curdir
