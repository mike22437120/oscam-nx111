#!/bin/sh

plat=azbox
plat_dir=build_azbox
rm -f oscam oscam-$plat-svn*.tar.gz
export OLDPATH=$PATH
if ! echo $PATH | grep  mipsel-azbox >/dev/null; then 
	export PATH=../../toolchains/mipsel-azbox/bin:$PATH     # 指定编译源码时要用的azbox mipsel环境下的GCC和C++编译器路径
fi
make clean
cmake -DCMAKE_TOOLCHAIN_FILE=../toolchains/toolchain-mips-azbox.cmake ..    #用cmake命令对源码进行交叉编译
make
export PATH=$OLDPATH

[ -d image/PLUGINS/OpenXCAS/oscamCAS ] || mkdir -p image/PLUGINS/OpenXCAS/oscamCAS
cp oscam image/PLUGINS/OpenXCAS/oscamCAS/

curdir=`pwd`
builddir=`dirname $0`
[ "$builddir" = "." ] && svnroot=".."
[ "$builddir" = "." ] || svnroot=`dirname $builddir`
svnver=`svnversion  -c ${svnroot} | cut -f 2 -d: | sed -e "s/[^[:digit:]]//g"`
cd ${svnroot}/${plat_dir}/image
tar czf ../oscam-${plat}-svn${svnver}-nx111-`date +%Y%m%d`.tar.gz *
cd ../ 
rm -rf CMake* *.a Makefile cscrypt csctapi *.cmake algo image/PLUGINS/OpenXCAS/oscamCAS/oscam
cd $curdir
