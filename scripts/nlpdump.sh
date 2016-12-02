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
strusAnalyze -M /usr/local/lib/strus/modules -m analyzer_wikipedia_search -R "$srcprefix"resources/ -D "orig,sent=' .\n',para=' .\n',start=' .\n'" "$srcprefix"config/wikipedia.ana $ff | iconv -c -f utf-8 -t utf-8 - | "$srcprefix"scripts/nlpclean.sh | perl -C -pe 's/[_=\/\#-\+\-]/ /g' | perl -C -pe 's/[()\[\]\{\}]/\,/g' | perl -C -pe 's/[,]+\./\./g' | perl -C -pe 's/([\;\,\!\?\:\.])/\1 /g' >> $outputfile
strusSegment -e '/mediawiki/page/title()' $ff | sed -E 's/\(.*\)//g' | "$srcprefix"scripts/nlpclean.sh | sed -E 's/[\/\,\:].*//' | sed -E 's/^[0-9]+[ -]+//' | sed -E 's/^[0-9]+[ ]+//' | sed -E 's/[\x27"-]/ /g' | sed -E 's/[ ]+/ /g' | sed -E 's/[ ]+$//' | sed -E 's/[ ]+/_/g' | egrep -v '[0-9]' | sort | uniq >> $titlefile
done

rm -Rf $prefix$jobid/

