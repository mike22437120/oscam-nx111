#!/bin/bash

curdir=`pwd`
cd `dirname $0`
srcdir=`pwd`
builddir="$srcdir/build"
outdir="$srcdir/build/output"
cd $builddir
if [ $# -gt 0 ]; then 
	buildname="$1"
	[ _$1 = "_linux" ] && buildname="i686Linux"
	if [ -f $builddir/build_$buildname/install.sh ]; then 
		$builddir/build_$buildname/install.sh $outdir
	else
		echo "Error:$builddir/build_$1/install.sh Not Exists!!!"
	fi
else
	BUILDDIRS=(`ls -d build_*`)
	echo ${BUILDDIRS[@]}
	for item in ${BUILDDIRS[@]}; do
       		if [ -f $builddir/$item/install.sh -a -d $builddir/$item ]; then
			$item/install.sh $outdir 
			if [ $? -ne 0 ]; then
				cd $curdir
				echo "============Something is wrong when compiling!============="
				exit 1
			fi
       		fi
	done
	echo "===============All Compiled====================="
	echo "Compiled oscam files will be found in: $outdir"
fi
cd $curdir

