#!/bin/sh

# This script assumes that the meta data table schema is initialized as
# doclen UInt16
# doclen_tist UInt8
# contribid UInt32
# pageweight Float32
#

STORAGEPATH="/data/storage_xs09"

# [1] Initialize the document title length attribute (for weighting schema BM15 on title):
echo "[1.1] create a map docno -> length of document (number of stems)"
strusInspect -s "path=$STORAGEPATH" ttc stem > resources/metadata_doclen.txt
echo "[1.1] create a map docno -> length of title (number of title terms)"
strusInspect -s "path=$STORAGEPATH" ttc tist > resources/metadata_tist_doclen.txt
echo "[1.2] update the meta data table element doclen with the document lengths calculated"
strusUpdateStorage -s "path=$STORAGEPATH" -m doclen resources/metadata_doclen.txt
echo "[1.2] update the meta data table element doclen_tist with the title lengths calculated"
strusUpdateStorage -s "path=$STORAGEPATH" -m doclen_tist resources/metadata_tist_doclen.txt


# [2] Initialize the link popularity weight in document meta data (element pageweight):
echo "[2.1] get the link reference statistics"
strusInspect -s "path=$STORAGEPATH" fwstats linkid > resources/linkid_list.txt
echo "[2.2] get the docno -> docid map"
strusInspect -s "path=$STORAGEPATH" attribute title > resources/docid_list.txt
echo "[2.3] calculate a map docno -> number of references to this page"
scripts/calcDocidRefs.pl resources/docid_list.txt resources/linkid_list.txt > resources/docnoref_map.txt
echo "[2.4] calculate a map docno -> link popularity weight"
scripts/calcWeights.pl resources/docnoref_map.txt 'tanh(x/50)' > resources/pageweight_map.txt
echo "[2.5] update the meta data table element pageweight with the link popularity weight"
strusUpdateStorage -s "path=$STORAGEPATH" -m pageweight resources/pageweight_map.txt

