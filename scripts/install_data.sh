#!/bin/sh

export SCRIPT=$(readlink -f "$0")
export PYTHONHASHSEED=123
export SCRIPTPATH=$(dirname "$SCRIPT")
export PROJECTPATH=$(dirname "$SCRIPTPATH")
export DATAPATH=/data/wikipedia
export STORAGEPATH=/srv/wikipedia/storage
export MAXNOFHTREADS=14

mkdir -p $DATAPATH
mkdir -p $STORAGEPATH

createData() {
	cd $DATAPATH
	wget http://dumps.wikimedia.your.org/enwiki/latest/enwiki-latest-pages-articles.xml.bz2
	bunzip2 enwiki-latest-pages-articles.xml.bz2
	
	mkdir -p xml
	mkdir -p nlpxml
	mkdir -p storage
	
	strusWikimediaToXml -n 0 -P 10000 -R ./redirects.txt enwiki-latest-pages-articles.xml
	strusWikimediaToXml -B -n 0 -P 10000 -t "$MAXNOFHTREADS" -L ./redirects.txt enwiki-latest-pages-articles.xml xml

	for ext in err mis wtf org txt; do find xml -name "*.$ext" | xargs rm; done
}

mergePosTagging()
{
	cd $DATAPATH
	strusMergeMarkup -x ".xml" -t "$MAXNOFHTREADS" -k "T,P,C,X,A,V,M,N,E,U,R,W" -o nlpxml/ -F errout/ nlpxml.old/ xml/
}

processPosTagging() {
    DID=$1
    NLPCONV=$SCRIPTPATH/strusnlp.py
    PYTHONHASHSEED=123
    # [1] Call a strus program to scan the Strus Wikipedia XML generated in the previous step from the Wikimedia dump.
    #	the program creates a text dump in $DATAPATH/pos/$DID.txt with all the selected contents as input for the
    #	POS tagging script.
    strusPosTagger -I -x xml -C XML -D '; ' -X '//pagelink@id://pagelink//*()' -Y '##' -e '//pagelink()' -e '//weblink()' -e '//text()' -e '//attr()' -e '//char()' -e '//math()' -e '//code()' -e '//bibref()' -E '//mark' -E '//text' -E '//entity' -E '//attr' -E '//attr~' -E '//quot' -E '//quot~' -E '//pagelink' -E '//weblink' -E '//tablink' -E '//citlink' -E '//reflink' -E '//tabtitle' -E '//head' -E '//cell' -E '//bibref' -E '//time' -E '//char' -E '//code' -E '//math' -p '//heading' -p '//table' -p '//citation' -p '//ref' -p '//list' -p '//cell~' -p '//head~' -p '//heading~' -p '//list~' -p '//br' $DATAPATH/xml/$DID $DATAPATH/pos/$DID.txt
    EC="$?"
    if [ "$EC" != "0" ]; then
        echo "Error creating POS tagger input: $EC" > $DATAPATH/err/$DID.txt
    fi
    # [2] Call the POS tagging script with the text dumps in $DATAPATH/pos/$DID.txt and write the output to $DATAPATH/tag/$DID,txt
    cat $DATAPATH/pos/$DID.txt | $NLPCONV -S -C 100 > $DATAPATH/tag/$DID.txt
    EC="$?"
    if [ "$EC" != "0" ]; then
        echo "Error in POS tagger script: $EC" > $DATAPATH/err/$DID.txt
    fi
    # [3] Merge the output of the POS tagging script with the original XML in $DATAPATH/xml/$DID/
    #	and write a new XML file with the same name into $DATAPATH/nlpxml/$DID/
    strusPosTagger -F $DATAPATH/err/tag_$DID.err -x ".xml" -C XML -e '//pagelink()' -e '//weblink()' -e '//text()' -e '//attr()' -e '//char()' -e '//math()' -e '//code()' -e '//bibref()' -o $DATAPATH/nlpxml/$DID $DATAPATH/xml/$DID $DATAPATH/tag/$DID.txt
    EC="$?"
    if [ "$EC" != "0" ]; then
        echo "Error tagging XML with POS tagger output: $EC" > $DATAPATH/err/$DID.txt
    fi
}

