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
import re
from sets import Set
from copy import copy

reload(sys)
sys.setdefaultencoding('utf-8')

nnp_dict = {}
nnp_left_dict = {}
nnp_right_dict = {}
title_dict = {}

def fill_nnp_split_dict():
    for key,value in nnp_dict.iteritems():
        if key[0] == '_':
            continue
        halfsize = key.find('_')
        while halfsize != -1:
            leftkey = key[0:halfsize]
            if leftkey[-1] == '.':
                leftkey = leftkey[ 0:-1]
            if leftkey in nnp_left_dict:
                nnp_left_dict[ leftkey] += value
            else:
                nnp_left_dict[ leftkey] = value
            rightkey = key[(halfsize+1):]
            if rightkey:
                if rightkey[-1] == '.':
                    rightkey = rightkey[ 0:-1]
                if rightkey in nnp_right_dict:
                    nnp_right_dict[ rightkey] += value
                else:
                    nnp_right_dict[ rightkey] = value
            halfsize = key.find('_',halfsize+1)

def nnp_left_weight( word):
    occ = 0
    leftocc = 0
    if word in nnp_left_dict:
        leftocc += nnp_left_dict[ word]
    if word in nnp_dict:
        occ += nnp_dict[ word]
    if len(word) > 4 and word[ -1] == 's':
        if word[:-1] in nnp_left_dict:
            leftocc += nnp_left_dict[ word[:-1]]
        if word[:-1] in nnp_dict:
            occ += nnp_dict[ word[:-1]]
    dv = float( occ + 1) / float(leftocc + 1)
    return math.log( 1.0 + dv),occ,leftocc,dv

def nnp_right_weight( word):
    occ = 0
    rightocc = 0
    if word in nnp_right_dict:
        rightocc += nnp_right_dict[ word]
    if word in nnp_dict:
        occ += nnp_dict[ word]
    dv = float(occ + 1) / float(rightocc + 1)
    return math.log( 1.0 + dv),occ,rightocc,dv

def nnp_join_weight( occ, seqlen):
    if seqlen == 1:
        return math.log( float( occ + 1))
    else:
        return math.log( float( occ + 1) / float( seqlen))

def nnp_split( seqword, verbose):
    if verbose:
        print >> sys.stderr, "SPLIT '%s'" % seqword
    seqlen = 1
    if seqword[-1] == '.':
        seqword = seqword[ 0:-1]
    if seqword[0] == '_' or seqword in title_dict:
        return None
    halfsize = seqword.find('_')
    while halfsize != -1:
        seqlen += 1
        halfsize = seqword.find('_',halfsize+1)
    candidates = []
    halfsize = seqword.find('_')
    len1 = 0
    len2 = seqlen
    while halfsize != -1:
        half1 = seqword[ 0:halfsize]
        if half1[-1] == '.':
            half1 = half1[ 0:-1]
        half2 = seqword[ (halfsize+1):]
        if half2[-1] == '.':
            half2 = half2[ 0:-1]

        if half1[0].islower() and half2[0].isupper():
            if verbose:
                print >> sys.stderr, "    RULE up follow lo: '%s' '%s'" % (half1,half2)
            return halfsize
        w1,occ1,partocc1,dv1 = nnp_left_weight( half1)
        len1 += 1
        w2,occ2,partocc2,dv2 = nnp_right_weight( half2)
        len2 -= 1
        if verbose:
            print >> sys.stderr, "    WEIGHT '%s' %f %u %u %f" % (half1, w1,occ1,partocc1,dv1)
            print >> sys.stderr, "    WEIGHT '%s' %f %u %u %f" % (half2, w2,occ2,partocc2,dv2)
        candidates.append( [halfsize, (min(w1,w2) + w1 + w2) / 2.0] )
        halfsize = seqword.find('_',halfsize+1)
    if seqword in nnp_dict:
        occ = nnp_dict[ seqword]
        wjoin = nnp_join_weight( occ, seqlen)
        if verbose:
            print >> sys.stderr, "    FIRST WEIGHT '%s' %f (%u)" % (seqword, wjoin, nnp_dict[ seqword])
        candidates.append( [ None, wjoin ])
    best_halfsize = None
    best_weight = 0.0
    for cd in candidates:
        if cd[1] > best_weight:
            best_halfsize = cd[0]
            best_weight = cd[1]
            if verbose:
                if cd[0] == None:
                    print >> sys.stderr, "    CANDIDATE '%s' %f" % (seqword, cd[1])
                else:
                    print >> sys.stderr, "    CANDIDATE '%s' '%s' %f" % (seqword[ 0:(cd[0])], seqword[ (cd[0]+1):], cd[1])
    if verbose:
         if best_halfsize == None:
             print >> sys.stderr, "    BEST None"
         else:
             print >> sys.stderr, "    BEST %u" % best_halfsize
    return best_halfsize

