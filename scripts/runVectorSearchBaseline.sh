#!/bin/sh

outdir=tmp
vsmdir=strusrepos

run() {
	residx=$1
	resfileBase="$outdir/baseline$residx.txt"
	resfileSearch="$outdir/search$residx.txt"
	nofFeatures=`strusInspectVectorStorage -s "path=$vsmdir/vsm" nofFeat`
	feat1=`shuf -i 0-$nofFeatures -n 1`
	feat2=`shuf -i 0-$nofFeatures -n 1`
	featname1=`strusInspectVectorStorage -s "path=$vsmdir/vsm" featName $feat1`
	featname2=`strusInspectVectorStorage -s "path=$vsmdir/vsm" featName $feat2`
	echo "Find $featname1 + $featname2"
	echo "Find $featname1 + $featname2" > $resfileBase
	echo "Find $featname1 + $featname2" > $resfileSearch
	echo "strusInspectVectorStorage -N 100 -x -s \"path=$vsmdir/vsm\" opfeatwname \%$feat1 + \%$feat2" > $resfileBase
	echo "strusInspectVectorStorage -N 20     -s \"path=$vsmdir/vsm\" opfeatwname \%$feat1 + \%$feat2" > $resfileSearch
}

for ii in $(seq 1 100); do echo `run $ii`; done
