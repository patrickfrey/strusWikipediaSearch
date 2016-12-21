#!/bin/sh

tarfile=$1
jobid=$2
prefix=$3
outputfile=$4
titlefile=$5
srcprefix=$6

if [ x$prefix = 'x' ]
then
	prefix=tmp
fi
if [ x$outputfile = 'x' ]
then
	outputfile=docs.dump.$jobid.txt
fi
if [ x$titlefile = 'x' ]
then
	titlefile=title.$jobid.txt
fi

mkdir -p $prefix$jobid
tar -C $prefix$jobid/ -xvzf $1
for ff in `find $prefix$jobid/ -name "*.xml" | sort`
do
echo "analyze $ff"
strusAnalyze -M /usr/local/lib/strus/modules -m analyzer_wikipedia_search -R "$srcprefix"resources/ -D "orig,sent=' .\n',para=' .\n',start=' .\n'" "$srcprefix"config/wikipedia.ana $ff | iconv -c -f utf-8 -t utf-8 - | "$srcprefix"scripts/nlpclean.sh | perl -C -pe 's/[~_=\/\#-\+\-]/ /g' | perl -C -pe 's/[\<\>()\|\[\]\{\}]/ \,/g' | perl -C -pe 's/[,]+\./\./g' | perl -C -pe 's/([\.])(^[0-9])/ \1 \2/g' | perl -C -pe 's/([\;\,\!\?\:])/ \1 /g' | perl -C -pe 's/([\\])/ /g' >> $outputfile
strusAnalyze -M /usr/local/lib/strus/modules -m analyzer_wikipedia_search -R "$srcprefix"resources/ -D "tiword,start='\n'" "$srcprefix"config/wikipedia.ana $ff >> $titlefile
done

rm -Rf $prefix$jobid/