def nnp_split_words( seqword, verbose):
    rt = []
    halfsize = nnp_split( seqword, verbose)
    if halfsize == None:
        return [ seqword ]
    half1 = seqword[ 0:halfsize]
    half2 = seqword[ (halfsize+1):]
    return nnp_split_words( half1, verbose) + nnp_split_words( half2, verbose)

def parse_tokendef( tk):
    spidx = tk.find('#')
    if spidx == 0:
        return [ tk, None ]
    elif spidx < len(tk)-1:
        return [ tk[(spidx+1):], tk[0:spidx] ]
    else:
        return [ None, tk[0:spidx] ]

def match_tag( tg, seektg):
    if (seektg[1] == None or tg[1] in seektg[1:]) and (seektg[0] == None or seektg[0] == tg[0]):
        return True
    return False

def find_sequence( tagged, sequence):
    rt = []
    matchidx = None
    state = 0
    if len( sequence) == 0:
        print >> sys.stderr, "empty sequence passed to find_sequence"
        raise
    for tidx,tg in enumerate( tagged):
        is_match = False
        if (sequence[state][1] == None or (sequence[state][1] is list and tg[1] in sequence[state][1]) or sequence[state][1] == tg[1]) and (sequence[ state][0] == None or sequence[ state][0] == tg[0]):
            is_match = True
        else:
            matchidx = None
            state = 0
            if (sequence[state][1] == None or (sequence[state][1] is list and tg[1] in sequence[state][1]) or sequence[state][1] == tg[1]) and (sequence[ state][0] == None or sequence[ state][0] == tg[0]):
                is_match = True
        if is_match == True:
            if state == 0:
                matchidx = tidx
            state += 1
            if state >= len( sequence):
                rt.append( matchidx)
                matchidx = None
                state = 0
    return rt

def mark_sequences( tagged, sequence, marker):
    rt = ""
    prevpos = 0
    for pos in find_sequence( tagged, sequence):
        for sq in tagged[ prevpos:pos]:
            if rt:
                rt += ' '
            rt += sq[1] + "#" + sq[0]
        if rt:
            rt += ' '
        rt += marker
        prevpos = pos
    for sq in tagged[ prevpos:]:
        if rt:
            rt += ' '
        rt += sq[1] + "#" + sq[0]
    return rt

def get_sequences( tagged, sequence):
    rt = []
    for pos in find_sequence( tagged, sequence):
        endpos = pos + len(sequence)
        seqstr = ""
        for sq in tagged[ pos:endpos]:
            if seqstr:
                seqstr += ' '
            seqstr += sq[1] + "#" + sq[0]
        rt.append( seqstr)
    return rt

