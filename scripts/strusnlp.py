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
import math

reload(sys)
sys.setdefaultencoding('utf-8')

nnp_dict = {}
nnp_left_dict = {}
nnp_right_dict = {}

def fill_nnp_split_dict():
    for key,value in nnp_dict.iteritems():
        halfsize = key.find('_')
        while halfsize != -1:
            leftkey = key[0:halfsize]
            if leftkey in nnp_left_dict:
                nnp_left_dict[ leftkey] += value
            else:
                nnp_left_dict[ leftkey] = value
            rightkey = key[(halfsize+1):]
            if rightkey in nnp_right_dict:
                nnp_right_dict[ rightkey] += value
            else:
                nnp_right_dict[ rightkey] = value
            halfsize = key.find('_',halfsize+1)


def nnp_split( seqword):
#    print "SPLIT '%s'" % seqword
    seqlen = 1
    halfsize = seqword.find('_')
    while halfsize != -1:
       seqlen += 1
       halfsize = seqword.find('_',halfsize+1)
    candidates = []
    if seqword in nnp_dict:
        candidates.append( [ None, math.log( nnp_dict[ seqword]) ] )
    halfsize = seqword.find('_')
    len1 = 0
    len2 = seqlen
    while halfsize != -1:
        half1 = seqword[ 0:halfsize]
        w1 = 0.0
        len1 += 1
        half2 = seqword[ (halfsize+1):]
        w2 = 0.0
        len2 -= 1
        if half1 in nnp_dict:
            if half1 in nnp_left_dict:
#                print "    LEFT OCCUR '%s' %f %f" % (half1, float(nnp_dict[ half1]), float(nnp_left_dict[ half1]))
                w1 = math.log(nnp_dict[ half1]) / (1.0 + math.log(nnp_left_dict[ half1]))
            else:
#                print "    LEFT OCCUR '%s' %f" % (half1, float(nnp_dict[ half1]))
                w1 = math.log(nnp_dict[ half1])
#        print "    WEIGHT '%s' %f" % (half1, w1)
        if half2 in nnp_dict:
            if half2 in nnp_right_dict:
#                print "    RIGHT OCCUR '%s' %f %f" % (half2, float(nnp_dict[ half2]), float(nnp_right_dict[ half2]))
                w2 = math.log( nnp_dict[ half2]) / (1.0 + math.log(nnp_right_dict[ half2]))
            else:
#                print "    RIGHT OCCUR '%s' %f" % (half2, float(nnp_dict[ half2]))
                w2 = math.log(nnp_dict[ half2])
#        print "    WEIGHT '%s' %f" % (half2, w2)
        candidates.append( [halfsize, w1 + w2] )
        halfsize = seqword.find('_',halfsize+1)
    best_halfsize = None
    best_weight = 0.0
    for cd in candidates:
        if cd[1] > best_weight:
            best_halfsize = cd[0]
            best_weight = cd[1]
#        if cd[0] == None:
#            print "    CANDIDATE '%s' %f" % (seqword, cd[1])
#        else:
#            print "    CANDIDATE '%s' '%s' %f" % (seqword[ 0:(cd[0])], seqword[ (cd[0]+1):], cd[1])
#    if best_halfsize == None:
#        print "    BEST None"
#    else:
#        print "    BEST %u" % best_halfsize
    return best_halfsize

def nnp_split_words( seqword):
    rt = []
    halfsize = nnp_split( seqword)
    if halfsize == None:
        return [ seqword ]
    half1 = seqword[ 0:halfsize]
    half2 = seqword[ (halfsize+1):]
    return nnp_split_words( half1) + nnp_split_words( half2)

def match_tag( tg, seektg):
    if tg[1] in seektg[1:] and (seektg[0] == None or seektg[0] == tg[0]):
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
            print >> sys.stderr, "illegal state in concat_seq"
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
            print >> sys.stderr, "illegal state in concat_seq"
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
            print >> sys.stderr, "illegal state in tag_first"
            raise
    return rt

def tag_tokens_NLP( text):
    tokens = nltk.word_tokenize( text)
    tagged = nltk.pos_tag( tokens)
#    print "NLP %s" % tagged
    tagged = tag_first( tagged, [None,"VB","VBZ","VBD","VBG","VBP","VBZ"], [None,"IN","TO"], ["RB","RBZ","RBS"], "_")
    tagged = tag_first( tagged, [None,"VB","VBZ","VBD","VBG","VBP","VBZ"], ["a","DT"], ["RB","RBZ","RBS"], "_")
    tagged = tag_first( tagged, [None,"VB","VBZ","VBD","VBG","VBP","VBZ"], ["the","DT"], ["RB","RBZ","RBS"], "_")
    tagged = concat_pairs( tagged, [None,"NN"], ["er","NN"], "NN", "")
    tagged = concat_pairs( tagged, [None,"NN"], ["s","NN"], "NN", "")
    tagged = concat_pairs( tagged, [None,"NN"], ["I","PRP"], "NN", "_")
    tagged = concat_pairs( tagged, [None,"NNP"], ["n","JJ"], "NNP", "")
    tagged = concat_pairs( tagged, [None,"NNP"], ["s","NN"], "NNP", "")
    tagged = concat_pairs( tagged, ["non","JJ"], [None,"NN"], "NN", "_")
    tagged = concat_sequences( tagged, [None,"NN"], [None,"NN"], "NN", "_")
    tagged = concat_sequences( tagged, [None,"NNP"], [None,"NNP"], "NN", "_")
    return tagged

def get_tagged_tokens( text):
    rt = []
    tokens = text.strip().split()
    for tk in tokens:
        spidx = tk.find('#')
        rt.append( [ tk[(spidx+1):], tk[0:spidx] ])
    return rt

