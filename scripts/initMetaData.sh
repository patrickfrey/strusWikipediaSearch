#!/bin/sh

# This script assumes that the meta data table schema is initialized as
# doclen UInt16
# doclen_tist UInt8
# contribid UInt32
#

# Initialize the document title length attribute (for weighting schema BM15 on title)
strusInspect -s "path=storage.old" ttc tist > resources/metadata_tist_doclen.txt
strusUpdateStorage -s "path=storage.old" -m doclen_tist resources/metadata_tist_doclen.txt

# Initialize the link popularity weight document meta data
strusInspect -s "path=storage.old" fwstats linkid > resources/linkid_list.txt
strusInspect -s "path=storage.old" attribute title > resources/docid_list.txt