def concat_sequences( tagged, elem0, elem1, jointype, joinchr):
    rt = []
    state = 0
    bufelem = None
    for tg in tagged:
        if state == 0:
            if tg[0][0] != '_' and match_tag( tg, elem0):
                state = 1
                bufelem = tg
            else:
                rt.append( tg)
        elif state == 1:
            if tg[0][0] != '_' and match_tag( tg, elem1):
                bufelem = [bufelem[0] + joinchr + tg[0], jointype]
            else:
                rt.append( bufelem)
                if tg[0][0] != '_' and match_tag( tg, elem0):
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
            if tg[0][0] != '_' and match_tag( tg, elem0):
                state = 1
                bufelem = tg
            else:
                rt.append( tg)
        elif state == 1:
            if tg[0][0] != '_' and match_tag( tg, elem1):
                rt.append( [bufelem[0] + joinchr + tg[0], jointype])
                state = 0
                bufelem = None
            else:
                rt.append( bufelem)
                if tg[0][0] != '_' and match_tag( tg, elem0):
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
            if tg[0][0] != '_' and match_tag( tg, elem0):
                state = 1
                elemidx = tgidx
        elif state == 1:
            if tg[1] in skiptypes:
                pass
            elif tg[0][0] != '_' and match_tag( tg, elem1):
                rt[ elemidx] = [ rt[ elemidx][0] + joinchr + tg[0], rt[ elemidx][1] ]
                state = 0
                elemidx = None
            elif tg[0][0] != '_' and match_tag( tg, elem0):
                state = 1
                elemidx = tgidx
            else:
                state = 0
                elemidx = None
        else:
            print >> sys.stderr, "illegal state in tag_first"
            raise
    return rt

def elim_plural( tagged):
    rt = []
    for tg in tagged:
        if tg[1] == "NNS":
            rt.append( [ tg[0], "NN" ])
        elif tg[1] == "NNPS":
            rt.append( [ tg[0], "NNP" ])
        else:
            rt.append( tg)
    return rt

def tag_tokens_NLP( text):
    tokens = nltk.word_tokenize( text)
    tagged = elim_plural( nltk.pos_tag( tokens))

#    print >> sys.stderr, "NLP %s" % tagged
    for tgidx in find_sequence( tagged, [[None,'NNP'],[None,None],[None,'NNP']]):
        if tagged[tgidx+1][0][0].isupper() == True or tagged[tgidx+1][1] == "IN":
            tagged[tgidx+1] = [ tagged[tgidx+1][0],"NNP" ]
    for tgidx in find_sequence( tagged, [[None,'NNP'],[["de","del","della","di","du","le","la","von","der","ibn","bin","al"],None],[None,'NNP']]):
        tagged[tgidx+1] = [ tagged[tgidx+1][0],"NNP" ]
    tagged = tag_first( tagged, [None,"VB","VBZ","VBD","VBG","VBP","VBZ"], [None,"IN","TO"], ["RB","RBZ","RBS"], "_")
    tagged = tag_first( tagged, [None,"VB","VBZ","VBD","VBG","VBP","VBZ"], ["a","DT"], ["RB","RBZ","RBS"], "_")
    tagged = tag_first( tagged, [None,"VB","VBZ","VBD","VBG","VBP","VBZ"], ["the","DT"], ["RB","RBZ","RBS"], "_")
    tagged = concat_pairs( tagged, [None,"NN"], ["er",None], "NN", "")
    tagged = concat_pairs( tagged, [None,"NN"], ["ers",None], "NN", "")
    tagged = concat_pairs( tagged, [None,"NN"], ["n",None], "NN", "")
    tagged = concat_pairs( tagged, [None,"NN"], ["ns",None], "NN", "")
    tagged = concat_pairs( tagged, [None,"NN"], ["s",None], "NN", "")
    tagged = concat_pairs( tagged, [None,"JJ"], [None,"NNP"], "NNP", "_")
    tagged = concat_pairs( tagged, ["The","DT"], [None,"NNP"], "NNP", "_")
    tagged = concat_pairs( tagged, [None,"NNP"], ["I","PRP"], "NNP", "_")
    tagged = concat_pairs( tagged, [None,"NNP"], ["ian","JJ"], "NNP", "")
    tagged = concat_pairs( tagged, [None,"NNP"], ["ese","JJ"], "NNP", "")
    tagged = concat_pairs( tagged, [None,"NNP"], ["n",None], "NNP", "")
    tagged = concat_pairs( tagged, [None,"NNP"], ["ns",None], "NNP", "")
    tagged = concat_pairs( tagged, [None,"NNP"], ["s",None], "NNP", "")
    tagged = concat_pairs( tagged, [None,"JJ"], [None,"NN"], "NN", "_")
    tagged = concat_sequences( tagged, [None,"NN"], [None,"NN"], "NN", "_")
    tagged = concat_sequences( tagged, [None,"NNP"], [None,"NNP"], "NNP", "_")
    return tagged

