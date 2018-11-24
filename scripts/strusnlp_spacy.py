#!/usr/bin/python3
#
import nltk
from pprint import pprint
import sys
import math
import re
import time
import getopt
import os
import stat
import subprocess
from recordtype import recordtype
from copy import deepcopy
import spacy
import en_core_web_sm
import time

spacy_nlp = en_core_web_sm.load()
NlpToken = recordtype('NlpToken', ['strustag','strusrole','nlptag','nlprole', 'value', 'alphavalue', 'ref'])
Subject = recordtype('Subject', ['sex','strustag','value','sentidx'])

def mapTagValue( tagname):
    if tagname == "." or tagname == ";":
        return "T!" # [delimiter] sentence delimiter
    if tagname == "-LRB-" or tagname == "-RRB-" or tagname == "," or tagname == ":" or tagname == "#":
        return "P!" # [part delimiter] delimiter
    if tagname == "CC":
        return "" # coordinating conjunction
    if tagname == "CD":
        return "C" # cardinal number
    if tagname == "DT":
        return "" # [determiner] determiner
    if tagname == "EX":
        return "X" # existential there
    if tagname == "FW":
        return "N" # foreign word
    if tagname == "IN":
        return "" # [determiner] preposition or subordinating conjunction
    if tagname == "JJ" or tagname == "JJR" or tagname == "JJS":
        return "A" # [adjective/adverb] adjective
    if tagname == "LS":
        return "T" # [delimiter] list item marker
    if tagname == "MD":
        return "m" # modal
    if tagname == "TO":
        return "" # to
    if tagname == "NN" or tagname == "NNS":
        return "N" # noun singular or plural
    if tagname == "NNP" or tagname == "NNPS":
        return "E" # [entity] proper noun, singular or plural
    if tagname == "ADD":
        return "U" # [URI]
    if tagname == "PDT":
        return "m" # [modal] pre determiner
    if tagname == "POS":
        return "" # [possesive] possessive ending
    if tagname == "PRP":
        return "R" # [entity] personal pronoun
    if tagname == "PRP$":
        return "R" # [entity] possesive pronoun
    if tagname == "RB" or tagname == "RBR" or tagname == "RBS":
        return "A" # [adjective/adverb] adverb or comparative or superlative
    if tagname == "RP":
        return "_" # [particle] particle
    if tagname == "HYPH":
        return "" # empty
    if tagname == "AFX":
        return "A" # empty
    if tagname == "$" or tagname == "S" or tagname == "SBAR" or tagname == "SBARQ" or tagname == "SINV" or tagname == "SQ":
        return "" # [] declarative clause, question, etc.
    if tagname == "SYM":
        return "N" # symbol
    if tagname == "VBD" or tagname == "VBG" or tagname == "VBN" or tagname == "VBP" or tagname == "VBZ" or tagname == "VB":
        return "V" # Verb, past tense or gerund or present participle or past participle or singular present
    if tagname == "WDT" or tagname == "WP" or tagname == "WP$" or tagname == "WRB":
        return "W" # determiner
    if tagname == "UH":
        return "" # exclamation
    if tagname == "''":
        return "" # empty
    if tagname == "``":
        return "" # empty
    if tagname == "NFP":
        return "" # empty
    if tagname == "":
        return "" # empty
    if tagname == "XX":
        return "" # empty
    if tagname == "_SP" or tagname == "NFP":
        return "" # empty
    sys.stderr.write( "unknown NLP tag %s\n" % tagname)
    return "?"

tgmaplist = [
 "CC", "CD", "DT", "EX", "FW", "IN", "JJ", "JJR", "JJS", "LS", "MD", "TO",
 "NNS", "NN", "NNP", "NNPS", "PDT", "POS", "PRP", "PRP$", "RB", "RBR", "RBS", "RP", "HYPH", "NFP", "AFX",
 "S", "SBAR", "SBARQ", "SINV", "SQ", "SYM", "VBD", "VBG", "VBN", "VBP", "VBZ", "VB", "ADD",
 "WDT", "WP", "WP$", "WRB", ".", ";", ":", ",", "-LRB-", "-RRB-", "$", "#" , "UH", "", "''", "``", "XX", "_SP", "NFP"
]

tgmap = {key: mapTagValue(key) for key in tgmaplist}

def mapTag( tagname):
    global tgmap
    if tagname in tgmap:
        return tgmap[ tagname]
    else:
        sys.stderr.write( "unknown NLP tag %s\n" % tagname)
        return "?"

def unifyType( type):
    if type == "NNPS":
        return "NNP"
    if type == "NNS":
        return "NN"
    return type

def splitAbbrev( name):
    rt = []
    while name.find('.') >= 0:
        pi = name.index('.');
        if pi > 0:
            rt.append( name[ :pi+1])
        name = name[ pi+1:]
    if name:
        rt.append( name)
    return rt

def getPrpSex( prp):
    if isinstance( prp, list):
        if len(prp) != 1:
            return "P"
        else:
            prp = prp[0]
    prp = prp.lower()
    if prp == "he" or prp == "his":
        return "M"
    if prp == "she" or prp == "her":
        return "W"
    if prp == "it" or prp == "its":
        return "N"
    if prp == "they" or prp == "their":
        return "P"
    return None

def trimApos( nam):
    quotlist = ["'","’","`","\"","!","—"]
    nidx = 0
    eidx = len(nam)
    changed = True
    while changed:
        changed = False
        for quot in quotlist:
            while eidx > len(quot)+nidx and nam[nidx:nidx+len(quot)] == quot:
                nidx += len(quot)
                changed = True
            while eidx > nidx and nam[ eidx-len(quot):eidx] == quot:
                eidx -= len(quot)
                changed = True
    if eidx > nidx:
        return nam[nidx:eidx]
    else:
        return ""

def matchName( obj, candidate, relaxed):
    cd = deepcopy(candidate)
    if not obj:
        return False
    for nam in obj:
        nam = trimApos( nam)
        if relaxed or (nam and nam[ -1:] in ['.']):
            if not nam or nam in [".", "..", "..."]:
                continue
            prefix = nam
            if nam[ -1:] == '.':
                prefix = nam[ :-1]
            elif relaxed:
                prefix = nam[0:2]
            else:
                prefix = nam[ :-1]
            found = False
            for eidx,elem in enumerate(cd):
                elemnam = trimApos( elem)
                if len(elemnam) > len(prefix) and elemnam[ :len(prefix)] == prefix:
                    del cd[ eidx]
                    found = True
                    break
            if not found:
                return False
        else:
            found = False
            for eidx,elem in enumerate(cd): 
                if trimApos( nam) == trimApos( elem):
                    del cd[ eidx]
                    found = True
                    break
            if not found:
                return False
    return True

