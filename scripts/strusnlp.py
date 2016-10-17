#!/usr/bin/python
#
# Do taggig of noun phrases
# Read input from stdin
#
# This script requires resources to be installed:
#    $ sudo mkdir /usr/share/nltk_data
#    $ sudo python -m nltk.downloader -d /usr/share/nltk_data all
#
import nltk
from pprint import pprint
import fileinput
import sys
import io

reload(sys)
sys.setdefaultencoding('utf-8')

nnp_dict = {}

def nnp_ouput( seq):
    halfsize = len(seq) / 2
    if len(seq) == (halfsize * 2) and (seq[0:halfsize] == seq[halfsize:] or seq[0:halfsize] == seq[halfsize:][::-1]):
        elem1 = '_'.join( seq[0:halfsize])
        elem2 = '_'.join( seq[halfsize:])
        return ' '.join( [elem1, elem2])
    else:
        if len(seq) > 4:
            try:
                halfsize = seq[ 1:].index( seq[0])+1
                for ee in seq[0:halfsize]:
                    dupidx = seq[ halfsize:].index( ee)
                elem1 = '_'.join( seq[0:halfsize])
                elem2 = '_'.join( seq[halfsize:])
                return ' '.join( [elem1, elem2])
            except:
                pass
        for halfsize in reversed( range( 2, len(seq)-1)):
            if len(seq[halfsize]) < 4 and nnp_dict.get( '_'.join( seq[0:halfsize])) != None:
                elem1 = '_'.join( seq[0:halfsize])
                elem2 = '_'.join( seq[halfsize:])
                return ' '.join( [elem1, elem2])
        nnp_dict[ '_'.join( seq)] = 1
        return '_'.join( seq)

def concat_nounphrases( text):
    tokens = nltk.word_tokenize( text)
    tagged = nltk.pos_tag( tokens)
    seq = []
    pre = None
    rt = ""
    curtk = None
    for tg in tagged:
        print "%s %s" % (tg[1],tg[0])
        if tg[1] == "JJ" and tg[0] == "non":
            if seq:
                rt += nnp_ouput( seq) + " "
                seq = []
            curtk = "non"
        elif tg[1] == "NNP" or tg[1] == "NN":
            if curtk != None and curtk != tg[1]:
                if curtk == "non":
                    if tg[1] == "NN":
                        seq.append( "non")
                    else:
                        rt += "non "
                else:
                    rt += nnp_ouput( seq) + " "
                    seq = []
            seq.append( tg[0])
            curtk = tg[1]
        else:
            if seq:
                rt += nnp_ouput( seq) + " "
                seq = []
            rt += tg[0] + " "
            curtk = None
    if curtk == "non":
        rt += "non "
    if seq:
        rt += nnp_ouput( seq) + " "
    return rt

for line in io.open( sys.argv[1], encoding='utf-8'):
    print concat_nounphrases( line)