def get_tagged_tokens( text):
    rt = []
    tokens = text.strip().split()
    for tk in tokens:
        tkdef = parse_tokendef( tk)
        if tkdef != None:
            rt.append( tkdef)
    return rt

digits_pattern = re.compile( "(\d\d)(\d\d\d+)")

def normalize_numbers( word):
    result = digits_pattern.search(word)
    while result != None:
        prev = result.group(1)
        match = result.group(2)
        repl = "".ljust( len(match), '#')
        word = word[ 0:result.start()] + prev + repl + word[ result.end():]
        result = digits_pattern.search( word)
    return word

def separate_affix_s( word):
    if len(word) > 4 and word[-1] == 's':
        if word[-2] == '_':
            if word[:-2] in nnp_dict:
                return word[:-2] + " s"
        elif word[:-1] in nnp_dict:
            occ_with_s = 0
            occ_without_s = nnp_dict[ word[:-1]]
            if word in nnp_dict:
                occ_with_s = nnp_dict[ word]
            if occ_without_s > occ_with_s * 3:
                return word[:-1] + " s"
    return word

def concat_word( tg):
    word = normalize_numbers( tg[0])
    if word[-1] == '.':
        word = word[ 0:-1] + " ."
    if tg[1] == "NNP":
        rt = ''
        for part in nnp_split_words( word, False):
            rt += ' ' + separate_affix_s( part)
        return rt
    if tg[1] == "NN":
        if len(word) >= 2 and word[0].isupper() and word[1].islower():
            word = word[0].lower() + word[1:]
        rt = ''
        for part in nnp_split_words( word, False):
            rt += ' ' + separate_affix_s( part)
        return rt
    if tg[1] == "PRP":
        return word
    else:
        if len(word) >= 2 and word[0].isupper() and word[1].islower():
            word = word[0].lower() + word[1:]
        return word

def concat_phrases( text):
    tagged = get_tagged_tokens( text)
    if not tagged:
        return ""
#    print >> sys.stderr, "RES %s" % tagged
    rt = concat_word( tagged[0])
    for tg in tagged[1:]:
        rt += " " + concat_word( tg)
    return rt

def mark_phrases( text, sequence, marker):
    tagged = get_tagged_tokens( text)
    if not tagged:
        return ""
    return mark_sequences( tagged, sequence, marker)

def get_phrases( text, sequence):
    tagged = get_tagged_tokens( text)
    if not tagged:
        return ""
    return get_sequences( tagged, sequence)

def fill_dict( text):
    tagged = get_tagged_tokens( text)
    if tagged:
        for tg in tagged:
            if tg[1] == "NNP" or tg[1] == "NN":
                key = tg[0]
                if key in nnp_dict:
                    nnp_dict[ key] += 1
                else:
                    nnp_dict[ key] = 1
            elif tg[1] == "NNPS" or tg[1] == "NNS":
                if tg[0][-1] == 's':
                    key = tg[0][:-1]
                else:
                    key = tg[0]
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

def get_phrase_id( text, pstart, pend):
    rt = ''
    spcidx = -1
    while text[pstart] == ' ':
        pstart += 1
    for spc in ['\'','.',',',';']:
        spcidx = text.find( spc, pstart, pend)
        if spcidx != -1:
            break
    if spcidx == -1:
        pi = pstart
        pn = text.find( ' ', pi+1, pend)
        while pn >= 0 and pn > pi+1:  # find spaces and do not accept two subsequent spaces
            rt += '_' + text[ pi:pn]
            pi = pn+1
            pn = text.find( ' ', pi, pend)
        if pn == -1:
            if pi != pend:
                rt += '_' + text[ pi:pend]
        else:
            rt = " \"" + text[ pstart:pend] + "\""
    else:
        rt = " \"" + text[ pstart:pend] + "\""
    return rt

