#!/bin/sh

outdir=tmp
vsmdir=strusrepos

run() {
	residx=$1
	resfileBase="$outdir/baseline$residx.txt"
	resfileSearch="$outdir/search$residx.txt"
	nofFeatures=`strusInspectVectorStorage -s "path=$vsmdir/vsm" noffeat`
	feat=`shuf -i 0-$nofFeatures -n 1`
	featname=`strusInspectVectorStorage -s "path=$vsmdir/vsm" featname $feat`
	echo "Find $featname"
	echo "Find $featname" > $resfileBase
	echo "Find $featname" > $resfileSearch
	strusInspectVectorStorage -t 20 -N 100 -x -s "path=$vsmdir/vsm" opfeatwname \%$feat > $resfileBase
	strusInspectVectorStorage -t 20 -N 20     -s "path=$vsmdir/vsm" opfeatwname \%$feat > $resfileSearch
}

for ii in $(seq 1 100); do echo `run $ii`; done