processPosTaggingDumpSlice() {
    WHAT=$1
    SLICE=$2
    START=${3:-0000}
    END=${4:-9999}
    LASTJOB=none
    STOPPED=false
    for aa in 0 1 2 3 4 5 6 ; do
    for bb in 0 1 2 3 4 5 6 7 8 9; do
    for cc in 0 1 2 3 4 5 6 7 8 9; do
    for dd in 0 1 2 3 4 5 6 7 8 9; do
        DID=$aa$bb$cc$dd
        if [ $DID -ge $START ]; then
            if [ $DID -le $END ]; then
                if [ `expr $DID % $SLICE` == $WHAT ]; then
                    if [ -e $DATAPATH/flags/stop_nlp ]
                    then
                        if [ "_$STOPPED" == "_false" ]; then
                            echo "stopped POS tagging ($LASTJOB)."
                            STOPPED=true
                        fi
                        break
                    else
                        echo "processing POS tagging of $DID ..."
                        processPosTagging $DID
                        LASTJOB=$DID
                    fi
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
                strusTagMarkup -t "$MAXNOFHTREADS" -x xml -e '/doc/title' -e '//heading' -d '//br' -P $DID"_1" $DATAPATH/nlpxml/$DID $DATAPATH/nlpxml/$DID
            fi
        fi
    done
    done
    done
    done
}

processCategoryTagMarkup() {
    echo "processing category tag markup ..."
    strusTagMarkup -x xml  --markup map --attribute cid -e '//category' -P "1:lc:convdia" $DATAPATH/nlpxml $DATAPATH/nlpxml
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
        if [ -d $DATAPATH/nlpxml ]; then
            echo "checking $DID ..."
            for ff in `ls $DATAPATH/nlpxml/$DID/*.xml`; do xmllint --noout $ff; done > $DATAPATH/err/xmlerr.$DID.xml 2>&1
            [ -s $DATAPATH/err/xmlerr.$DID.xml ] || rm $DATAPATH/err/xmlerr.$DID.xml # ... delete empty files
            [ -e $DATAPATH/err/xmlerr.$DID.xml ] && echo "$DID has errors, see $DATAPATH/err/xmlerr.$DID.xml"
        fi
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
    strusAnalyze --dump "eod='\n. . . . . . . .\n',punct=' , ',eos=' .\n',refid,word" --unique -C XML $CFG $DATAPATH/nlpxml/$DID/ | $FILTER >> $DATAPATH/vec.txt
}

calcWord2vec() {
    word2vec -size 256 -window 8 -sample 1e-5 -negative 16 -threads "$MAXNOFHTREADS" -type-prefix-delim '#' -type-min-count 'H=1,V=100,E=4,N=20,A=100,C=100,X=100,M=100,U=2,R=100,W=100,T=100' -min-count 5 -alpha 0.025 -classes 0 -debug 2 -binary 1 -portable 1 -save-vocab $DATAPATH/vocab.txt -cbow 0 -train $DATAPATH/vec.txt -output $DATAPATH/vec.bin
}

insertVectors() {
    STORAGEID=vec
    if [ -d "$STORAGEPATH/$STORAGEID" ]; then
         strusDestroy -s "path=$STORAGEPATH/$STORAGEID"
    fi
    strusCreateVectorStorage -c 100000 -s "path=$STORAGEPATH/$STORAGEID;vecdim=256;bits=64;variations=32" -P -f $DATAPATH/vec.bin
}

dumpVectorInputAll() {
    rm $DATAPATH/vec.txt
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
    if [ -d "$STORAGEPATH/$STORAGEID" ]; then
        strusDestroy -s "path=$STORAGEPATH/$STORAGEID"
    fi
    strusCreate -s "path=$STORAGEPATH/$STORAGEID"
    strusAlterMetaData -s "path=$STORAGEPATH/$STORAGEID" "add doclen UINT32"
}

insertDocuments() {
    STORAGEID=$1
    WHAT=$2
    SLICE=$3
    START=${4:-0000}
    END=${5:-9999}
    CFG=$PROJECTPATH/config/doc.ana
    LASTJOB=none
    STOPPED=false

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
    if [ -e $DATAPATH/flags/stop_insert ]
    then
        if [ "_$STOPPED" == "_false" ]; then
            echo "stopped inserting documents ($LASTJOB)."
            STOPPED=true
        fi
        break
    else
        cd $DATAPATH/nlpxml
        if [ "_$PATHLIST" != "_" ]; then
            echo "inserting documents of $PATHLIST ..."
            strusInsert -s "path=$STORAGEPATH/$STORAGEID" -x xml -C XML -t 3 -c 5000 $CFG $PATHLIST 
        fi
        LASTJOB=$PATHLIST
        cd -
    fi
    done
    done
}

createData

processPosTaggingDumpSlice 0 3 0000 5762
processPosTaggingDumpSlice 1 3 0000 5762
processPosTaggingDumpSlice 2 3 0000 5762

processHeadingTagMarkup 0000 5762

processDocumentCheck 0000 5762

processCategoryTagMarkup 
dumpVectorInputAll 0000 5762
calcWord2vec
insertVectors

createStorage doc1
insertDocuments doc1 0 4 0000 5762

createStorage doc2
insertDocuments doc2 1 4 0000 5762

createStorage doc3
insertDocuments doc3 2 4 0000 5762

createStorage doc4
insertDocuments doc4 3 4 0000 5762