def isEqualName( obj, candidate):
    if not obj or len(obj) != len(candidate):
        return False
    cd = deepcopy(candidate)
    for nam in obj:
        found = False
        for eidx,elem in enumerate(cd):
            if trimApos( nam) == trimApos( elem):
                del cd[ eidx]
                found = True
                break
        if not found:
            return False
    return True

def isUpperCaseName( sq):
    for elem in sq:
        if not elem or not elem[ 0].isupper():
            return False
    return True

def getMultipartName( tokens, tidx):
    rt = [tokens[ tidx].alphavalue]
    ti = tidx + 1
    while ti < len(tokens) and tokens[ ti].nlptag == '_':
        trnam = tokens[ ti].alphavalue
        if trnam:
            rt.append( trnam)
        ti += 1
    return rt

def getMultipartNameStr( tokens, tidx):
    rt = tokens[tidx].value
    ti = tidx + 1
    while ti < len(tokens) and tokens[ ti].nlptag == '_':
        trnam = tokens[ ti].alphavalue
        if trnam:
            rt += " " + trnam
        ti += 1
    return rt

def skipMultipartName( tokens, tidx):
    ti = tidx + 1
    while ti < len(tokens) and tokens[ ti].nlptag == '_':
        ti += 1
    return ti

def getTitleSubject( title):
    if len(title) > 0 and title[0] == '(':
        title = title.translate( str.maketrans( "", "", "’'\"()?!/;:"))
    else:
        title = title.translate( str.maketrans( "", "", "’'\"?!/;:"))
    endtitle = title.find('(')
    if endtitle >= 0:
        titleparts = title[ :endtitle ].split(' ')
    else:
        titleparts = title.split(' ')
    rt = []
    for tp in titleparts:
        rt += splitAbbrev( tp)
    return rt

def getEntityElements( name):
    rt = []
    for nm in name:
        if nm:
            rt.append( nm)
    return rt

def getFirstKeyMap( map, getKeyElementsFunc):
    rt = {}
    for key in map:
        keyelements = getKeyElementsFunc( key)
        if len(keyelements) >= 2:
            fkey = keyelements[0]
            if fkey in rt:
                rt[ fkey].append( keyelements)
            else:
                rt[ fkey] = [keyelements]
    return rt

def getLastKeyMap( map, getKeyElementsFunc):
    rt = {}
    for key in map:
        keyelements = getKeyElementsFunc( key)
        if len(keyelements) >= 1:
            fkey = keyelements[-1]
            if fkey in rt:
                rt[ fkey].append( keyelements)
            else:
                rt[ fkey] = [keyelements]
    return rt

def normalizedCountTokenValue( tok):
    return tok.lower().translate( str.maketrans( "", "", "'\"()?!/;:\’"))

def countTokens( toknctmap, toklist):
    for tok in toklist:
        tv = normalizedCountTokenValue( tok.value)
        if tv:
            if tv in toknctmap:
                toknctmap[ tv] += 1
            else:
                toknctmap[ tv] = 1

def calcTokenWeight( N, tf):
    return 1.0 / math.log( (N - 0.5) / (tf + 0.5))

def tokenCountToWeightMap( tokCntMap, tokCntTotal):
    rt = {}
    maxcnt = 1
    maxkey = ""
    for key,cnt in tokCntMap.items():
        if int(cnt) > maxcnt:
            maxcnt = int(cnt)
            maxkey = key
    wBase = calcTokenWeight( tokCntTotal, maxcnt)
    mBase = calcTokenWeight( tokCntTotal, 1)
    for key,cnt in tokCntMap.items():
        rt[ key] = (calcTokenWeight( tokCntTotal, cnt) - mBase) / (wBase - mBase)
    return rt

def assignRef( tokens, tidx, ref):
    ridx = 0
    while ridx < len(ref) and tokens[ tidx+ridx].value == ref[ ridx]:
        ridx += 1
    if ridx != len(ref):
        tokens[ tidx].ref = ref

def tagLongestTitleMatch( tokens, tidx, titlesubject, relaxed):
    name = getMultipartName( tokens, tidx)
    isSubject = (tokens[ tidx].nlprole[:5] == "nsubj")
    matches = False
    if relaxed:
        mt = matchName( titlesubject, name, True)
    if not mt:
        mt = matchName( name, titlesubject, False)
    if mt:
        btidx = tidx - 1
        while btidx > 0:
            name.insert( 0, tokens[ btidx].value)
            if relaxed:
                if not (matchName( titlesubject, name, True) or matchName( name, titlesubject, False)):
                    break
            else:
                if not matchName( name, titlesubject, False):
                    break
            if tokens[ btidx].nlprole[:5] == "nsubj":
                isSubject = True
            btidx -= 1
        btidx += 1
        tidx += 1
        while tidx < len(tokens):
            name.append( tokens[ tidx].value)
            if not matchName( [tokens[ tidx].value], titlesubject, False):
                break
            if relaxed:
                if not (matchName( titlesubject, name, True) or matchName( name, titlesubject, False)):
                    break
            else:
                if not matchName( name, titlesubject, False):
                    break
            if tokens[ tidx].nlprole[:5] == "nsubj":
                isSubject = True
            tidx += 1
        if isSubject:
            tokens[ btidx].strusrole = 'S'
        tokens[ btidx].strustag = 'E'
        tokens[ btidx].nlptag = 'NNP'
        tokens[ btidx].ref = titlesubject
        btidx += 1
        while btidx < tidx:
            if isSubject:
                tokens[ btidx].strusrole = '_'
            tokens[ btidx].strustag = '_'
            tokens[ btidx].nlptag = '_'
            tokens[ btidx].ref = None
            btidx += 1
        return btidx
    else:
        return -1

