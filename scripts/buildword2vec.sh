#!/bin/sh

#
# PATH SETTINGS
#
docprefix=origdata/
# ... document data directory
resprefix=origdata/
# ... resource directory
blkprefix=strusrepos/
# ... storage repository directory
srcprefix=github/strusWikipediaSearch/
# ... wikipedia project source directory
w2wprefix=github/word2vec/bin/
# ... directory of word2vec program (clone with patch for portable, network order output)
scriptdir="$srcprefix"scripts
# ... scripts directory

#
# COLLECT ALL LINK RELATIONS OF DOCUMENTS
#
runLINKS() {
	jobid=$1
	infile=$2
	dmp_outputfile="$resprefix""links.$infile.txt"

	rm $dmp_outputfile
	$scriptdir/linkdump.sh "$docprefix"wikipedia$infile.tar.gz "$jobid" tmp "$dmp_outputfile" $srcprefix
}

rm "$resprefix""links.all.txt"
for dd in 00 01 02 03 04 05 06 07 08 09 10 11 12 13 14 15 16 17 18 19 20 21 22 23 24 25 26
do
	runLINKS 1 $dd
	cat "$resprefix""links.$dd.txt" >> "$outprefix""links.all.txt"
done

#
# RUN NLP AND CREATE ENTITIES FOR WORD2VEC
#
runNLP() {
	jobid=$1
	infiles=$2
	dmp_outputfile="$resprefix""docs.dump.$jobid.txt"
	nlp_outputfile="$resprefix""docs.nlp.$jobid.txt"
	dic_outputfile="$resprefix""dict.$jobid.txt"
	rm $dmp_outputfile
	rm $nlp_outputfile
	for dd in $infiles; do echo "PROCESS $dd"; $scriptdir/nlpdump.sh "$resprefix"wikipedia$dd.tar.gz $jobid tmp $dmp_outputfile $srcprefix; done
	$scriptdir/strusnlp.py nlp $dmp_outputfile > $nlp_outputfile
	mkdir -p "$resprefix"data
	mv $dmp_outputfile "$resprefix"data/
	$scriptdir/strusnlp.py makedict $nlp_outputfile > $dic_outputfile
}

runNLP 00 "00 01 02 03" &
runNLP 01 "04 05 06 07" &
runNLP 02 "08 09 10 11" &
runNLP 03 "12 13 14 15" &
runNLP 04 "16 17 18 19" &
runNLP 05 "20 21 22 23" &
runNLP 06 "24 25 26" &

$scriptdir/strusnlp.py joindict "$resprefix"dict.{00,01,02,03,04,05,06}.txt > "$resprefix"dict.txt
$scriptdir/strusnlp.py seldict "$resprefix"dict.txt 2 > "$resprefix"dict.sel.txt
$scriptdir/strusnlp.py splitdict "$resprefix"dict.sel.txt "$resprefix"title.txt > "$resprefix"dict.split.txt
mkdir -p "$resprefix"data
rm "$resprefix"dict.sel.txt
mv "$resprefix"dict.{00,01,02,03,04,05,06}.txt "$resprefix"data/

buildText() {
	jobid=$1
	$scriptdir/strusnlp.py concat "$resprefix"docs.nlp.$jobid.txt "$resprefix"dict.split.txt "$resprefix"title.txt > "$resprefix"docs.word2vec.$jobid.txt
}

for dd in 00 01 02 03 04 05 06
do
	buildText $dd
done


cat "$resprefix"docs.word2vec.{00,01,02,03,04,05,06}.txt > "$resprefix"docs.word2vec.txt

#
# RUN WORD2VEC AND INSERT CALCULATED VECTORS INTO A VECTOR STORAGE AND BUILD CONCEPT RELATIONS
#
"$w2wprefix"word2vec -size 300 -window 8 -sample 1e-5 -negative 8 -threads 24 -min-count 4 -alpha 0.025 -classes 0 -debug 1 -binary 1 -portable 1 -save-vocab "$resprefix"vocab.txt -cbow 0 -train "$resprefix"docs.word2vec.txt -output "$resprefix"vectors.bin

strusCreateVectorStorage -S "$srcprefix"config/vsm.conf -f "$resprefix"vectors.bin
strusBuildVectorStorage -S "$srcprefix"config/vsm.conf


# CALCULATE PAGE WEIGHT AND BUILD RULES FOR PATTERN MATCHING
pattern_forwardfeat_doc="$resprefix"pattern_forwardfeat_doc.txt
pattern_searchfeat_doc="$resprefix"pattern_searchfeat_doc.txt
pattern_searchfeat_qry="$resprefix"pattern_searchfeat_qry.txt
pattern_stopwords_doc="$resprefix"dict.stopwords_doc.txt
pattern_stopwords_qry="$resprefix"dict.stopwords_qry.txt
pattern_vocabulary="$resprefix"dict.vocabulary.txt
pattern_lnkfeat_doc="$resprefix"pattern_lnkfeat_doc.txt
pattern_titlefeat_doc="$resprefix"pattern_titlefeat_doc.txt

cat "$resprefix"docs.word2vec.txt | $scriptdir/countTerms.pl > $pattern_vocabulary
$scriptdir/strusnlp.py seldict $pattern_vocabulary 1000000 | grep -v '_' > $pattern_stopwords_qry
$scriptdir/strusnlp.py seldict $pattern_vocabulary 500000 > $pattern_stopwords_doc

echo "" > "$resprefix"redirects_empty.txt
strusPageWeight -i 100 -g -n 100 -r "$resprefix"redirects.txt "$resprefix"links.all.txt > "$resprefix"pagerank.txt

strusInspectVectorStorage -S "$srcprefix"config/vsm.conf featname    | iconv -c -f utf-8 -t utf-8 - | sed -E 's/[\\\>\<\"/]//g' | grep '_' | $scriptdir/createFeatureRules.pl - lexem F '' $pattern_stopwords_doc > $pattern_searchfeat_doc
strusInspectVectorStorage -S "$srcprefix"config/vsm.conf featname    | iconv -c -f utf-8 -t utf-8 - | sed -E 's/[\\\>\<\"/]//g' | grep '_' | $scriptdir/createFeatureRules.pl - lexem name '' $pattern_stopwords_doc > $pattern_forwardfeat_doc
strusInspectVectorStorage -S "$srcprefix"config/vsm.conf featname    | iconv -c -f utf-8 -t utf-8 - | sed -E 's/[\\\>\<\"/]//g' | $scriptdir/createFeatureRules.pl - lexem F lc $pattern_stopwords_qry > $pattern_searchfeat_qry
cat "$resprefix"pagerank.txt    | iconv -c -f utf-8 -t utf-8 - | sed -E 's/[\\\>\<\"/\!\?\:\;\-]/ /g' | $scriptdir/createTitleRules.pl - "$resprefix"redirects.txt lnklexem T lc $pattern_stopwords_qry > $pattern_lnkfeat_doc
cat "$resprefix"pagerank.txt    | iconv -c -f utf-8 -t utf-8 - | sed -E 's/[\\\>\<\"/\!\?\:\;\-]/ /g' | $scriptdir/createTitleRules.pl - "$resprefix"redirects_empty.txt titlexem T lc $pattern_stopwords_qry > $pattern_titlefeat_doc



