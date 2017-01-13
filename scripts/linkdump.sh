#!/bin/sh

tarfile=$1
jobid=$2
prefix=$3
outputfile=$4
srcprefix=$5

if [ x$prefix = 'x' ]
then
	prefix=tmp
fi
if [ x$outputfile = 'x' ]
then
	outputfile=title.$jobid.txt
fi

mkdir -p $prefix$jobid
tar -C $prefix$jobid/ -xvzf $1
for ff in `find $prefix$jobid/ -name "*.xml" | sort`
do
strusAnalyze -M /usr/local/lib/strus/modules -m analyzer_wikipedia_search -R "$srcprefix"resources/ -D "doc='\n*',docid,linkid,start=' = ',end=';'" "$srcprefix"config/wikipedia_links.ana $ff | iconv -c -f utf-8 -t utf-8 - > $outputfile
done

rm -Rf $prefix$jobid/

