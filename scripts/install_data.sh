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
export SCRIPT=$(readlink -f "$0")
export SCRIPTPATH=$(dirname "$SCRIPT")
export PROJECTPATH=$(dirname "$SCRIPTPATH")

mkdir /srv/wikipedia

cd /srv/wikipedia/
wget http://dumps.wikimedia.your.org/enwiki/latest/enwiki-latest-pages-articles.xml.bz2
bunzip2 enwiki-latest-pages-articles.xml.bz2

mkdir -p xml
mkdir -p nlpxml
mkdir -p storage

strusWikimediaToXml -n 0 -P 10000 -R ./redirects.txt enwiki-latest-pages-articles.xml
strusWikimediaToXml -I -B -n 0 -P 10000 -t 12 -L ./redirects.txt enwiki-latest-pages-articles.xml xml

for ext in err mis wtf org txt; do find xml -name "*.$ext" | xargs rm; done

processPosTagging() {
    DID=$1
    # mv /srv/wikipedia/nlpxml/$DID /srv/wikipedia/nlpxml/$DID.old
    NLPCONV=$SCRIPTPATH/strusnlp.py
    PYTHONHASHSEED=123
    # [1] Call a strus program to scan the Strus Wikipedia XML generated in the previous step from the Wikimedia dump.
    #	the program creates a text dump in /srv/wikipedia/pos/$DID.txt with all the selected contents as input for the
    #	POS tagging script.
    strusPosTagger -I -x xml -C XML -D '; ' -X '//pagelink@id://pagelink//*()' -Y '##' -e '//pagelink()' -e '//weblink()' -e '//text()' -e '//attr()' -e '//char()' -e '//math()' -e '//code()' -e '//bibref()' -E '//mark' -E '//text' -E '//entity' -E '//attr' -E '//attr~' -E '//quot' -E '//quot~' -E '//pagelink' -E '//weblink' -E '//tablink' -E '//citlink' -E '//reflink' -E '//tabtitle' -E '//head' -E '//cell' -E '//bibref' -E '//time' -E '//char' -E '//code' -E '//math' -p '//heading' -p '//table' -p '//citation' -p '//ref' -p '//list' -p '//cell~' -p '//head~' -p '//heading~' -p '//list~' -p '//br' /srv/wikipedia/xml/$DID /srv/wikipedia/pos/$DID.txt
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
    # rm -Rf /srv/wikipedia/nlpxml/$DID.old
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
                    echo "processing POS tagging of $DID ..."
                    processPosTagging $DID
                fi
            fi
        fi
    done
    done
    done
    done
}

processHeadingTagMarkup() {
    START=${1:-0000}
    END=${2:-9999}
    for aa in 0 1 2 3 4 5 6 ; do
    for bb in 0 1 2 3 4 5 6 7 8 9; do
    for cc in 0 1 2 3 4 5 6 7 8 9; do
    for dd in 0 1 2 3 4 5 6 7 8 9; do
        DID=$aa$bb$cc$dd
        if [ $DID -ge $START ]; then
            if [ $DID -le $END ]; then
                echo "processing title/heading tag markup of $DID ..."
                strusTagMarkup -x xml -e '/doc/title' -e '//heading' -P $DID"_1" /srv/wikipedia/nlpxml/$DID /srv/wikipedia/nlpxml/$DID
            fi
        fi
    done
    done
    done
    done
}

processCategoryTagMarkup() {
    echo "processing category tag markup ..."
    strusTagMarkup -x xml  --markup map --attribute cid -e '//category' -P "1:lc:convdia" /srv/wikipedia/nlpxml /srv/wikipedia/nlpxml
}

processDocumentCheck() {
    START=${1:-0000}
    END=${2:-9999}
    for aa in 0 1 2 3 4 5 6 ; do
    for bb in 0 1 2 3 4 5 6 7 8 9; do
    for cc in 0 1 2 3 4 5 6 7 8 9; do
    for dd in 0 1 2 3 4 5 6 7 8 9; do
        DID=$aa$bb$cc$dd
        if [ $DID -ge $START ]; then
        if [ $DID -le $END ]; then
            echo "checking $DID ..."
            for ff in `ls /srv/wikipedia/nlpxml/$DID/*.xml`; do xmllint --noout $ff; done > /srv/wikipedia/err/xmllint.$DID.xml 2>&1
            sed -e '/parser error [:] Attribute id redefined/,+2d' /srv/wikipedia/err/xmllint.$DID.xml > /srv/wikipedia/err/xmlerr.$DID.xml
            rm /srv/wikipedia/err/xmllint.$DID.xml
            [ -s /srv/wikipedia/err/xmlerr.$DID.xml ] || rm /srv/wikipedia/err/xmlerr.$DID.xml # ... delete empty files
            [ -e /srv/wikipedia/err/xmlerr.$DID.xml ] && echo "$DID has errors, see /srv/wikipedia/err/xmlerr.$DID.xml"
        fi
    fi
    done
    done
    done
    done
}


