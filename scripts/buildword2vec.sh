#!/bin/sh

# 24 CORE MACHINE:
outprefix=origdata/
inprefix=origdata/
srcprefix=github/strusWikipediaSearch/
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
	nlp_outputfile="$outprefix""docs.nlp.$jobid.txt"
	dic_outputfile="$outprefix""dict.$jobid.txt"
	rm $dmp_outputfile
	rm $nlp_outputfile
	for dd in $infiles ; do echo "-------- $dd"; $scriptdir/nlpdump.sh "$inprefix"wikipedia$dd.tar.gz $jobid "$outprefix"tmp $dmp_outputfile $srcprefix; done
	$scriptdir/strusnlp.py nlp $dmp_outputfile > $nlp_outputfile
	rm $dmp_outputfile
	$scriptdir/strusnlp.py makedict $nlp_outputfile 3 > $dic_outputfile
}

# runNLP 1 "00 13 08 01 14 26 18" &
# runNLP 2 "03 16 21 04 17 02 15" &
# runNLP 3 "06 19 11 07 20 05 12" &
# runNLP 4 "09 22 24 10 23 25" &

runNLP 00 "00" &
runNLP 01 "01" &
runNLP 02 "02" &
runNLP 03 "03" &
runNLP 04 "04" &
runNLP 05 "05" &
runNLP 06 "06" &
runNLP 07 "07" &
runNLP 08 "08" &
runNLP 09 "09" &
runNLP 10 "10" &
runNLP 11 "11" &
runNLP 12 "12" &
runNLP 13 "13" &
runNLP 14 "14" &
runNLP 15 "15" &
runNLP 16 "16" &
runNLP 17 "17" &
runNLP 18 "18" &
runNLP 19 "19" &
runNLP 20 "20" &
runNLP 21 "21" &
runNLP 22 "22" &
runNLP 23 "23" &
runNLP 24 "24" &
runNLP 25 "25" &
runNLP 26 "26" &

$scriptdir/strusnlp.py joindict "$outprefix"dict.{0,1,2,3,4,5,6,7,8,9}.txt > "$outprefix"dict.txt
rm dict.{0,1,2,3,4,5,6,7,8,9}.txt
$scriptdir/strusnlp.py splitdict "$outprefix"dict.txt "$outprefix"dict.split.txt

buildText() {
	jobid=$1
	$scriptdir/strusnlp.py concat "$outprefix"docs.nlp.$jobid.txt "$outprefix"dict.split.txt > "$outprefix"docs.word2vec.$jobid.txt
}

for dd in 0 1 2 3 4 5 6 7 8 9
do
	buildText $dd &
done

cat "$outprefix"docs.word2vec.{0,1,2,3,4,5,6,7,8,9}.txt > "$outprefix"docs.word2vec.txt

../word2vec/bin/word2vec -size 300 -window 8 -sample 1e-5 -negative 8 -threads 4 -min-count 4 -alpha 0.025 -classes 0 -debug 1 -binary 1 -save-vocab vocab.txt -cbow 0 -train "$outprefix"docs.word2vec.txt -output "$outprefix"vectors.bin


