#!/bin/sh

tarfile=$1
jobid=$2
prefix=$3
outputfile=$4
sourcedir=$5

if [ x$prefix = 'x' ]
then
	prefix=tmp
fi
if [ x$outputfile = 'x' ]
then
	outputfile=docs.dump.$jobid.txt
fi

mkdir -p $prefix$jobid
tar -C $prefix$jobid/ -xvzf $1
for ff in `find $prefix$jobid/ -name "*.xml" | sort`
do
echo "analyze $ff"
strusAnalyze -M /usr/local/lib/strus/modules -m analyzer_wikipedia_search -R "$sourcedir"resources/ -D "orig,sent=' .\n',para=' .\n',start=' .\n'" "$sourcedir"config/wikipedia.ana $ff | iconv -c -f utf-8 -t utf-8 - | perl -C -pe 's/\&[a-z]*\;/ /g' | perl -C -pe 's/nbsp[\;]/ /g' | perl -C -pe 's/\xe2\xe2/"/g' | perl -C -pe 's/[_=\/\#-\+\-]/ /g' | perl -C -pe 's/[()\[\]\{\}]/\,/g' | perl -C -pe 's/[,]+\./\./g' | sed 's/(\xe2\x80\x93\|\xe2\x80\x94\|\xe2\x80\x98\|\xe2\x80\x99\|\xe2\x85\x9b\|\xe2\x86\x94\|\xe2\x87\x86\|\xe2\x88\x92\|\xe2\x89\xa4\|\xc2\xab\|\xc2\xb1\|\xc2\xb7\|\xc2\xbb\|\xc2\xbc\|\xc2\xbd\|\xc2\xbe\|\xc3\x97)/ /g' | perl -C -pe 's/([\;\,\!\?\:\.])/\1 /g' >> $outputfile
done

rm -Rf $prefix$jobid/

