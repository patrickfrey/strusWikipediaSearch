#!/bin/sh

# This script assumes that the meta data table schema is initialized as
# doclen UInt16
# doclen_tist UInt8
# contribid UInt32
# pageweight Float32
#

echo "Initializing the document title length attribute ..."
# [1] Initialize the document title length attribute (for weighting schema BM15 on title):
# [1.1] create a map docno -> length of title (number of terms)
strusInspect -s "path=/data/wikipedia/storage1" ttc tist >  resources/metadata_tist_doclen.txt
strusInspect -s "path=/data/wikipedia/storage2" ttc tist >> resources/metadata_tist_doclen.txt
# [1.2] update the meta data table element doclen_tist with the title lengths calculated
strusUpdateStorage -s "path=/data/wikipedia/storage1" -m doclen_tist resources/metadata_tist_doclen.txt
strusUpdateStorage -s "path=/data/wikipedia/storage2" -m doclen_tist resources/metadata_tist_doclen.txt
echo "... done"


echo "Initializing the link popularity weight in meta data ..."
# [2] Initialize the link popularity weight in document meta data (element pageweight):
# [2.1] get the link reference statistics
strusInspect -s "path=/data/wikipedia/storage1" fwstats linkid > resources/linkid_list1.txt
strusInspect -s "path=/data/wikipedia/storage2" fwstats linkid > resources/linkid_list2.txt
scripts/mergeWeights.pl resources/linkid_list1.txt resources/linkid_list2.txt > resources/linkid_list.txt
for ii in 1 2; do
echo "... in storage $ii"
# [2.2] get the docno -> docid map
strusInspect -s "path=/data/wikipedia/storage$ii" attribute title > resources/docid_list$ii.txt
# [2.3] calculate a map docno -> number of references to this page
scripts/calcDocidRefs.pl resources/docid_list$ii.txt resources/linkid_list.txt > resources/docnoref_map$ii.txt
# [2.4] calculate a map docno -> link popularity weight
scripts/calcWeights.pl resources/docnoref_map$ii.txt 'tanh(x/50)' > resources/pageweight_map$ii.txt
# [2.5] update the meta data table element pageweight with the link popularity weight
strusUpdateStorage -s "path=/data/wikipedia/storage$ii" -m pageweight resources/pageweight_map$ii.txt
done
echo "... done"