def sentenceHasVerb( tokens):
    hasVerb = False
    for elem in tokens:
        if elem.nlptag and elem.nlptag[0] == 'V':
            hasVerb = True
    return hasVerb

def sentenceIsTitle( tokens):
    for elem in tokens:
        if elem.nlptag:
            if elem.nlptag == "HYPH":
                continue
            if elem.strustag == '_':
                continue
            elif elem.nlptag[0] in ['J','N','P','R','W','M','C','F','T','D','I']:
                if elem.value and elem.value[0].isupper():
                    continue
        if elem.nlprole == "punct":
            continue
        return False
    return True

def getMapBestWeight( firstNameMap, name2weightMap, name):
    bestCd = None
    bestWeight = 0
    cdList = []
    for nm in name:
        if nm in firstNameMap:
            cdList.extend( firstNameMap[ nm])
    for cd in cdList:
        weight = name2weightMap[ cd]
        thisCd = cd.split(' ')
        if len(name) == 1:
            if len(thisCd) >= 3:
                if len(thisCd) > 3 or thisCd[-1] not in name:
                    continue
        elif len(name) == 2:
            if len(thisCd) >= 4:
                if len(thisCd) > 4 or thisCd[-1] not in name:
                    continue
        if matchName( name, thisCd, False):
            if weight > bestWeight or (weight == bestWeight and (not bestCd or len(bestCd) < len(thisCd))):
                bestCd = thisCd
                bestWeight = weight
    return bestCd,bestWeight

def removeFirstNameMap( map, namekey):
    for nm in namekey.split(' '):
        map[ nm].remove( namekey)

def defineFirstNameMap( map, namekey):
    for nm in namekey.split(' '):
        if nm in map:
            map[ nm].append( namekey)
        else:
            map[ nm] = [namekey]

def skipToOwnPrp( tokens, tidx):
    nidx = tidx
    while nidx<len(tokens) and tokens[nidx].nlptag[:2] == "RB":
        nidx += 1
    if nidx<len(tokens) and tokens[nidx].nlptag in ["VBZ","VBD"]:
        nidx += 1
    if nidx<len(tokens) and tokens[nidx].nlptag == "VBN":
        nidx += 1
    if nidx<len(tokens) and tokens[nidx].nlptag in ["IN","CC"]:
        nidx += 1
        if nidx<len(tokens) and tokens[nidx].nlptag[:3] == "PRP":
            return nidx
    return -1

def tagSentenceStrusTags( tokens):
    prev = ""
    mapprev = ""
    for eidx,elem in enumerate(tokens):
        if elem.nlprole == "none" or elem.strustag == '_':
            prev = ""
            mapprev = ""
            continue
        type = elem.nlptag
        utype = unifyType( type)
        maptype = mapTag( type)
        if type == 'PRP' or type == 'PRP$':
            if not getPrpSex( elem.value):
                maptype = ""
        if prev and utype == prev and prev[0] in ['N','E','V']:
            type = "_"
            maptype = "_";
        elif maptype and maptype == mapprev and maptype[-1:] == '!':
            maptype = '_'
        else:
            prev = utype
            mapprev = maptype
        elem.strustag = maptype
        elem.nlptag = type

def tagEntitySequenceToken( elem, eidx):
    if elem.nlprole[:5] == "nsubj":
        role = elem.nlprole
    else:
        role = "none"
    if eidx == 0:
        elem.strustag = 'E'
        elem.nlptag = 'NNP'
        elem.nlprole = role
    else:
        elem.strustag = '_'
        elem.nlptag = '_'
        elem.nlprole = role

def tagEntitySequenceTokenShift( toklist, tokidx, eidx):
    if tokidx+1 < len(toklist):
        if toklist[ tokidx+1].nlptag == '_':
            if toklist[ tokidx].nlptag == '_':
                toklist[ tokidx+1].nlptag = ""
            else:
                toklist[ tokidx+1].nlptag = toklist[ tokidx].nlptag
                if toklist[ tokidx].nlprole[:5] == "nsubj":
                    toklist[ tokidx+1].nlprole = toklist[ tokidx].nlprole
                    toklist[ tokidx].nlprole = "none"
        if toklist[ tokidx+1].strustag == '_':
            if toklist[ tokidx].strustag == '_':
                toklist[ tokidx+1].strustag = ""
            else:
                toklist[ tokidx+1].strustag = toklist[ tokidx].strustag
    tagEntitySequenceToken( toklist[tokidx], eidx)

def tagEntitySequenceStrusTags( tokens, startidx, endidx):
    eidx = startidx
    ofs = 0
    while eidx < endidx:
        if tokens[eidx].nlprole == "none" or  tokens[eidx].strustag == '_':
            ofs = 0
            eidx += 1
        elif tokens[eidx].nlprole == "punct" or tokens[eidx].nlptag == "HYPH":
            ofs = 0
            eidx += 1
        else:
            nidx = eidx
            doSkip = False
            while nidx < endidx and tokens[nidx].nlprole != "punct" and tokens[nidx].nlptag != "HYPH":
                if tokens[nidx].nlprole == "none" or  tokens[nidx].strustag == '_':
                    doSkip = True
                    break
                nidx += 1
            if not doSkip:
                while eidx < nidx:
                    tagEntitySequenceTokenShift( tokens, eidx, ofs)
                    ofs += 1
                    eidx += 1
                ofs = 0
            else:
                eidx = nidx

def tagEntitySequenceStrusTagsInBrackets( tokens):
    eidx = 0
    while eidx < len(tokens):
        eb = None
        if tokens[eidx].nlptag == "-LRB-" and tokens[eidx].value in ['(','[']:
            eb = tokens[eidx].value
        elif tokens[eidx].nlptag == "HYPH" and tokens[eidx].value == "–":
            eb = "–"
        elif tokens[eidx].value == ";":
            eb = ';'
        if eb:
            eidx += 1
            startidx = eidx
            while eidx < len(tokens) and not tokens[eidx].value.find(eb) >= 0:
                eidx += 1
            if eidx < len(tokens):
                endidx = eidx
                if sentenceIsTitle( tokens[ startidx:endidx]):
                    tagEntitySequenceStrusTags( tokens, startidx, endidx)
            else:
                eidx = startidx
        else:
            eidx += 1

