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

def nnp_ouput( seq):
    halfsize = len(seq) / 2
    if len(seq) == (halfsize * 2) and (seq[0:halfsize] == seq[halfsize:] or seq[0:halfsize] == seq[halfsize:][::-1]):
        return ' '.join( ['_'.join( seq[0:halfsize]), '_'.join( seq[halfsize:]) ])
    else:
        if len(seq) > 4:
            try:
                halfsize = seq[ 1:].index( seq[0])+1
                for ee in seq[0:halfsize]:
                    dupidx = seq[ halfsize:].index( ee)
                return ' '.join( ['_'.join( seq[0:halfsize]), '_'.join( seq[halfsize:]) ])
            except:
                pass
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

for line in fileinput.input():
    print concat_nounphrases( line)



