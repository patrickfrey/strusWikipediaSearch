#!/bin/sh

# 24 CORE MACHINE:
outprefix=origdata/
inprefix=origdata/
srcprefix=github/strusWikipediaSearch/
w2wprefix=github/word2vec/bin/
scriptdir="$srcprefix"scripts
# TOIMUB:
# outprefix=nlpdata/
# inprefix=data/
# srcprefix=
# scriptdir="$srcprefix"scripts

runNLP() {
	jobid=$1
	infiles=$2
	dmp_outputfile="$outprefix""docs.dump.$jobid.txt"
	dmp_titlefile="$outprefix""title.$jobid.txt"
	nlp_outputfile="$outprefix""docs.nlp.$jobid.txt"
	dic_outputfile="$outprefix""dict.$jobid.txt"
	rm $dmp_outputfile
	rm $dmp_titlefile
	rm $nlp_outputfile
	for dd in $infiles; do echo "PROCESS $dd"; $scriptdir/nlpdump.sh "$inprefix"wikipedia$dd.tar.gz $jobid "$outprefix"tmp $dmp_outputfile $dmp_titlefile $srcprefix; done
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

#,07,08,09,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26
cat "$outprefix"title.{00,01,02,03,04,05,06}.txt | sort | uniq > "$outprefix"title.txt
$scriptdir/strusnlp.py joindict "$outprefix"dict.{00,01,02,03,04,05,06}.txt > "$outprefix"dict.txt
$scriptdir/strusnlp.py splitdict "$outprefix"dict.txt "$outprefix"title.txt > "$outprefix"dict.split.txt
mkdir -p "$outprefix"data
mv "$outprefix"dict.{00,01,02,03,04,05,06}.txt "$outprefix"data/
mv "$outprefix"title.{00,01,02,03,04,05,06}.txt "$outprefix"data/

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

strusCreateVsm -S "$srcprefix"config/vsm.conf -f "$outprefix"vectors.bin
strusBuildVsm -S "$srcprefix"config/vsm.conf