def concat_word( tg):
    if tg[1] == "NNP" or tg[1] == "NN":
        return ' '.join( nnp_split_words( tg[0]))
    else:
        return tg[0]

def concat_phrases( text):
    tagged = get_tagged_tokens( text)
    if not tagged:
        return ""
#    print "RES %s" % tagged
    rt = concat_word( tagged[0])
    for tg in tagged[1:]:
        rt += " " + concat_word( tg)
    return rt

def fill_dict( text):
    tagged = get_tagged_tokens( text)
    if tagged:
        for tg in tagged:
            if tg[1] == "NNP" or tg[1] == "NN":
                key = tg[0].decode('utf-8')
                if key in nnp_dict:
                    nnp_dict[ key] += 1
                else:
                    nnp_dict[ key] = 1

def tag_NLP( text):
    tagged = tag_tokens_NLP( text)
    rt = ""
    if tagged:
        for tg in tagged:
            rt += tg[1] + "#" + tg[0] + " "
    return rt

cmd = None
if len( sys.argv) > 1:
    cmd = sys.argv[1]

def print_usage():
    print "usage strusnlp.py <command> ..."
    print "<command>:"
    print "    dict <infile> [<mincnt>]:"
    print "        print the content of the dictionary passed as argument"
    print "        do not print elements with a lower count than <mincnt>"
    print "        default for <mincnt> is 50"
    print "    npl <infile>:"
    print "        do NLP with the Python NLTK library on <infile>"
    print "    joindict { <dictfile> }:"
    print "        join several dictionaries passed as arguments"
    print "    splitdict <dictfile>:"
    print "        try to split entries in the dictionary"
    print "        use the term occurrence statistics to make decisions"
    print "    seldict <dictfile> [<mincnt>]:"
    print "        select entries in a dictionary <dictfile> with a lower count than <mincnt>"
    print "        default for <mincnt> is 50"
    print "    concat <infile> [<dictfile>]:"
    print "        produces phrases from NLP output and with help of a dictionary"
    
if cmd == None or cmd == '-h' or cmd == '--help':
    print_usage()

elif cmd == "dict":
    infile = sys.argv[2]
    linecnt = 0
    for line in codecs.open( infile, "r", encoding='utf-8'):
        if line.strip():
            fill_dict( line)
            linecnt += 1
            if linecnt % 10000 == 0:
                print >> sys.stderr, "processed %u lines" %linecnt
    mincnt = 50
    if len(sys.argv) > 3:
        mincnt = int(sys.argv[3])
    for key,value in nnp_dict.iteritems():
        if value >= mincnt:
            print "%s %u" % (key,value)

elif cmd == "nlp":
    infile = sys.argv[2]
    linecnt = 0
    for line in codecs.open( infile, "r", encoding='utf-8'):
        if line.strip():
            print "%s" % tag_NLP( line)
            linecnt += 1
            if linecnt % 10000 == 0:
                print >> sys.stderr, "processed %u lines" %linecnt

elif cmd == "joindict":
    infile = []
    for dictfile in sys.argv[2:]:
        for line in codecs.open( dictfile, "r", encoding='utf-8'):
            if line.strip():
                tokstr,tokcnt = line.strip().split()
                key = tokstr.decode('utf-8')
                if key in nnp_dict:
                    nnp_dict[ key] = nnp_dict[ key] + int(tokcnt)
                else:
                    nnp_dict[ key] = int(tokcnt)
    for key,value in nnp_dict.iteritems():
        print "%s %u" % (key,value)

elif cmd == "splitdict":
    dictfile = sys.argv[2]
    for line in codecs.open( dictfile, "r", encoding='utf-8'):
        if line.strip():
            tokstr,tokcnt = line.strip().split()
            key = tokstr.decode('utf-8')
            if key in nnp_dict:
                nnp_dict[ key] = nnp_dict[ key] + int(tokcnt)
            else:
                nnp_dict[ key] = int(tokcnt)
    fill_nnp_split_dict()
    new_dict = {}
    for key,value in nnp_dict.iteritems():
        for word in nnp_split_words( key):
            if word in new_dict:
                new_dict[ word] += value
            else:
                new_dict[ word] = value
    for key,value in new_dict.iteritems():
        print "%s %u" % (key,value)

elif cmd == "seldict":
    dictfile = sys.argv[2]
    for line in codecs.open( dictfile, "r", encoding='utf-8'):
        if line.strip():
            tokstr,tokcnt = line.strip().split()
            key = tokstr.decode('utf-8')
            if key in nnp_dict:
                nnp_dict[ key] = nnp_dict[ key] + int(tokcnt)
            else:
                nnp_dict[ key] = int(tokcnt)
    mincnt = 50
    if len(sys.argv) > 3:
        mincnt = int(sys.argv[3])
    for key,value in nnp_dict.iteritems():
        if value >= mincnt:
            print "%s %u" % (key,value)

elif cmd == "concat":
    infile = sys.argv[2]
    if len(sys.argv) > 3:
        dictfile = sys.argv[3]
        for line in codecs.open( dictfile, "r", encoding='utf-8'):
            if line.strip():
                tokstr,tokcnt = line.strip().split()
                nnp_dict[ tokstr.decode('utf-8')] = int(tokcnt)
        fill_nnp_split_dict()
    linecnt = 0
    for line in codecs.open( infile, "r", encoding='utf-8'):
        print concat_phrases( line.encode('utf-8'))
        linecnt += 1
        if linecnt % 10000 == 0:
            print >> sys.stderr, "processed %u lines" %linecnt

else:
    print >> sys.stderr, "unknown command"
    print_usage()
    raise


