#!/bin/sh

# This script assumes that the meta data table schema is initialized as
# doclen UInt16
# doclen_tist UInt8
# contribid UInt32
# pageweight Float32
#

echo "Initializing the document title length attribute ..."
strusInspect -s "path=/data/storage_xs03" ttc tist >  resources/metadata_tist_doclen.txt
strusUpdateStorage -s "path=/data/storage_xs03" -m doclen_tist resources/metadata_tist_doclen.txt
echo "... done"

echo "Initializing the link popularity weight in meta data ..."
strusInspect -s "path=/data/storage_xs03" fwstats linkid > resources/linkid_list.txt
echo "... get the document id's"
strusInspect -s "path=/data/storage_xs03" attribute title > resources/docid_list.txt
echo "... calculate a map docno -> number of references to this page"
scripts/calcDocidRefs.pl resources/docid_list.txt resources/linkid_list.txt > resources/docnoref_map.txt
echo "... calculate a map docno -> link popularity weight"
scripts/calcWeights.pl resources/docnoref_map.txt 'tanh(x/50)' > resources/pageweight_map.txt
echo "... update the meta data table element pageweight with the link popularity weight"
strusUpdateStorage -s "path=/data/storage_xs03" -m pageweight resources/pageweight_map.txt
echo "... done"


