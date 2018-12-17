#!/bin/sh
# Prerequisites:
#	Strus projects installed:
#		strusBase
#		strus strusAnalyzer
#		strusTrace strusModule strusRpc
#		strusPattern strusVector
#		strusUtilities strusBindings
#		strusWebService strusWikipediaSearch
#	SyntaxNet:
#		cuda 9.0 (https://yangcha.github.io/CUDA90)
#	Tensorflow (for SyntaxNet):
#		https://github.com/tensorflow/tensorflow
#	Bazel (for Tensorflow):
#		https://docs.bazel.build/versions/master/install-ubuntu.html
#	Python:
#		numpy

export PYTHONHASHSEED=123
SCRIPT=$(readlink -f "$0")
SCRIPTPATH=$(dirname "$SCRIPT")

mkdir /srv/wikipedia

cd /srv/wikipedia/
wget http://dumps.wikimedia.your.org/enwiki/latest/enwiki-latest-pages-articles.xml.bz2
bunzip2 enwiki-latest-pages-articles.xml.bz2

mkdir -p xml
mkdir -p nlpxml
strusWikimediaToXml -n 0 -P 10000 -R ./redirects.txt enwiki-latest-pages-articles.xml
strusWikimediaToXml -I -B -n 0 -P 10000 -t 12 -L ./redirects.txt enwiki-latest-pages-articles.xml xml

for ext in err mis wtf org txt; do find xml -name "*.$ext" | xargs rm; done

processPosTagging() {
    DID=$1
    NLPCONV=$SCRIPTPATH/strusnlp.py
    PYTHONHASHSEED=123
    # [1] Call a strus program to scan the Strus Wikipedia XML generated in the previous step from the Wikimedia dump.
    #	the program creates a text dump in /srv/wikipedia/pos/$DID.txt with all the selected contents as input for the
    #	POS tagging script.
    strusPosTagger -I -x xml -C XML -D '; ' -X '//pagelink@id' -Y '##' -e '//pagelink()' -e '//weblink()' -e '//text()' -e '//attr()' -e '//char()' -e '//math()' -e '//code()' -e '//bibref()' -E '//mark' -E '//text' -E '//entity' -E '//attr' -E '//attr~' -E '//quot' -E '//quot~' -E '//pagelink' -E '//weblink' -E '//tablink' -E '//citlink' -E '//reflink' -E '//tabtitle' -E '//head' -E '//cell' -E '//bibref' -E '//time' -E '//char' -E '//code' -E '//math' -p '//heading' -p '//table' -p '//citation' -p '//ref' -p '//list' -p '//cell~' -p '//head~' -p '//heading~' -p '//list~' -p '//br' /srv/wikipedia/xml/$DID /srv/wikipedia/pos/$DID.txt
    EC="$?"
    if [ "$EC" != "0" ]; then
        echo "Error creating POS tagger input: $EC" > /srv/wikipedia/err/$DID.txt
    fi
    # [2] Call the POS tagging script with the text dumps in /srv/wikipedia/pos/$DID.txt and write the output to /srv/wikipedia/tag/$DID,txt
    cat /srv/wikipedia/pos/$DID.txt | $NLPCONV -S -C 100 > /srv/wikipedia/tag/$DID.txt
    EC="$?"
    if [ "$EC" != "0" ]; then
        echo "Error in POS tagger script: $EC" > /srv/wikipedia/err/$DID.txt
    fi
    # [3] Merge the output of the POS tagging script with the original XML in /srv/wikipedia/xml/$DID/
    #	and write a new XML file with the same name into /srv/wikipedia/nlpxml/$DID/
    strusPosTagger -x ".xml" -C XML -e '//pagelink()' -e '//weblink()' -e '//text()' -e '//attr()' -e '//char()' -e '//math()' -e '//code()' -e '//bibref()' -o /srv/wikipedia/nlpxml/$DID /srv/wikipedia/xml/$DID /srv/wikipedia/tag/$DID.txt
    EC="$?"
    if [ "$EC" != "0" ]; then
        echo "Error tagging XML with POS tagger output: $EC" > /srv/wikipedia/err/$DID.txt
    fi
    # [4] Cleanup temporary files
    rm /srv/wikipedia/pos/$DID.txt
    rm /srv/wikipedia/tag/$DID.txt
}

processPosTaggingDumpSlice() {
    WHAT=$1
    SLICE=$2
    START=${3:-0000}
    END=${4:-9999}
    for aa in 0 1 2 3 4 5 6 ; do
    for bb in 0 1 2 3 4 5 6 7 8 9; do
    for cc in 0 1 2 3 4 5 6 7 8 9; do
    for dd in 0 1 2 3 4 5 6 7 8 9; do
        DID=$aa$bb$cc$dd
        if [ $DID -ge $START ]; then
            if [ $DID -le $END ]; then
                if [ `expr $DID % $SLICE` == $WHAT ]; then
                    echo "processing $DID ..."
                    processPosTagging $DID
                fi
            fi
        fi
    done
    done
    done
    done
}

processPosTaggingDumpSlice 0 3 0000 &
processPosTaggingDumpSlice 1 3 0000 &
processPosTaggingDumpSlice 2 3 0000 &





