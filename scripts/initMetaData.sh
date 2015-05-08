#!/bin/sh

# This script assumes that the meta data table schema is initialized as
# doclen UInt16
# doclen_tist UInt8
# contribid UInt32
#

strusInspect -s "path=storage.old" ttc tist > resources/metadata_tist_doclen.txt