def tagSentenceLinkReferences( tokens, firstKeyLinkListMap):
    tidx = 0
    while tidx < len(tokens):
        if tokens[tidx].value in firstKeyLinkListMap:
            candidateList = firstKeyLinkListMap[ tokens[tidx].value]
            bestIdx = -1
            bestLen = 0
            for cidx,cd in enumerate( candidateList):
                sidx = 0
                while sidx < len(cd) and tidx+sidx < len(tokens):
                    if tokens[tidx+sidx].value != cd[sidx] or tokens[tidx+sidx].nlprole == 'none' or tokens[tidx+sidx].nlptag in ["VBZ","VBD"]:
                        break
                    sidx += 1
                if sidx == len(cd):
                    if len(cd) > bestLen:
                        bestLen = len(cd)
                        bestIdx = cidx
            if bestIdx >= 0:
                sidx = 0
                while sidx < bestLen:
                    tagEntitySequenceTokenShift( tokens, tidx+sidx, sidx)
                    sidx += 1
                tidx += sidx
            else:
                tidx += 1
        else:
            tidx += 1

def matchTokenNameReference( tokens, tidx, nam):
    sidx = 0
    while sidx < len(nam) and tidx+sidx < len(tokens):
        if tokens[tidx+sidx].alphavalue != nam[sidx] or tokens[tidx+sidx].nlprole == 'none':
            break
        sidx += 1
    if sidx == len(nam):
        return True
    else:
        return False

def tagTokenNameReference( tokens, tidx, size):
    sidx = 0
    while sidx < size:
        tagEntitySequenceTokenShift( tokens, tidx+sidx, sidx)
        sidx += 1

def tagSentenceNameReferences( tokens, nam):
    if nam:
        tidx = 0
        while tidx < len(tokens):
            if matchTokenNameReference( tokens, tidx, nam):
                tagTokenNameReference( tokens, tidx, len(nam))
                tidx += len(nam)
            else:
                tidx += 1

def tagSentenceNameMapReferences( tokens, namemap):
    tidx = 0
    while tidx < len(tokens):
        tokval = tokens[tidx].alphavalue
        if tokval in namemap:
            bestIdx = -1
            bestLen = 0
            cdlist = namemap[ tokval]
            for cdIdx,cd in enumerate(cdlist):
                if len(cd) > bestLen and matchTokenNameReference( tokens, tidx, cd):
                    bestLen = len(cd)
                    bestIdx = cdIdx
            if bestIdx >= 0:
                tagTokenNameReference( tokens, tidx, bestLen)
                tidx += bestLen
            else:
                tidx += 1
        else:
            tidx += 1

# UNUSED
def tagSentenceSingleLastNameReferences( tokens, entityLastKeyMap):
    tidx = 0
    while tidx < len(tokens):
        if tokens[tidx].nlptag[:1] == 'N':
            if not tokens[ tidx].ref and tokens[ tidx].value and tokens[ tidx].value[0].isupper():
                if tidx+1 == len(tokens) or not tokens[tidx+1].nlptag[:1] in ['N','_']:
                    tokval = tokens[tidx].alphavalue
                    if tokval in entityLastKeyMap:
                        bestIdx = -1
                        bestLen = -1
                        cdlist = entityLastKeyMap[ tokval]
                        for cdIdx,cd in enumerate(cdlist):
                            if bestLen == 0 or len(cd) > bestLen:
                                bestLen = len(cd)
                                bestIdx = cdIdx
                        if bestIdx >= 0 and bestLen > 1 and bestLen <= 3:
                            ref = cdlist[ bestIdx]
                            if isUpperCaseName( ref):
                                tokens[ tidx].ref = ref
            while tidx < len(tokens) and tokens[tidx].nlptag[:1] in ['N','_']:
                tidx += 1
        else:
            tidx += 1

def tagSentenceSubjects( tokens):
    bracketCnt = 0
    for eidx,elem in enumerate( tokens):
        name = None
        if elem.nlptag == "-LRB-" and elem.value == '(':
            bracketCnt += 1
        elif elem.nlptag == "-RRB-" and elem.value == ')' and bracketCnt > 0:
            bracketCnt -= 1
        elif bracketCnt > 0:
            pass
        elif elem.nlptag == 'PRP' and elem.nlprole[:5] == "nsubj" and getPrpSex(elem.value):
            elem.strusrole = "S"
        elif elem.nlptag[:2] == 'NN':
            if elem.nlprole == "conj" or elem.nlprole == "compound":
                ei = eidx-1
                while ei >= 0 and tokens[ei].nlprole == "compound":
                    ei -= 1
                while ei >= 0 and tokens[ei].nlprole == "amod":
                    ei -= 1
                while ei >= 0 and tokens[ei].nlprole == "cc":
                    ei -= 1
                if ei >= 0 and tokens[ei].nlprole[:5] == "nsubj":
                    elem.strusrole = "S"
            elif elem.nlprole[:5] == "nsubj":
                elem.strusrole = "S"
            if not elem.strusrole:
                ei = eidx+1
                while ei < len(tokens) and tokens[ei].nlptag == '_':
                    if tokens[ei].nlprole[:5] == "nsubj":
                        elem.strusrole = "S"
                    ei += 1
                if not elem.strusrole and eidx == 0:
                    while ei < len(tokens):
                       if tokens[ei].nlptag == "-LRB-" and tokens[ei].value == '(':
                           while ei < len(tokens) and not (tokens[ei].nlptag == "-RRB-" and tokens[ei].value == ')'):
                               ei += 1
                           ei += 1
                       elif tokens[ei].nlptag == "-LRB-" and tokens[ei].value == '[':
                           while ei < len(tokens) and not (tokens[ei].nlptag == "-RRB-" and tokens[ei].value == ']'):
                               ei += 1
                           ei += 1
                       elif tokens[ei].nlptag == "," and tokens[ei].value == ',':
                           ei += 1
                           if tokens[ei].nlptag == "RB":
                               ei += 1
                           if tokens[ei].nlptag == "VBN":
                               ei += 1
                               while ei < len(tokens) and not (tokens[ei].nlptag == "," and tokens[ei].value == ','):
                                   ei += 1
                               ei += 1
                           else:
                               break
                       elif tokens[ei].nlprole == "ROOT" and (tokens[ei].nlptag == "VBZ" or tokens[ei].nlptag == "VBD"):
                           elem.strusrole = "S"
                           break
                       else:
                           break
        if elem.strusrole == "S":
            ei = eidx+1
            while ei < len(tokens) and tokens[ei].nlptag == '_':
                tokens[ei].strusrole = "_"
                ei += 1