def tag_phrases( text):
    ti = text.find( '"')
    if ti == -1:
        return text
    rt = text[ :ti]
    while ti != -1:
        pstart = ti+1
        while text[pstart] == ' ':
            pstart += 1
        tn = text.find( '"', pstart, max( len(text),80))
        if tn != -1:
            pi = text.find( ':', pstart, tn)
            while pi != -1:
                rt += " " + get_phrase_id( text, pstart, pi)
                pstart = pi+1
                pi = text.find( ':', pstart, tn)
            rt += " " + get_phrase_id( text, pstart, tn)
        else:
            rt += text[ ti:]
            break
        ti = tn + 1
        tn = text.find( '"', ti)
        if tn == -1:
            rt += ' ' + text[ ti:]
            break
        else:
            rt += ' ' + text[ ti:tn]
        ti = tn
    return rt

def parse_sequence_pattern( sequence):
    rt = []
    for sq in sequence:
        if sq == '*':
            rt.append( [None,None])
        else:
            tk = parse_tokendef( sq)
            if tk == None:
                rt.append( [None,None])
            else:
                rt.append( tk)
    return rt

cmd = None
if len( sys.argv) > 1:
    cmd = sys.argv[1]

def read_dict( dictfile):
    dict = {}
    for line in codecs.open( dictfile, "r", encoding='utf-8'):
        sline = line.decode('utf-8').strip()
        if sline:
            if sline.find(' ') == -1:
                print >> sys.stderr, "IGNORE [%s]" % sline
            else:
                key,cnt = sline.split()
                if key in dict:
                    dict[ key] = dict[ key] + int(cnt)
                else:
                    dict[ key] = int(cnt)
    return dict

def read_titles( titlefile):
    dict = {}
    for line in codecs.open( titlefile, "r", encoding='utf-8'):
        key = line.decode('utf-8').strip()
        if key:
            dict[ key] = 1
    return dict

def print_usage():
    print "usage strusnlp.py <command> ..."
    print "<command>:"
    print "    makedict <infile> [<mincnt>]:"
    print "        Create a dictionary from the NLP output in <infile>."
    print "        Print only elements with a higher or equal count than <mincnt>."
    print "        Default for <mincnt> is 1"
    print "    nlp <infile>:"
    print "        Do NLP with the Python NLTK library on <infile>."
    print "        Output tokens of the form \"<type>#<value>\", e.g. \"DT#the\"."
    print "    joindict { <dictfile> }:"
    print "        Join several dictionaries passed as arguments"
    print "    splitdict <dictfile> [<titlesfile>]:"
    print "        Try to split entries in the dictionary."
    print "        Use the term occurrence statistics to make decisions"
    print "    splittest <dictfile> <titlesfile> <word>:"
    print "        Try to split the term <word> (for testing)."
    print "        Use the term occurrence statistics to make decisions"
    print "    seldict <dictfile> [<mincnt>]:"
    print "        Select the dictionary elements with a higher or equal count than <mincnt>."
    print "        Default for <mincnt> is 1"
    print "    concat <infile> [<dictfile>] [<titlesfile>]:"
    print "        Produce phrases from NLP output and with help of a dictionary."
    print "    markseq <infile> <sequence...> <marker>:"
    print "        Marks a sequence of types in the NLP dump with a start string <marker>."

if cmd == None or cmd == '-h' or cmd == '--help':
    print_usage()

elif cmd == "makedict":
    infile = sys.argv[2]
    linecnt = 0
    for lineitr in codecs.open( infile, "r", encoding='utf-8'):
        line = lineitr.decode('utf-8').strip()
        if line:
            fill_dict( line)
            linecnt += 1
            if linecnt % 10000 == 0:
                print >> sys.stderr, "processed %u lines" %linecnt
    mincnt = 1
    if len(sys.argv) > 3:
        mincnt = int(sys.argv[3])
    for key,value in nnp_dict.iteritems():
        if value >= mincnt:
            print "%s %u" % (key,value)

