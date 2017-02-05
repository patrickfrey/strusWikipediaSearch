#!/bin/sh

# 24 CORE MACHINE:
outprefix=origdata/
blkrefix=nlpdata/
srcprefix=github/strusWikipediaSearch/
w2wprefix=github/word2vec/bin/
scriptdir="$srcprefix"scripts

# TOIMUB:
outprefix=vecdata/
blkrefix=vecdata/
srcprefix=strusWikipediaSearch/
w2wprefix=word2vec/bin/
scriptdir="$srcprefix"scripts

runLINKS() {
	jobid=$1
	infile=$2
	dmp_outputfile="$outprefix""links.$infile.txt"

	rm $dmp_outputfile
	$scriptdir/linkdump.sh "$outprefix"wikipedia$infile.tar.gz "$jobid" tmp "$dmp_outputfile" $srcprefix
}

rm "$outprefix""links.all.txt"
for dd in 00 01 02 03 04 05 06 07 08 09 10 11 12 13 14 15 16 17 18 19 20 21 22 23 24 25 26
do
	runLINKS 1 $dd
	cat "$outprefix""links.$dd.txt" >> "$outprefix""links.all.txt"
done

runNLP() {
	jobid=$1
	infiles=$2
	dmp_outputfile="$outprefix""docs.dump.$jobid.txt"
	nlp_outputfile="$outprefix""docs.nlp.$jobid.txt"
	dic_outputfile="$outprefix""dict.$jobid.txt"
	rm $dmp_outputfile
	rm $nlp_outputfile
	for dd in $infiles; do echo "PROCESS $dd"; $scriptdir/nlpdump.sh "$outprefix"wikipedia$dd.tar.gz $jobid tmp $dmp_outputfile $srcprefix; done
	$scriptdir/strusnlp.py nlp $dmp_outputfile > $nlp_outputfile
	mkdir -p "$outprefix"data
	mv $dmp_outputfile "$outprefix"data/
	$scriptdir/strusnlp.py makedict $nlp_outputfile > $dic_outputfile
}

runNLP 00 "00 01 02 03" &
runNLP 01 "04 05 06 07" &
runNLP 02 "08 09 10 11" &
runNLP 03 "12 13 14 15" &
runNLP 04 "16 17 18 19" &
runNLP 05 "20 21 22 23" &
runNLP 06 "24 25 26" &

$scriptdir/strusnlp.py joindict "$outprefix"dict.{00,01,02,03,04,05,06}.txt > "$outprefix"dict.txt
$scriptdir/strusnlp.py seldict "$outprefix"dict.txt 2 > "$outprefix"dict.sel.txt
$scriptdir/strusnlp.py splitdict "$outprefix"dict.sel.txt "$outprefix"title.txt > "$outprefix"dict.split.txt
mkdir -p "$outprefix"data
rm "$outprefix"dict.sel.txt
mv "$outprefix"dict.{00,01,02,03,04,05,06}.txt "$outprefix"data/

buildText() {
	jobid=$1
	$scriptdir/strusnlp.py concat "$outprefix"docs.nlp.$jobid.txt "$outprefix"dict.split.txt "$outprefix"title.txt > "$outprefix"docs.word2vec.$jobid.txt
}

for dd in 00 01 02 03 04 05 06
do
	buildText $dd
done

cat "$outprefix"docs.word2vec.{00,01,02,03,04,05,06}.txt > "$outprefix"docs.word2vec.txt

"$w2wprefix"word2vec -size 300 -window 8 -sample 1e-5 -negative 8 -threads 24 -min-count 4 -alpha 0.025 -classes 0 -debug 1 -binary 1 -portable 1 -save-vocab "$outprefix"vocab.txt -cbow 0 -train "$outprefix"docs.word2vec.txt -output "$outprefix"vectors.bin

strusCreateVectorStorage -S "$srcprefix"config/vsm.conf -f "$outprefix"vectors.bin
strusBuildVectorStorage -S "$srcprefix"config/vsm.conf

# BUILD DYM STORAGE
dymfile="$outprefix"dymitems.txt
dymdocs="$outprefix"dymitems.xml

createDymXML()
{
	echo '<?xml version="1.0" encoding="UTF-8" standalone="yes"?>' > $2
	echo '<list>' >> $2
	cat $1 | awk '{print "<item>" $0 "</item>"}' >> $2
	echo '</list>' >> $2
}