def weightIncrOnNounMatch( weight):
    rt = weight + weight / 3
    if weight >= 3.0:
        rt += weight / 3
        if weight >= 6.0:
            rt += weight / 3
            if weight >= 9.0:
                rt += math.sqrt( weight)
    return rt

def tagSentenceCompleteNounReferences( tokens, titlesubject, bestTitleMatches, nounCandidateKeyMap, nounCandidates, tokCntWeightMap, sentenceIdx):
    usedEntities = []
    eidx = 0
    ref = None
    while eidx < len( tokens):
        if tokens[ eidx].nlptag[:2] == 'NN':
            ref = tokens[ eidx].ref
            isEntity = False
            if tokens[ eidx].nlptag[:3] == 'NNP':
                isEntity = True
            elif tokens[ eidx].nlptag[:2] == 'NN' and tokens[ eidx].value[0].isupper():
                isEntity = True
            if sentenceIdx == 0:
                nidx = tagLongestTitleMatch( tokens, eidx, titlesubject, True)
                if nidx >= 0:
                    sentenceIdx = -1
                    tokens[ eidx].ref = titlesubject
                    ref = titlesubject
                    isEntity = True
            if isEntity and not ref:
                name = getMultipartName( tokens, eidx)
                for bt in bestTitleMatches:
                    if isEqualName( name, bt):
                        tokens[ eidx].ref = titlesubject
                        ref = titlesubject
                        isEntity = True
            if isEntity and not ref:
                item,weight = getMapBestWeight( nounCandidateKeyMap, nounCandidates, name)
                if item:
                    if len(name) == 1:
                        normname = normalizedCountTokenValue( name[0])
                        if normname in tokCntWeightMap:
                            nameweight = tokCntWeightMap[ normname]
                            if weight >= 0.1 + nameweight * 0.2:
                                assignRef( tokens, eidx, item)
                                ref = item
                    else:
                        assignRef( tokens, eidx, item)
                        ref = item
                if ref and tokens[ eidx].nlptag in ["NN","NNS"]:
                    tokens[ eidx].nlptag = "NNP"
                    if tokens[ eidx].strustag == "N":
                        tokens[ eidx].strustag = "E"
            if isEntity:
                key = ' '.join( ref or name)
                usedEntities.append( key)
            elif ref:
                tokens[ eidx].strustag = 'E'
                key = ' '.join( ref or name)
                usedEntities.append( key)
        eidx += 1
    for key in usedEntities:
        if key in nounCandidates:
            weight = weightIncrOnNounMatch( nounCandidates[ key] + 1)
        else:
            weight = (1/0.8)
            defineFirstNameMap( nounCandidateKeyMap, key)
        nounCandidates[ key] = weight
    expiredKeys = []
    for key,weight in nounCandidates.items():
        weight *= 0.8
        nounCandidates[ key] = weight
        if weight < 0.1:
            expiredKeys.append( key)
    for key in expiredKeys:
        del nounCandidates[ key]
        removeFirstNameMap( nounCandidateKeyMap, key)

# param sexSubjectMap: map sex:string -> Subject
def tagSentencePrpReferences( tokens, sentidx, sexSubjectMap, nnpSexMap):
    subjects = []
    eidx = 0
    while eidx < len( tokens):
        if tokens[ eidx].nlptag[:2] == 'NN' and tokens[ eidx].strusrole == "S":
            name = tokens[ eidx].ref or getMultipartName( tokens, eidx)
            namekey = ' '.join(name)
            sex = None
            if tokens[ eidx].nlptag == "NN":
                if tokens[ eidx].value in ["man","father","boy"]:
                    sex = "M"
                elif tokens[ eidx].value in ["woman","mother","girl"]:
                    sex = "W"
                else:
                    sex = "N"
            elif namekey in nnpSexMap:
                sex = nnpSexMap[ namekey]
            matchprev = False
            if subjects:
                for sb in subjects:
                    if sb.sex == sex and sb.strustag == tokens[ eidx].strustag and (matchName( name, sb.value, False) or matchName( sb.value, name, False)):
                        matchprev = True
            if not matchprev:
                subjects.append( Subject( sex, tokens[ eidx].strustag, name, sentidx))
            nidx = skipToOwnPrp( tokens, skipMultipartName( tokens, eidx))
            if nidx >= 0:
                eidx = nidx + 1

        elif tokens[ eidx].nlptag[:3] == 'PRP' and not tokens[ eidx].ref:
           sex = getPrpSex( tokens[ eidx].value)
           if sex:
               if sex in sexSubjectMap:
                   subj = sexSubjectMap[ sex]
                   if sentidx - subj.sentidx < 7:
                       subj.sentidx = sentidx
                       tokens[ eidx].ref = subj.value
        eidx += 1
    if subjects:
        if len(subjects) > 1:
           combined = []
           combinedtag = subjects[0].strustag
           si = 1
           while si < len(subjects):
               combined.append( ',')
               combined += subjects[ si].value
               if combinedtag != subjects[0].strustag:
                   combinedtag = None
                   break
               si += 1
           sexSubjectMap[ 'P'] = Subject( 'P', combinedtag, combined, sentidx)
        else:
           sb = subjects[0]
           sexSubjectMap[ sb.sex] = sb

def printSentence( sent, complete):
    rt = ""
    if complete:
        for elem in sent:
            sr = (elem.strusrole or "")
            nr = (elem.nlprole or "")
            st = (elem.strustag or "")
            nt = (elem.nlptag or "")
            vv = (elem.value or "")
            if elem.ref:
                rr = ' '.join( elem.ref)
            else:
                rr = ""
            rt += ("%s (%s)\t%s\t%s\t%s\t%s\n" % (sr,nr,st,nt,vv,rr))
    else:
        for elem in sent:
            st = (elem.strustag or "")
            vv = (elem.value or "")
            if elem.ref:
                rr = ' '.join( elem.ref)
            else:
                rr = ""
            rt += ("%s\t%s\t%s\n" % (st,vv,rr))
    return rt