dumpVectorInput() {
    DID=$1
    CFG=$PROJECTPATH/config/word2vecInput.ana
    FILTER=$SCRIPTPATH/filtervectok.py
    strusAnalyze --dump "eod='\n. . . . . . . .\n',punct=' , ',eos=' .\n',refid,word" --unique -C XML -m normalizer_entityid $CFG /srv/wikipedia/nlpxml/$DID/ | $FILTER >> /srv/wikipedia/vec.txt
}

calcWord2vec() {
    word2vec -size 300 -window 6 -sample 1e-5 -negative 16 -threads 12 -always 'H#' -min-count 5 -alpha 0.025 -classes 0 -debug 1 -binary 1 -portable 1 -save-vocab /srv/wikipedia/vocab.txt -cbow 0 -train /srv/wikipedia/vec.txt -output /srv/wikipedia/vec.bin
}

dumpVectorInputAll() {
    rm /srv/wikipedia/vec.txt
    START=${1:-0000}
    END=${2:-9999}
    for aa in 0 1 2 3 4 5 6 ; do
    for bb in 0 1 2 3 4 5 6 7 8 9; do
    for cc in 0 1 2 3 4 5 6 7 8 9; do
    for dd in 0 1 2 3 4 5 6 7 8 9; do
        DID=$aa$bb$cc$dd
        if [ $DID -ge $START ]; then
            if [ $DID -le $END ]; then
                echo "processing $DID ..."
                dumpVectorInput $DID
            fi
        fi
    done
    done
    done
    done
}

createStorage() {
    STORAGEID=$1
    STORAGEPATH="/srv/wikipedia/storage/$STORAGEID"
    if [ -d "$STORAGEPATH" ]; then
        strusDestroy -s "path=$STORAGEPATH"
    fi
    strusCreate -s "path=$STORAGEPATH; metadata=doclen UINT32"
}

insertDocuments() {
    STORAGEID=$1
    WHAT=$2
    SLICE=$3
    START=${4:-0000}
    END=${5:-9999}
    CFG=$PROJECTPATH/config/doc.ana
    STORAGEPATH="/srv/wikipedia/storage/$STORAGEID"

    for aa in 0 1 2 3 4 5 6 ; do
    for bb in 0 1 2 3 4 5 6 7 8 9; do
    PATHLIST=""
    for cc in 0 1 2 3 4 5 6 7 8 9; do
    for dd in 0 1 2 3 4 5 6 7 8 9; do
        DID=$aa$bb$cc$dd
        if [ $DID -ge $START ]; then
            if [ $DID -le $END ]; then
                if [ `expr $DID % $SLICE` == $WHAT ]; then
                    PATHLIST="$PATHLIST $DID"
                fi
            fi
        fi
    done
    done
    cd /srv/wikipedia/nlpxml
    echo "inserting documents of $PATHLIST ..."
    strusInsert -s "path=$STORAGEPATH" -x xml -C XML -m normalizer_entityid -t 3 -c 1000 $CFG $PATHLIST 
    cd -
    done
    done
}

processPosTaggingDumpSlice 0 3 0000 5762 &
processPosTaggingDumpSlice 1 3 0000 5762 &
processPosTaggingDumpSlice 2 3 0000 5762 &

processHeadingTagMarkup 0000 5762
processCategoryTagMarkup 
dumpVectorInputAll 0000 5762
calcWord2vec

createStorage doc1
createStorage doc2
createStorage doc3
createStorage doc4

insertDocuments doc1 0 4 0000 5762
insertDocuments doc2 1 4 0000 5762
insertDocuments doc3 2 4 0000 5762
insertDocuments doc4 3 4 0000 5762




