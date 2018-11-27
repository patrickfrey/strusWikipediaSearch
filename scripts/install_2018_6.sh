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

export USER=patrick:patrick
export PYTHONHASHSEED=123

sudo mkdir /srv/wikipedia
sudo chown $USER /srv/wikipedia
sudo mkdir /srv/wikipedia/

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
    NLPCONV=/home/patrick/github/strusWikipediaSearch/scripts/strusnlp_spacy.py
    PYTHONHASHSEED=123
    strusPosTagger -I -x xml -C XML -D '; ' -X '//pagelink@id' -Y '##' -e '//pagelink()' -e '//weblink()' -e '//text()' -e '//attr()' -e '//char()' -e '//math()' -e '//code()' -e '//bibref()' -E '//text' -E '//entity' -E '//attr' -E '//attr~' -E '//quot' -E '//quot~' -E '//pagelink' -E '//weblink' -E '//tablink' -E '//citlink' -E '//reflink' -E '//tabtitle' -E '//head' -E '//cell' -E '//bibref' -E '//time' -E '//char' -E '//code' -E '//math' -p '//heading' -p '//table' -p '//citation' -p '//ref' -p '//list' -p '//cell~' -p '//head~' -p '//heading~' -p '//list~' -p '//br' /srv/wikipedia/xml/$DID /srv/wikipedia/pos/$DID.txt
    EC="$?"
    if [ "$EC" != "0" ]; then
        echo "Error creating POS tagger input: $EC" > /srv/wikipedia/err/$DID.txt
    fi
    cat /srv/wikipedia/pos/$DID.txt | $NLPCONV -S -C 100 > /srv/wikipedia/tag/$DID.txt
    EC="$?"
    if [ "$EC" != "0" ]; then
        echo "Error in POS tagger script: $EC" > /srv/wikipedia/err/$DID.txt
    fi
    strusPosTagger -x ".xml" -C XML -e '//pagelink()' -e '//weblink()' -e '//text()' -e '//attr()' -e '//char()' -e '//math()' -e '//code()' -e '//bibref()' -o /srv/wikipedia/nlpxml/$DID /srv/wikipedia/xml/$DID /srv/wikipedia/tag/$DID.txt
    EC="$?"
    if [ "$EC" != "0" ]; then
        echo "Error tagging XML with POS tagger output: $EC" > /srv/wikipedia/err/$DID.txt
    fi
}

processPosTaggingDumpSlice() {
    WHAT=$1
    SLICE=$2
    for aa in 0 1 2 3 4 5 6 ; do
    for bb in 0 1 2 3 4 5 6 7 8 9; do
    for cc in 0 1 2 3 4 5 6 7 8 9; do
    for dd in 0 1 2 3 4 5 6 7 8 9; do
        DID=$aa$bb$cc$dd
        if [ `expr $DID % $SLICE` -eq $WHAT ]; then
            echo "processing $DID ..."
            processPosTagging $DID
        fi
    done
    done
    done
    done
}

processPosTaggingDumpSlice 0 3 &
processPosTaggingDumpSlice 1 3 &
processPosTaggingDumpSlice 2 3


# cat /srv/wikipedia/pos/0000.txt | scripts/strusnlp_spacy.py -S -C 100 > /srv/wikipedia/tag/0000.txt
# strusPosTagger -x ".xml" -C XML -e '//pagelink()' -e '//weblink()' -e '//text()' -e '//attr()' -e '//char()' -e '//math()' -e '//code()' -e '//bibref()' -o /srv/wikipedia/nlpxml/0000 /srv/wikipedia/xml/0000 /srv/wikipedia/tag/0000.txt