# return Sentence[]
def getDocumentSentences( text, verbose):
    tokens = []
    sentences = []
    last_subjects = []
    tg = spacy_nlp( text)
    eb = None
    ebstk = []
    for node in tg:
        value = str(node).strip()
        if value == '(' and node.tag_ == '-LRB-':
            eb = ')'
            ebstk.append( eb)
        elif value == '[' and node.tag_ == '-LRB-':
            eb = ']'
            ebstk.append( eb)
        elif len(ebstk) > 0 and value.find(eb) >= 0:
            ebstk.pop()
            if len(ebstk) > 0:
                eb = ebstk[ -1]
            else:
                eb = None
        if value:
            if node.tag_[:3] == "NNP" and value.count('.') > 1:
                for abr in splitAbbrev( value):
                    tokens.append( NlpToken( None, None, node.tag_, node.dep_, abr, trimApos(abr), None))
            elif node.tag_ and node.tag_[0] in ["N","J","V","R"]:
                tokens.append( NlpToken( None, None, node.tag_, node.dep_, value, trimApos(value), None))
            else:
                tokens.append( NlpToken( None, None, node.tag_, node.dep_, value, value, None))
        if node.dep_ == "punct":
            if not eb:
                if value in [':',';','.']:
                    if verbose:
                        print( "* Sentence %s" % ' '.join( [tk.value for tk in tokens]))
                    sentences.append( tokens)
                    tokens = []
            else:
                if value == '.':
                    if verbose:
                        print( "* Sentence %s" % ' '.join( [tk.value for tk in tokens]))
                    sentences.append( tokens)
                    tokens = []
                    ebstk = []
                    eb = None
    if tokens:
        sentences.append( tokens)
    return sentences

def getDocumentNlpTokCountMap( sentences, elements):
    rt = {}
    for sent in sentences:
        for tidx,tok in enumerate(sent):
            if tok.nlptag in elements:
                name = tok.ref or getMultipartName( sent, tidx)
                key = ' '.join( name)
                if key in rt:
                    rt[ key] += 1
                else:
                    rt[ key] = 1
    return rt

def splitNlpTokCountMap( usageMap):
    accMap = {}
    for key,usage in usageMap.items():
        accMap[ key] = usage
        eidx = key.rfind(' ', 0, len(key))
        while eidx >= 0:
            skeys = [ key[ :eidx], key[ eidx+1:] ]
            for skey in skeys:
                if skey:
                    if skey in accMap:
                        accMap[ skey] += usage
                    else:
                        accMap[ skey] = usage
            eidx = key.rfind(' ', 0, eidx)
    done = False
    changeMap = {}
    while not done:
        for key in usageMap:
            bestidx = -1
            secidx = -1
            bestusage = accMap[ key]
            secusage = bestusage
            eidx = key.rfind(' ', 0, len(key))
            while eidx >= 0:
                if key[ :eidx] in usageMap and key[ eidx+1:] in usageMap:
                    usage = accMap[ key[ :eidx]]
                    if usage > bestusage:
                        secusage = bestusage
                        secidx = bestidx
                        bestusage = usage
                        bestidx = eidx
                    elif usage > secusage:
                        secusage = usage
                        secidx = eidx
                eidx = key.rfind(' ', 0, eidx)
            if bestidx >= 0 and bestusage > secusage * 10 and key[:bestidx] in usageMap:
                keyusage = usageMap[ key]
                w_1 = usageMap[ key[:bestidx]] + keyusage
                w_2 = keyusage
                if key[bestidx+1:] in usageMap:
                    w_2 += usageMap[ key[bestidx+1:]]
                changeMap[ key[bestidx+1:]] = w_2
                changeMap[ key[:bestidx]] = w_1
                changeMap[ key] = 0
        for key,value in changeMap.items():
            if value:
                usageMap[ key] = changeMap[ key]
            else:
                del usageMap[ key]
        done = not changeMap
        changeMap = {}

def getMostUsedMultipartList( usageMap):
    selMap = {}
    usageCntMap = {}
    for key,usage in usageMap.items():
        if usage >= 3 and key.find(' ') >= 0:
             selMap[ key] = usage
             if usage in usageCntMap:
                 usageCntMap[ usage] += 1
             else:
                 usageCntMap[ usage] = 1
    minusage = 3
    maxlen = len(usageMap) / 5
    pp = 0
    for usage in sorted( usageCntMap.keys(), reverse=True):
        pp += usageCntMap[ usage]
        if pp > maxlen:
            minusage = usage
            break
    if minusage > 3:
        reduMap = {}
        for key,usage in selMap.items():
            if usage >= minusage:
                reduMap[ key] = usage
        return [elem.split(' ') for elem in reduMap.keys()]
    else:
        return [elem.split(' ') for elem in selMap.keys()]

def getBestTitleMatches( titlesubject, tokCountMap):
    maxTf = 0
    bestName = None
    for key,tf in tokCountMap.items():
        name = key.split(' ')
        if len(name) == 1 and len(titlesubject) > 3:
            continue
        if matchName( name, titlesubject, False):
            if tf > 20 and maxTf > 20 and len(name) < len(bestName) and tf * 2 > maxTf:
                maxTf = tf
                bestName = name
            elif tf > maxTf or (maxTf == tf and len(name) < len(bestName)):
                maxTf = tf
                bestName = name
    rt = []
    if bestName:
        rt.append( bestName)
        for key,tf in tokCountMap.items():
            name = key.split(' ')
            if matchName( name, titlesubject, False) and len(name) > len(bestName):
                rt.append( name)
    return rt

def addToNnpSexCountMap( map, nnp, prptok, weight):
    prpsex = getPrpSex( prptok)
    if prpsex:
        key = ' '.join( nnp)
        if prpsex and key in map:
            if prpsex in map[ key]:
                map[ key][ prpsex] += weight
            else:
                map[ key][ prpsex] = weight
        else:
            map[ key] = { prpsex : weight }