rm -Rf "$outprefix"dymitems
mkdir -p "$outprefix"dymitems
strusInspectVectorStorage -S "$srcprefix"config/vsm.conf featname | iconv -c -f utf-8 -t utf-8 - | sed -E 's/[\\\>\<]//g' | awk '{print $2}' | sed -E 's/_+/ /g' | split -l 100000 - "$outprefix"dymitems/doc
for ff in `find "$outprefix"dymitems/ -name "doc*" -type f`; do mv $ff $ff.txt; createDymXML $ff.txt $ff.xml; done
rm "$outprefix"dymitems/*.txt

strusDestroy -s "path=$blkrefix"storage_dym
strusCreate -s "path=$blkrefix""storage_dym;max_open_files=256;write_buffer_size=512K;block_size=4K;metadata=doclen UINT8"
time -p strusInsert -L error_insert_dym.log -s "path=$blkrefix""storage_dym;max_open_files=256;write_buffer_size=512K;block_size=4K" -R resources -m analyzer_wikipedia_search -f 1 -c 60000 -t 8 -x "xml" "$srcprefix"config/dym.ana "$outprefix"dymitems/

# BUILD CONCEPTS 
pattern_forwardfeat_doc="$outprefix"pattern_forwardfeat_doc.txt
pattern_searchfeat_doc="$outprefix"pattern_searchfeat_doc.txt
pattern_searchfeat_qry="$outprefix"pattern_searchfeat_qry.txt
pattern_stopwords_doc="$outprefix"dict.stopwords_doc.txt
pattern_stopwords_qry="$outprefix"dict.stopwords_qry.txt
pattern_vocabulary="$outprefix"dict.vocabulary.txt
pattern_lnkfeat_doc="$outprefix"pattern_lnkfeat_doc.txt
pattern_titlefeat_doc="$outprefix"pattern_titlefeat_doc.txt

cat "$outprefix"docs.word2vec.txt | $scriptdir/countTerms.pl > $pattern_vocabulary
$scriptdir/strusnlp.py seldict $pattern_vocabulary 1000000 | grep -v '_' > $pattern_stopwords_qry
$scriptdir/strusnlp.py seldict $pattern_vocabulary 500000 > $pattern_stopwords_doc

echo "" > "$outprefix"redirects_empty.txt
strusPageRank -i 100 -g -n 100 -r "$outprefix"redirects.txt "$outprefix"links.all.txt > "$outprefix"pagerank.txt

strusInspectVectorStorage -S "$srcprefix"config/vsm.conf featname    | iconv -c -f utf-8 -t utf-8 - | sed -E 's/[\\\>\<\"/]//g' | grep '_' | $scriptdir/createFeatureRules.pl - lexem F '' $pattern_stopwords_doc > $pattern_searchfeat_doc
strusInspectVectorStorage -S "$srcprefix"config/vsm.conf featname    | iconv -c -f utf-8 -t utf-8 - | sed -E 's/[\\\>\<\"/]//g' | grep '_' | $scriptdir/createFeatureRules.pl - lexem name '' $pattern_stopwords_doc > $pattern_forwardfeat_doc
strusInspectVectorStorage -S "$srcprefix"config/vsm.conf featname    | iconv -c -f utf-8 -t utf-8 - | sed -E 's/[\\\>\<\"/]//g' | $scriptdir/createFeatureRules.pl - lexem F lc $pattern_stopwords_qry > $pattern_searchfeat_qry
cat "$outprefix"pagerank.txt    | iconv -c -f utf-8 -t utf-8 - | sed -E 's/[\\\>\<\"/\!\?\:\;\-]/ /g' | $scriptdir/createTitleRules.pl - "$outprefix"redirects.txt lnklexem T lc $pattern_stopwords_qry > $pattern_lnkfeat_doc
cat "$outprefix"pagerank.txt    | iconv -c -f utf-8 -t utf-8 - | sed -E 's/[\\\>\<\"/\!\?\:\;\-]/ /g' | $scriptdir/createTitleRules.pl - "$outprefix"redirects_empty.txt titlexem T lc $pattern_stopwords_qry > $pattern_titlefeat_doc



