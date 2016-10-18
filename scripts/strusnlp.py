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
import codecs

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


def match_tag( tg, seektg):
    if seektg[1] == tg[1] and (seektg[0] == None or seektg[0] == tg[0]):
        return True
    return False

def concat_sequences( tagged, elem0, elem1, jointype, joinchr):
    rt = []
    state = 0
    bufelem = None
    for tg in tagged:
        if state == 0:
            if match_tag( tg, elem0):
                state = 1
                bufelem = tg
            else:
                rt.append( tg)
        elif state == 1:
            if match_tag( tg, elem1):
                bufelem = [bufelem[0] + joinchr + tg[0], jointype]
            else:
                rt.append( bufelem)
                if match_tag( tg, elem0):
                    bufelem = tg
                else:
                    rt.append( tg)
                    state = 0
                    bufelem = None
        else:
            print( "illegal state in concat_seq")
            raise
    if state == 1 and bufelem != None:
        rt.append( bufelem)
    return rt

def concat_pairs( tagged, elem0, elem1, jointype, joinchr):
    rt = []
    state = 0
    bufelem = None
    for tg in tagged:
        if state == 0:
            if match_tag( tg, elem0):
                state = 1
                bufelem = tg
            else:
                rt.append( tg)
        elif state == 1:
            if match_tag( tg, elem1):
                rt.append( [bufelem[0] + joinchr + tg[0], jointype])
                state = 0
                bufelem = None
            else:
                rt.append( bufelem)
                if match_tag( tg, elem0):
                    bufelem = tg
                else:
                    rt.append( tg)
                    state = 0
                    bufelem = None
        else:
            print( "illegal state in concat_seq")
            raise
    if state == 1 and bufelem != None:
        rt.append( bufelem)
    return rt

def tag_first( tagged, elem0, elem1, skiptypes, joinchr):
    rt = tagged
    state = 0
    elemidx = None
    for tgidx,tg in enumerate(tagged):
        if state == 0:
            if match_tag( tg, elem0):
                state = 1
                elemidx = tgidx
        elif state == 1:
            if tg[1] in skiptypes:
                pass
            elif match_tag( tg, elem1):
                rt[ elemidx] = [ rt[ elemidx][0] + joinchr + tg[0], rt[ elemidx][1] ]
                state = 0
                elemidx = None
            elif match_tag( tg, elem0):
                state = 1
                elemidx = tgidx
            else:
                state = 0
                elemidx = None
        else:
            print( "illegal state in tag_first")
            raise
    return rt

def get_tagged( text):
    tokens = nltk.word_tokenize( text)
    tagged = nltk.pos_tag( tokens)

#    print "SRC %s" % tagged
    tagged = tag_first( tagged, ["is","VBZ"], ["a","DT"], ["RB"], "_")
    tagged = tag_first( tagged, ["is","VBZ"], ["the","DT"], ["RB"], "_")
    tagged = tag_first( tagged, ["was","VBD"], ["a","DT"], ["RB"], "_")
    tagged = tag_first( tagged, ["was","VBD"], ["the","DT"], ["RB"], "_")
    tagged = concat_pairs( tagged, [None,"NN"], ["er","NN"], "NN", "")
    tagged = concat_pairs( tagged, [None,"NN"], ["s","NN"], "NN", "")
    tagged = concat_pairs( tagged, [None,"NN"], ["I","PRP"], "NN", "")
    tagged = concat_pairs( tagged, [None,"NNP"], ["n","JJ"], "NNP", "")
    tagged = concat_pairs( tagged, ["non","JJ"], [None,"NN"], "NN", "_")
    tagged = concat_pairs( tagged, [None,"NN"], ["s","NN"], "NN", "")
    tagged = concat_sequences( tagged, [None,"NN"], [None,"NN"], "NN", "_")
    tagged = concat_sequences( tagged, [None,"NNP"], [None,"NNP"], "NN", "_")
    return tagged

def concat_phrases( text):
    tagged = get_tagged( text)
    if not tagged:
        return ""
#    print "RES %s" % tagged
    rt = tagged[0][0]
    for tg in tagged[1:]:
        rt += " " + tg[0]
    return rt

def fill_dict( text):
    tagged = get_tagged( text)
    if tagged:
        for tg in tagged:
            if tg[1] == "NNP" or tg[1] == "NN":
                if tg[0] in nnp_dict:
                    nnp_dict[ tg[0]] += 1
                else:
                    nnp_dict[ tg[0]] = 1

cmd = sys.argv[1]

if cmd == "dict":
    infile = sys.argv[2]
    for line in codecs.open( infile, "r", encoding='utf-8'):
        fill_dict( line)
        mincnt = 50
        if len(sys.argv) > 3:
            mincnt = int(sys.argv[3])
    for key,value in nnp_dict.iteritems():
        if value > mincnt:
            print "%s %u" % (key,value)

elif cmd == "concat":
    infile = sys.argv[2]
    for line in codecs.open( infile, "r", encoding='utf-8'):
        print concat_phrases( line.encode('utf-8'))

else:
    print "ERROR unknown command"
    raise