# param titlesubject string[]
# param sentences NlpToken[][]
# return map NNP -> sex count  {'M':0, 'W':0, 'N':0, 'P':0} list 
def getDocumentNnpSexCountMap( titlesubject, sentences):
    rt = {}
    sidx = 0
    lastSentSubjects = titlesubject
    for sent in sentences:
        sentSubjects = []
        sentPrpRefsMap = {}
        tidx = 0
        while tidx < len(sent):
            tok = sent[tidx]
            if tok.nlptag[0:3] == "NNP":
                name = tok.ref or getMultipartName( sent, tidx)
                nidx = skipToOwnPrp( sent, skipMultipartName( sent, tidx))
                if nidx >= 0:
                    if getPrpSex( tok.value) in ['W','M']:
                        weightfactor = 3.0
                    else:
                        weightfactor = 2.5
                    if name:
                        addToNnpSexCountMap( rt, name, sent[nidx].value, weightfactor)
                    tidx = nidx + 1
                if tok.strusrole == 'S':
                    if sentSubjects:
                        sentSubjects.append( ',')
                    if name:
                        sentSubjects.extend( name)
            elif tok.nlptag[0:3] == "PRP" and lastSentSubjects:
                if tok.strusrole == 'S':
                    sentPrpRefsMap[ tok.value] = 5.0
                    if sentSubjects:
                        sentSubjects.append( ',')
                    sentSubjects.extend( lastSentSubjects)
                elif tok.value not in sentPrpRefsMap:
                    sentPrpRefsMap[ tok.value] = 1.0
            tidx += 1
        normfactor = float(len(sentPrpRefsMap) ** 2)
        for prp,weight in sentPrpRefsMap.items():
            addToNnpSexCountMap( rt, lastSentSubjects, prp, float(weight) / normfactor)
        sentPrpRefsMap = {}
        lastSentSubjects = sentSubjects or []
    return rt

def getMaxSexCount( sexCountMap):
    rt = None
    max = 0
    values = []
    for sex,cnt in sexCountMap.items():
        values.append( cnt)
        if cnt > max:
            max = cnt
            rt = sex
    values.sort( reverse=True)
    if len(values) >= 2 and values[0] <= values[1] * 2 + 1.0:
        return None
    if len(values) == 1 and values[0] <= 5.0:
        return None
    return rt

def getDocumentNnpSexMap( map):
    rt = {}
    for nnp,sexCountMap in map.items():
        maxsex = getMaxSexCount( sexCountMap)
        if maxsex:
            if ',' in nnp and maxsex != 'P':
                continue
            rt[ nnp] = maxsex
    return rt

def tagDocument( title, text, entityMap, accuvar, verbose, complete):
    rt = ""
    titlesubject = getTitleSubject( title)
    entityFirstKeyMap = getFirstKeyMap( entityMap, getTitleSubject)
    entityLastKeyMap = getLastKeyMap( entityMap, getTitleSubject)
    tokCntMap = {}
    tokCntWeightMap = {}
    tokCntTotal = 0

    if verbose:
        print( "* Document title %s" % ' '.join(titlesubject))
    sentences = getDocumentSentences( text, verbose)
    start_time = time.time()
    
    for sent in sentences:
        countTokens( tokCntMap, sent)
        tokCntTotal += len(sent)
        tagSentenceLinkReferences( sent, entityFirstKeyMap)
        if sentenceHasVerb( sent):
            tagSentenceStrusTags( sent)
        if sentenceIsTitle( sent):
            tagEntitySequenceStrusTags( sent, 0, len(sent))
        tagEntitySequenceStrusTagsInBrackets( sent)
        tagSentenceNameReferences( sent, titlesubject)

    tokCntWeightMap = tokenCountToWeightMap( tokCntMap, tokCntTotal)
    countNnp = getDocumentNlpTokCountMap( sentences, ["NNP","NNPS"])
    bestTitleMatches = getBestTitleMatches( titlesubject, countNnp)
    if verbose:
        for bm in bestTitleMatches:
            print( "* Best title match %s" % ' '.join(bm))
    countNnp = getDocumentNlpTokCountMap( sentences, ["NNP","NNPS"])
    nounCandidateKeyMap = {}
    nounCandidates = {}
    titlekey = ' '.join( titlesubject)
    for sidx,sent in enumerate(sentences):
        if sentenceHasVerb( sent):
            tagSentenceSubjects( sent)
            tagSentenceCompleteNounReferences( sent, titlesubject, bestTitleMatches, nounCandidateKeyMap, nounCandidates, tokCntWeightMap, sidx)

    countNnp = getDocumentNlpTokCountMap( sentences, ["NNP","NNPS"])
    splitNlpTokCountMap( countNnp)
    mostUsedNnp = getMostUsedMultipartList( countNnp)
    mostUsedNnpMap = getFirstKeyMap( mostUsedNnp, getEntityElements)
    for sidx,sent in enumerate(sentences):
        tagSentenceNameMapReferences( sent, mostUsedNnpMap)
        # UNUSED tagSentenceSingleLastNameReferences( sent, entityLastKeyMap)

    nnpSexCountMap = getDocumentNnpSexCountMap( titlesubject, sentences)
    nnpSexMap = getDocumentNnpSexMap( nnpSexCountMap)
    sexSubjectMap = {}
    titlesex = None
    if titlekey in nnpSexMap:
        titlesex = nnpSexMap[ titlekey]
        sexSubjectMap[ titlesex] = Subject( titlesex, "E", titlesubject, 0)
    for sidx,sent in enumerate(sentences):
        if sentenceHasVerb( sent):
            tagSentencePrpReferences( sent, sidx, sexSubjectMap, nnpSexMap)
    for sent in sentences:
        rt += printSentence( sent, complete)
    if verbose:
        for subj,sexmap in nnpSexCountMap.items():
            print( "* Entity sex stats %s -> %s" % (subj, sexmap))
        for subj,sex in nnpSexMap.items():
            print( "* Entity sex %s -> %s" % (subj, sex))
        for subj,sc in nnpSexCountMap.items():
            print( "* Subject %s -> %s" % (subj, sc))
        for key in countNnp:
            print( "* Entity usage %s # %d" % (key, countNnp[ key]))
        for key,linklist in entityFirstKeyMap.items():
            for link in linklist:
                print( "* Link '%s'" % ' '.join(link))
        for key,weight in tokCntWeightMap.items():
            print( "* Token count %s # %.3f" % (key, weight))
        for nnp,nnplist in mostUsedNnpMap.items():
            for ne in nnplist:
                print( "* Most used NNP '%s'" % (' '.join(ne)))
    diff_time = time.time() - start_time
    if "timepost" in accuvar:
        accuvar["timepost"] += diff_time
    else:
        accuvar["timepost"] = diff_time
    return rt

