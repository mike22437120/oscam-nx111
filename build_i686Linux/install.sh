#!/bin/sh
plat=i686-pc-linux
plat_dir=build_i686Linux
if [ `dirname $0` != "." ]; then 
	echo "Must run it in current dir!" 
	exit -1
fi

rm -f oscam oscam-$plat-svn*.tar.gz oscam-$plat-svn*.deb

TOOLCHAIN_ROOT=`pwd`/../../toolchains
make clean
cmake  -DCS_CONFDIR=/var/etc -DLIBUSBDIR=$TOOLCHAIN_ROOT/i686-pc-linux ..   
make

[ -d image/usr/bin ] || mkdir -p image/usr/bin
cp oscam image/usr/bin/

curdir=`pwd`
builddir=`dirname $0`
[ "$builddir" = "." ] && svnroot=".."
[ "$builddir" = "." ] || svnroot=`dirname $builddir`
csver=`grep "CS_VERSION" $svnroot/globals.h | sed -e "s/[^\"]*//" -e "s/\"//g" | cut -f1 -d-`
svnver=`svnversion  -c ${svnroot} | cut -f 2 -d: | sed -e "s/[^[:digit:]]//g"`
cd ${svnroot}/${plat_dir}/image
sed -i "s/Version:.*/Version: ${csver}-svn${svnver}/" DEBIAN/control
tar czf ../oscam-${plat}-svn${svnver}-nx111-`date +%Y%m%d`.tar.gz var usr
cd ../ 
dpkg -b image oscam-${plat}-svn${svnver}-nx111-`date +%Y%m%d`.deb
rm -rf CMake* *.a Makefile cscrypt csctapi *.cmake algo image/usr/bin/oscam
cd $curdir