elif cmd == "nlp":
    infile = sys.argv[2]
    linecnt = 0
    for lineitr in codecs.open( infile, "r", encoding='utf-8'):
        line = lineitr.decode('utf-8').strip()
        if line:
            print "%s" % tag_NLP( tag_phrases( line))
            linecnt += 1
            if linecnt % 10000 == 0:
                print >> sys.stderr, "processed %u lines" %linecnt

elif cmd == "joindict":
    infile = []
    for dictfile in sys.argv[2:]:
        for lineitr in codecs.open( dictfile, "r", encoding='utf-8'):
            line = lineitr.decode('utf-8').strip()
            if line:
                if line.find(' ') != -1:
                    key,cnt = line.split()
                    if key in nnp_dict:
                        nnp_dict[ key] = nnp_dict[ key] + int(cnt)
                    else:
                        nnp_dict[ key] = int(cnt)
    for key,value in nnp_dict.iteritems():
        print "%s %u" % (key,value)

elif cmd == "splitdict" or cmd == "splittest":
    nnp_dict = read_dict( sys.argv[2])
    fill_nnp_split_dict()
    if len(sys.argv) > 3:
        title_dict = read_titles( sys.argv[3])
    if cmd == "splitdict":
        new_dict = {}
        for key,value in nnp_dict.iteritems():
            for word in nnp_split_words( key, False):
                if word in new_dict:
                    new_dict[ word] += value
                else:
                    new_dict[ word] = value
        for key,value in new_dict.iteritems():
            print "%s %u" % (key,value)
    else: #cmd == "splittest"
        for key in sys.argv[4:]:
            for word in nnp_split_words( key.decode('utf-8'), True):
                print "%s" % word

elif cmd == "seldict":
    nnp_dict = read_dict( sys.argv[2])
    mincnt = 50
    if len(sys.argv) > 3:
        mincnt = int(sys.argv[3])
    for key,value in nnp_dict.iteritems():
        if value >= mincnt:
            print "%s %u" % (key,value)

elif cmd == "concat":
    infile = sys.argv[2]
    if len(sys.argv) > 3:
        nnp_dict = read_dict( sys.argv[3])
        fill_nnp_split_dict()
    if len(sys.argv) > 4:
        title_dict = read_titles( sys.argv[4])
    linecnt = 0
    for line in codecs.open( infile, "r", encoding='utf-8'):
        print concat_phrases( line.decode('utf-8'))
        linecnt += 1
        if linecnt % 10000 == 0:
            print >> sys.stderr, "processed %u lines" %linecnt

elif cmd == "markseq":
    infile = sys.argv[2]
    if len(sys.argv) < 5:
        print >> sys.stderr, "too few arguments for markseq, at lease %u expected" % 4
        raise
    sequence = parse_sequence_pattern( sys.argv[ 3:-1])
    marker = sys.argv[ -1]
    linecnt = 0
    for line in codecs.open( infile, "r", encoding='utf-8'):
        print mark_phrases( line.decode('utf-8'), sequence, marker)
        linecnt += 1
        if linecnt % 10000 == 0:
            print >> sys.stderr, "processed %u lines" %linecnt

elif cmd == "getseq":
    infile = sys.argv[2]
    if len(sys.argv) < 4:
        print >> sys.stderr, "too few arguments for getseq, at lease %u expected" % 3
        raise
    sequence = parse_sequence_pattern( sys.argv[ 3:])
    linecnt = 0
    for line in codecs.open( infile, "r", encoding='utf-8'):
        for phrase in get_phrases( line.decode('utf-8'), sequence):
            print "%s" % phrase
        linecnt += 1
        if linecnt % 10000 == 0:
            print >> sys.stderr, "processed %u lines" %linecnt

else:
    print >> sys.stderr, "unknown command"
    print_usage()
    raise