def substFilename( filename):
    rt = ""
    fidx = 0
    while fidx < len(filename):
        if filename[ fidx] == '_':
            rt += ' '
            fidx += 1
        elif filename[ fidx] == '%':
            rt += chr( int( filename[fidx+1:fidx+3], 16))
            fidx += 3
        else:
            rt += filename[ fidx]
            fidx += 1
    return rt

def getTitleFromFileName( filename):
    base = os.path.basename( filename)
    fnam = os.path.splitext( base)[0]
    return substFilename( fnam)

doccnt = 0
statusLine = False
startTime = time.time()

def printStatusLine( nofdocs):
    global doccnt
    global startTime
    doccnt += nofdocs
    elapsedTime = (time.time() - startTime)
    timeString = "%.3f seconds" % elapsedTime
    sys.stderr.write( "\rprocessed %d documents  (%s)          " % (doccnt,timeString))

def printOutput( filename, result):
    global statusLine
    if statusLine:
        printStatusLine( 1)
    print( "#FILE#%s" % filename)
    print( "%s" % result)

def printUsage():
    print( "%s [-h|--help] [-V|--verbose] [-S|--status] [-C <chunksize> ]" % __file__)

def parseProgramArguments( argv):
    rt = {}
    try:
        opts, args = getopt.getopt( argv,"hTVKDSC:",["chunksize=","status","verbose","complete","duration","test"])
        for opt, arg in opts:
            if opt in ("-h", "--help"):
                printUsage()
                sys.exit()
            elif opt in ("-C", "--chunksize"):
                ai = int(arg)
                if ai <= 0:
                    raise RuntimeError( "positive integer expected")
                rt['C'] = ai
            elif opt in ("-V", "--verbose"):
                rt['V'] = True
            elif opt in ("-K", "--complete"):
                rt['K'] = True
            elif opt in ("-D", "--duration"):
                rt['D'] = True
            elif opt in ("-T", "--test"):
                rt['T'] = True
            elif opt in ("-S", "--status"):
                rt['S'] = True
        return rt
    except getopt.GetoptError:
        sys.stderr.write( "bad arguments\n")
        printUsage()
        sys.exit(2)

def processStdin( verbose, complete, duration):
    content = ""
    filename = ""
    entityMap = {}
    accuvar = {}
    entityReadState = False
    for line in sys.stdin:
        if len(line) > 6 and line[0:6] == '#FILE#':
            if filename:
                if content:
                    title = getTitleFromFileName( filename)
                    result = tagDocument( title, content, entityMap, accuvar, verbose, complete)
                    printOutput( filename, result)
                    content = ""
                else:
                    printOutput( filename, "")
            filename = line[6:].rstrip( "\r\n")
            entityReadState = True
            entityMap = {}
        else:
            if entityReadState and line[:2] == '##':
                entityMap[ line[2:].strip("\n\r\t ")] = True
            else:
                entityReadState = False
                content += line
    if filename:
        if content:
            title = getTitleFromFileName( filename)
            result = tagDocument( title, content, entityMap, accuvar, verbose, complete)
            printOutput( filename, result)
            content = ""
        else:
            printOutput( filename, "")
    if duration and "timepost" in accuvar:
        sys.stderr.write( "duration post processing: %.3f seconds\n" % accuvar[ "timepost"])

linebuf = None
def readChunkStdin( nofFiles):
    ii = 0
    content = ""
    global linebuf
    if linebuf:
        if len(linebuf) > 6 and linebuf[0:6] == '#FILE#':
            ii += 1
        else:
            raise RuntimeError("unexpected line in buffer")
        content += linebuf
        linebuf = None
    for line in sys.stdin:
        if len(line) > 6 and line[0:6] == '#FILE#':
            ii += 1
            if ii > nofFiles:
                linebuf = line
                return content, ii-1
        content += line
    return content, ii

def printContent( content):
    for line in content.split("\n", 20):
        if len(line) > 30:
            sys.stderr.write( "> %s\n" % line[:30])
        else:
            sys.stderr.write( "> %s\n" % line)

def printTestResult( text, result):
    if result:
        resultstr = "OK"
    else:
        resultstr = "FAILED"
    sys.stderr.write( "Run test %s %s\n" % (text,resultstr))

def diffTransformationResult( output, expected):
    return output == expected

def runTest():
    printTestResult( "MATCH 1", matchName( ["Giuliani"], ["Rudy","Giuliani"], False))
    printTestResult( "MATCH 2", matchName( ["Rudy","Giuliani"], ["Rudolph", "William", "Louis", "Giuliani"], True))
    printTestResult( "NOT MATCH 1", not matchName( ["Hugo","Chavez"], ["Hurto", "Castro"], False))
    printTestResult( "TRIM APOS 1", diffTransformationResult( trimApos( "calvin'"), "calvin"))
    printTestResult( "TRIM APOS 2", diffTransformationResult( trimApos( "calvin—"), "calvin"))
    printTestResult( "TRIM APOS 3", diffTransformationResult( trimApos( "—calvin"), "calvin"))
    printTestResult( "TRIM APOS 4", diffTransformationResult( trimApos( "—calvin—'"), "calvin"))

if __name__ == "__main__":
    argmap = parseProgramArguments( sys.argv[ 1:])
    chunkSize = 0
    verbose = False
    complete = False
    duration = False

    if 'V' in argmap:
        verbose = True
    if 'K' in argmap:
        complete = True
    if 'D' in argmap:
        duration = True
    if 'T' in argmap:
        runTest()
        exit( 0)
    if 'C' in argmap:
        chunkSize = argmap[ 'C']
    if 'S' in argmap:
        statusLine = True
    if chunkSize > 0:
        content, nofdocs = readChunkStdin( chunkSize)
        if verbose:
            printContent( content)
        while content:
            result = subprocess.check_output([ os.path.abspath(__file__)], input=bytearray(content,"UTF-8")).decode("utf-8")
            if statusLine:
                printStatusLine( nofdocs)
            print( result)
            content, nofdocs = readChunkStdin( chunkSize)
            if verbose:
                printContent( content)
    else:
        processStdin( verbose, complete, duration)








