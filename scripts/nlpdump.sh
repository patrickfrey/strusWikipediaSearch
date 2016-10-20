#!/bin/sh

tarfile=$1
jobid=$2
prefix=$3
if [ x$prefix = 'x' ]
then
	prefix=tmp
fi

mkdir -p $prefix$jobid
tar -C $prefix$jobid/ -xvzf $1
for ff in `ls $prefix$jobid/*.xml`
do
echo "analyze $ff"
strusAnalyze -M /usr/local/lib/strus/modules -m analyzer_wikipedia_search -R resources/ -D "orig,sent=' .\n',para=' .\n',start=' .\n'" config/wikipedia.ana $ff | iconv -c -f utf-8 -t utf-8 - | perl -C -pe 's/\&[a-z]*\;/ /g' | perl -C -pe 's/nbsp[\;]/ /g' | perl -C -pe 's/[—––_=\/;,()\[\]\{\}\"-\-]/ /g' | perl -C -pe 's/([\!\?\:\.])/ \1 /g' | perl -C -pe 's/[ ]+/ /g' | perl -C -pe 's/[ ][.][ ]/ .\n/g'  >> docs.dump.$jobid.txt
done

rm -Rf $prefix$jobid/

