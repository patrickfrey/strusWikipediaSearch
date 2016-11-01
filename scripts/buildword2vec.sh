#!/bin/sh

outprefix=nlpdata/
scriptdir=scripts

runNLP() {
	jobid=$1
	infiles=$2
	dmp_outputfile="$outprefix""docs.dump.$jobid.txt"
	nlp_outputfile="$outprefix""docs.nlp.$jobid.txt"
	dic_outputfile="$outprefix""dict.$jobid.txt"
	rm $dmp_outputfile
	rm $nlp_outputfile
	for dd in $infiles ; do echo "-------- $dd"; $scriptdir/nlpdump.sh data/wikipedia$dd.tar.gz $jobid "$outprefix"tmp $dmp_outputfile; done
	$scriptdir/strusnlp.py nlp $dmp_outputfile > $nlp_outputfile
	rm $dmp_outputfile
	$scriptdir/strusnlp.py makedict $nlp_outputfile 3 > $dic_outputfile
}

runNLP 1 "00 13 08" &
runNLP 2 "03 16 21" &
runNLP 3 "06 19 11" &
runNLP 4 "09 22 24" &
runNLP 5 "01 14 26" &
runNLP 6 "04 17" &
runNLP 7 "07 20" &
runNLP 8 "10 23 25" &
runNLP 9 "02 15" &
runNLP 0 "05 18 12" &

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


