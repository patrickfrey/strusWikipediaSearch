#!/usr/bin/python3
#
# This script converts a text dump with multiple files and writes a line for every part of speech (POS) item to stdout. 
# The output of this script can be merged with the original XML to get an XML with POS tagging.
#
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
Sentence = recordtype('Sentence', ['type','tokens'])

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
        return "T!" # [delimiter] list item marker
    if tagname == "MD":
        return "M" # modal
    if tagname == "TO":
        return "" # to
    if tagname == "NN" or tagname == "NNS":
        return "N" # noun singular or plural
    if tagname == "NNP" or tagname == "NNPS":
        return "E" # [entity] proper noun, singular or plural
    if tagname == "ADD":
        return "U" # [URI]
    if tagname == "PDT":
        return "M" # [modal] pre determiner
    if tagname == "POS":
        return "" # [possesive] possessive ending
    if tagname == "PRP":
        return "R" # [entity] personal pronoun
    if tagname == "PRP$":
        return "R" # [entity] possesive pronoun
    if tagname == "RB" or tagname == "RBR" or tagname == "RBS":
        return "A" # [adjective/adverb] adverb or comparative or superlative
    if tagname == "RP":
        return "A" # [particle] particle
    if tagname == "HYPH":
        return "" # empty
    if tagname == "AFX":
        return "A" # empty
    if tagname == "$" or tagname == "S" or tagname == "SBAR" or tagname == "SBARQ" or tagname == "SINV" or tagname == "SQ":
        return "" # [] declarative clause, question, etc.
    if tagname == "SYM":
        return "" # symbol
    if tagname == "VBD" or tagname == "VBG" or tagname == "VVG" or tagname == "VHG" or tagname == "VBN" or tagname == "VBP" or tagname == "VBZ" or tagname == "VB":
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
 "S", "SBAR", "SBARQ", "SINV", "SQ", "SYM", "VBD", "VBG", "VVG", "VHG", "VBN", "VBP", "VBZ", "VB", "ADD",
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
    sidx = 0
    nidx = name.find('.', sidx)
    while nidx >= 0:
        rt.append( name[ sidx:nidx+1].strip())
        sidx = nidx+1
        nidx = name.find('.', sidx)
    if sidx < len(name):
        rt.append( name[sidx:].strip())
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
    if prp == "he" or prp == "him":
        return "M"
    if prp == "she" or prp == "her":
        return "W"
    if prp == "it" or prp == "its":
        return "N"
    if prp == "they" or prp == "their":
        return "P"
    return None

def getNounSex( noun):
    if noun in ["man","father","boy"]:
        return "M"
    elif noun in ["woman","mother","girl"]:
        return "W"
    else:
        return None

def trimApos( nam):
    quotlist = ["'","’","`","\"",";",".",":"]
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

alphaTokenDelimiters = ["’","'",'"',"`","?","!","/",";",":",".",",","-","—"," ",")","(","[","]","{","}","<",">"]
def getAlphaTokenList( value):
    value = trimApos(value)
    sidx = 0
    nidx = 0
    rt = []
    while nidx < len(value):
        if value[nidx] in alphaTokenDelimiters:
            if sidx < nidx:
                rt.append( value[ sidx:nidx])
                if value[nidx] != ' ':
                    rt.append("-")
            sidx = nidx+1
        nidx += 1
    if sidx < nidx:
        rt.append( value[ sidx:nidx])
    return rt

def getAlphaTokenValue( value):
    return ' '.join( getAlphaTokenList( value))

def getTokenListString( tokens):
    rt = ""
    for tk in tokens:
        if tk.value:
            if rt:
                rt += " "
            rt += tk.value
    return rt

def matchNameSubset( name, candidate):
    cd = deepcopy(candidate)
    if not name:
        return False
    for nam in name:
        if nam == '-':
            if nam not in cd:
                return False
            continue
        found = False
        for eidx,elem in enumerate(cd):
            if nam == elem:
                del cd[ eidx]
                found = True
                break
        if not found:
            return False
    return True

def isEqualName( name, candidate):
    hasDelim = False
    cd = []
    for c in candidate:
        if c == '-':
            hasDelim = True
        else:
            cd.append( c)
    for nam in name:
        if nam == '-':
            if not hasDelim:
                return False
            continue
        found = False
        for eidx,elem in enumerate(cd): 
            if nam == elem:
                del cd[ eidx]
                found = True
                break
        if not found:
            return False
    return len(cd) == 0

def getLongestMatchIndex( tokens, tidx, candidate, maxErrors):
    cd = deepcopy(candidate)
    failCount = 0
    lastMatch = -1
    while tidx < len(tokens) and cd:
        nam = tokens[tidx].alphavalue
        found = False
        for eidx,elem in enumerate(cd):
            if nam == elem:
                found = True
                del cd[ eidx]
                break
        if not found:
            for eidx,elem in enumerate(cd):
                if (len(elem) == 1 and nam[:1] == elem) or (nam[:2] == elem[:2]):
                    del cd[ eidx]
                    found = True
                    break
        if found:
            lastMatch = tidx
        elif lastMatch == -1:
            return -1
        else:
            failCount += 1
            if failCount > maxErrors:
                return -1
        tidx += 1
    if lastMatch >= 0:
        return lastMatch+1
    else:
        return -1

def isUpperCaseName( sq):
    for elem in sq:
        if not elem or not elem[ 0].isupper():
            return False
    return True

def cmpStringListPart( dest, dstart, dend, add, astart, aend):
    ee = aend - astart
    ei = 0
    if ee != dend - dstart:
        return False
    while ei < ee and dest[ dstart + ei] == add[ astart + ei]:
        ei += 1
    return ei == ee

def isInStringListPart( dest, add, astart, aend):
    di = 0
    de = len(dest)
    while di < de:
        dn = di
        while dn < de and dest[ dn] != ',':
            dn += 1
        if cmpStringListPart( dest, di, dn, add, astart, aend):
            return True
        di = dn + 1
    return False

def joinStringList( dest, add):
    if not dest:
        return add
    ai = 0
    ae = len(add)
    rt = []
    for dd in dest:
        rt.append( dd)
    while ai < ae:
        an = ai
        while an < ae and add[ an] != ',':
            an += 1
        if ai < an and not isInStringListPart( rt, add, ai, an):
            rt.append( ',')
            rt.extend( add[ ai:an])
        ai = an + 1
    return rt

def getAlphaTokens( tokens, tidx, tend):
    rt = []
    while tidx < tend:
        if tokens[tidx].alphavalue:
            rt.append( tokens[tidx].alphavalue)
        tidx += 1
    while rt and rt[-1] in [";",",","."]:
        rt = rt[ :-1]
    return rt

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
    rt = tokens[tidx].alphavalue
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

def getNonBracketPrefix( name):
    pidx = name.find('(')
    if pidx >= 0:
        return name[ :pidx].strip()
    else:
        return name

def getLocationCandidate( name):
    pidx = name.find(',')
    oidx = name.find(' of ')
    if pidx < 0 and oidx < 0:
        return ""
    pidx += 1
    oidx += 4
    if pidx > oidx:
        return name[ pidx:].strip()
    else:
        return name[ oidx:].strip()

def getTitleSubject( title):
    if len(title) > 0 and title[0] == '(' and title[-1] == ')':
        title = title[1:-1]
    title = getNonBracketPrefix( title)
    rt = []
    for abr in splitAbbrev( title):
        rt += getAlphaTokenList( abr)
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
        if len(keyelements) >= 1:
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
    return 1.0 / (math.log( (N - 0.5) / (tf + 0.5)) + 1.0)

def tokenCountToWeightMap( tokCntMap, tokCntTotal):
    rt = {}
    if tokCntTotal:
        maxcnt = 1
        maxkey = ""
        for key,cnt in tokCntMap.items():
            if int(cnt) > maxcnt:
                maxcnt = int(cnt)
                maxkey = key
        wBase = calcTokenWeight( tokCntTotal, maxcnt)
        mBase = calcTokenWeight( tokCntTotal, 0)
        for key,cnt in tokCntMap.items():
            rt[ key] = (calcTokenWeight( tokCntTotal, cnt) - mBase) / (wBase - mBase)
    return rt

def assignRef( tokens, tidx, ref):
    ridx = 0
    while ridx < len(ref) and tidx+ridx < len(tokens) and tokens[ tidx+ridx].value == ref[ ridx]:
        ridx += 1
    if ridx != len(ref):
        tokens[ tidx].ref = ref

def listIsSentence( tokens):
    hasVerb = False
    hasObj = False
    for elem in tokens:
        if elem.nlptag:
            if elem.nlptag in ['VBG','VVG','VHG']:
                hasObj = True
            elif elem.nlptag[0] == 'V':
                hasVerb = True
            elif elem.nlptag[0] in ['P','N','W']:
                hasObj = True
    return hasVerb and hasObj

def listIsNumberSequence( tokens):
    for elem in tokens:
        if elem.nlptag == "CD":
            continue
        elif elem.nlprole == "punct":
            continue
        else:
            return False
    return True

def listIsDate( tokens):
    pt = ""
    for elem in tokens:
        if elem.nlptag == "CD":
            pt += "N"
            continue
        elif elem.value == "," or elem.value == ";":
            pt += ","
            continue
        elif elem.value in ["January","February","March","April","May","June","July","August","September","October","November","December","Feb.","Mar.","Apr.","Aug.","Sept.","Oct.","Nov.","Dec."]:
            pt += "M"
            continue
        else:
            return False
    while pt and pt[-1:] == ',':
        pt = pt[:-1]
    return pt in ["N,MN","NMN","N,M,N","MN,N","MNN","MN","M,N","NM","N,M"]

def listIsTitle( tokens):
    quotlist = ["'","’","`",'"',"—",".","-",":","!"]
    if listIsNumberSequence( tokens) or listIsDate( tokens):
        return False
    for eidx,elem in enumerate(tokens):
        if elem.nlptag:
            if elem.strustag == '_' or elem.nlptag == '_':
                continue
            if eidx > 0 and elem.nlptag == "HYPH":
                continue
            if eidx > 0 and elem.nlptag == 'CC' and elem.value and elem.value[0].islower():
                continue
            if elem.nlptag[0] in ['U','V','J','N','P','R','W','M','C','F','T','D','I']:
                if elem.value:
                    if elem.value and (elem.value[0].isupper() or elem.value[0].isdigit() or elem.value[0] in quotlist):
                        continue
        if elem.nlprole == "punct":
            continue
        return False
    return True

def outvoteTagging( tokens):
    tidx = 0
    while tidx+2 < len(tokens):
        if tokens[ tidx].nlptag == "DT":
            nidx = tidx+1
            if tokens[nidx].nlptag in ["JJ","VBG"] and tokens[nidx+1].nlptag in ["VBD","VBZ","VBP"]:
                tokens[nidx].nlptag = "NN"
        elif tokens[ tidx].nlptag in ["NNS","NNPS"]:
            nidx = tidx+1
            if tokens[nidx].nlptag in ["VBZ"]:
                if tokens[ tidx].nlptag == "NNS":
                    tokens[tidx].nlptag = "NN"
                else:
                    tokens[tidx].nlptag = "NNP"
        tidx += 1

def getSentence( tokens):
    if listIsNumberSequence( tokens):
        return Sentence( "numseq", tokens)
    if listIsDate( tokens):
        return Sentence( "date", tokens)
    if listIsTitle( tokens):
        return Sentence( "title", tokens)
    if listIsSentence( tokens):
        outvoteTagging( tokens)
        return Sentence( "sent", tokens)
    return Sentence( "", tokens)

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
        if matchNameSubset( name, thisCd):
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
    while nidx<len(tokens) and tokens[nidx].nlptag in ["MD","VBZ","VBD","VBG","VBP","VBN"]:
        nidx += 1
    if nidx<len(tokens) and tokens[nidx].nlptag in ["IN","CC"]:
        nidx += 1
        if nidx<len(tokens) and tokens[nidx].nlptag[:3] == "PRP":
            if tokens[nidx].value not in ["it","him"]:
                if tokens[nidx].value == "her":
                    if nidx+1<len(tokens) and tokens[nidx+1].nlptag not in ["VBG","JJ","JJR","JJS","RB","RBR","RBS","NN","NNS","NNP","NNPS"]:
                        return nidx
                else:
                    return nidx
    return -1

def skipNeutralPrp( tokens, tidx):
    if tokens[tidx].value.lower() == "it":
        tidx += 1
        if tidx < len(tokens) and tokens[tidx].nlptag in ["VBZ","VBD"] and tokens[tidx].value in ["'s","s","is","was"]:
            tidx += 1
            if tidx < len(tokens) and tokens[tidx].nlptag in ["RBR","RBS","RB","JJ","JJR","JJS"]:
                tidx += 1
            if tidx < len(tokens) and tokens[tidx].nlptag in ["IN"] and tokens[tidx].value in ["that","to"]:
                return tidx + 1
            if tidx < len(tokens) and tokens[tidx].nlptag in ["WP","WP$","WRB","WDT"]:
                return tidx + 1
    elif tidx+1 < len(tokens) and tokens[tidx].value.lower() in ["his","her","its"] and tokens[tidx+1].value.lower() in ["own"]:
        return tidx + 2
    return -1

def tagSentenceStrusTags( tokens):
    prev = ""
    mapprev = ""
    prev_nlprole = ""
    prevIsPlural = False
    termidx = -1
    eidx = 0
    while eidx < len(tokens):
        elem = tokens[eidx]
        if elem.nlprole == "none":
            prev = ""
            mapprev = ""
        elif elem.nlptag == '_':
            elem.strustag = '_'
            prev = ""
            mapprev = ""
        else:
            type = elem.nlptag
            isPlural = type in ["NNPS","NNS"]
            utype = unifyType( type)
            maptype = mapTag( type)
            if maptype == "T!":
                if type != ".":
                    termidx = eidx
                    maptype = "P!"
            elif maptype == "P!":
                pass
            else:
                termidx = -1
            if type == 'PRP' or type == 'PRP$':
                if not getPrpSex( elem.value):
                    maptype = ""
            if prev and utype == prev and prev in ['NN','NNS','NNP','NNPS'] and not (prevIsPlural and not isPlural):
                # TODO: "'giving the couple advice'"
                type = "_"
                maptype = "_";
            elif maptype and maptype == mapprev and maptype[-1:] == '!':
                maptype = '_'
            else:
                prev = utype
                mapprev = maptype
            prevIsPlural = isPlural
            elem.strustag = maptype
            elem.nlptag = type
            prev_nlprole = elem.nlprole
        eidx = skipMultipartName( tokens, eidx)
    if termidx >= 0:
        tokens[ termidx].strustag = "T!"

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
    while eidx < endidx:
        if tokens[eidx].nlprole == "none" or  tokens[eidx].strustag == '_':
            eidx += 1
        elif tokens[eidx].nlprole == "punct" and tokens[eidx].nlptag not in ["''","``","NFP"] and tokens[eidx].value not in ["!","-"]:
            eidx += 1
        elif tokens[eidx].nlptag == 'CC' and tokens[eidx].value and tokens[eidx].value[0].islower():
            eidx += 1
        elif tokens[eidx].nlptag == "HYPH":
            eidx += 1
        else:
            nidx = eidx
            doSkip = False
            while nidx < endidx and not (tokens[nidx].nlprole == "punct" and tokens[nidx].nlptag not in ["''","``","NFP"] and tokens[nidx].value not in ["!","-"]) and tokens[nidx].nlptag != "HYPH":
                if tokens[nidx].nlprole == "none" or  tokens[nidx].strustag == '_':
                    doSkip = True
                    break
                nidx += 1
            if not doSkip and not listIsNumberSequence( tokens[eidx:nidx]) and not listIsDate( tokens[eidx:nidx]):
                ofs = 0
                if eidx+2 < nidx and tokens[eidx].nlptag in ["''","``"] and tokens[nidx-1].nlptag in ["''","``"]:
                    eidx += 1
                    nidx -= 1
                while eidx < nidx:
                    tagEntitySequenceTokenShift( tokens, eidx, ofs)
                    ofs += 1
                    eidx += 1
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
        elif tokens[eidx].value == ",":
            eb = ','
        if eb:
            eidx += 1
            startidx = eidx
            while eidx < len(tokens) and not tokens[eidx].value.find(eb) >= 0:
                eidx += 1
            if eidx < len(tokens):
                endidx = eidx
                if listIsTitle( tokens[ startidx:endidx]):
                    if eb == ',' and tokens[ startidx].nlptag == "CC" and tokens[ startidx].value and tokens[ startidx].value[0].islower():
                        startidx += 1
                    tagEntitySequenceStrusTags( tokens, startidx, endidx)
            else:
                eidx = startidx
        else:
            eidx += 1

def tagSentenceEntityInvReferences( tokens, entityInvMap):
    tidx = 0
    while tidx < len(tokens):
        if not tokens[ tidx].ref and tokens[tidx].nlprole != "none" and tokens[ tidx].nlptag in ["NNP","NNPS"]:
            name = getMultipartName( tokens, tidx)
            namestr = ' '.join( name)
            if namestr in entityInvMap:
                refstr = entityInvMap[ namestr]
                if refstr != namestr:
                    tokens[ tidx].ref = refstr.split(' ')
                tagTokenNameReference( tokens, tidx, len(name))
        tidx = skipMultipartName( tokens, tidx)

def tagSentenceLinkReferences( tokens, firstKeyLinkListMap):
    tidx = 0
    while tidx < len(tokens):
        if not tokens[ tidx].ref and tokens[tidx].nlprole != "none" and tokens[tidx].alphavalue in firstKeyLinkListMap:
            candidateList = firstKeyLinkListMap[ tokens[tidx].alphavalue]
            bestIdx = -1
            bestLen = 0
            bestTokLen = 0
            bestCd = None
            bestDoAddRef = False
            for cidx,cd in enumerate( candidateList):
                doAddRef = False
                sidx = 0
                nidx = tidx
                while sidx < len(cd) and nidx < len(tokens):
                    if tokens[nidx].alphavalue != cd[sidx] or tokens[nidx].nlprole == 'none' or (tokens[nidx].nlptag in ["MD","VBZ","VBD","VBP"] and tokens[nidx].value and tokens[nidx].value[0].islower()):
                        if not tokens[nidx].alphavalue:
                            nidx += 1
                            doAddRef = True
                            continue
                        if len(cd) > 3 and tokens[nidx].alphavalue == '-':
                            nidx += 1
                            doAddRef = True
                            continue
                        break
                    nidx += 1
                    sidx += 1
                if sidx == len(cd):
                    if len(cd) > bestLen:
                        bestLen = len(cd)
                        bestIdx = cidx
                        bestTokLen = nidx - tidx
                        bestCd = cd
                        bestDoAddRef = doAddRef
            if bestIdx >= 0 and bestLen > 1:
                sidx = 0
                if bestDoAddRef and not tokens[ tidx].ref:
                    tokens[ tidx].ref = bestCd
                while sidx < bestTokLen:
                    tagEntitySequenceTokenShift( tokens, tidx+sidx, sidx)
                    sidx += 1
                tidx += sidx
            else:
                tidx = skipMultipartName( tokens, tidx)
        else:
            tidx = skipMultipartName( tokens, tidx)

def matchTokenNameReference( tokens, tidx, nam):
    sidx = 0
    nidx = tidx
    while sidx < len(nam) and nidx < len(tokens):
        if not tokens[nidx].alphavalue:
            nidx += 1
            continue
        if tokens[nidx].alphavalue != nam[sidx] or tokens[nidx].nlprole == 'none':
            break
        sidx += 1
        nidx += 1
    if sidx == len(nam):
        return nidx-tidx
    else:
        return -1

def tagTokenNameReference( tokens, tidx, size):
    sidx = 0
    while sidx < size:
        tagEntitySequenceTokenShift( tokens, tidx+sidx, sidx)
        sidx += 1

def tagSentenceNameReferences( tokens, nam):
    if nam:
        tidx = 0
        while tidx < len(tokens):
            if tokens[tidx].nlprole == "none":
                tidx = skipMultipartName( tokens, tidx)
                continue
            mlen = matchTokenNameReference( tokens, tidx, nam)
            if mlen >= 1:
                tagTokenNameReference( tokens, tidx, len(nam))
                tidx += mlen
            else:
                tidx += 1

def tagSentenceNameMapReferences( tokens, namemap):
    tidx = 0
    while tidx < len(tokens):
        if tokens[tidx].nlprole == "none":
            tidx = skipMultipartName( tokens, tidx)
            continue
        tokval = tokens[tidx].alphavalue
        if tokval in namemap:
            bestIdx = -1
            bestLen = 0
            cdlist = namemap[ tokval]
            for cdIdx,cd in enumerate(cdlist):
                if len(cd) > bestLen:
                    mlen = matchTokenNameReference( tokens, tidx, cd)
                    if mlen >= 1:
                        bestLen = mlen
                        bestIdx = cdIdx
            if bestIdx >= 0 and bestLen > 1:
                tagTokenNameReference( tokens, tidx, bestLen)
                tidx += bestLen
            else:
                tidx += 1
        else:
            tidx += 1

def tagSentenceSubjects( tokens):
    bracketCnt = 0
    eidx = 0
    while eidx < len(tokens):
        elem = tokens[ eidx]
        name = None
        if elem.nlptag == "-LRB-" and elem.value == '(':
            bracketCnt += 1
        elif elem.nlptag == "-RRB-" and elem.value == ')' and bracketCnt > 0:
            bracketCnt -= 1
        elif bracketCnt > 0:
            pass
        elif elem.nlptag in ["WP","WP$","WRB","WDT","IN"] and elem.value and elem.value[0] == 'w':
            # skip subordinate clause
            nidx = eidx + 1
            if nidx < len(tokens) and tokens[nidx].nlptag == "DT" and tokens[nidx].value and not tokens[nidx].value[0].isupper():
                nidx += 1
            if nidx < len(tokens) and tokens[nidx].nlptag in ["RBR","RBS","RB"] and tokens[nidx].value and not tokens[nidx].value[0].isupper():
                nidx += 1
            while nidx < len(tokens) and tokens[nidx].nlptag in ["RBS","JJ","JJS","JJR","VBG","HYPH"] and not tokens[nidx].value[0].isupper():
                nidx += 1
            while nidx < len(tokens) and tokens[nidx].nlptag in ["NN","NNS","NNP","NNPS"]:
                nidx = skipMultipartName( tokens, nidx)
            if nidx < len(tokens) and tokens[nidx].nlptag in ["MD","VBZ","VBD","VBP"] and tokens[nidx].value and not tokens[nidx].value[0].isupper():
                eidx = nidx
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
                       elif tokens[ei].nlprole == "ROOT" and tokens[ei].nlptag in ["MD","VBZ","VBD","VBP"]:
                           elem.strusrole = "S"
                           break
                       else:
                           break
        if elem.strusrole == "S":
            ei = eidx+1
            while ei < len(tokens) and tokens[ei].nlptag == '_':
                tokens[ei].strusrole = "_"
                ei += 1
        eidx += 1

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
        isTagged = tokens[eidx].nlprole == "none"
        if tokens[ eidx].nlptag[:2] == 'NN':
            ref = tokens[ eidx].ref
            isEntity = False
            if tokens[ eidx].nlptag[:3] == 'NNP':
                isEntity = True
            elif tokens[ eidx].nlptag[:2] == 'NN' and tokens[ eidx].value and tokens[ eidx].value[0].isupper():
                isEntity = True
            if isEntity:
                name = getMultipartName( tokens, eidx)
            if sentenceIdx == 0:
                nidx = getLongestMatchIndex( tokens, eidx, titlesubject, 2)
                if nidx >= 0:
                    ref = titlesubject
                    tokens[ eidx].ref = ref
                    ofs = 0
                    while eidx < nidx:
                        tagEntitySequenceTokenShift( tokens, eidx, ofs)
                        ofs += 1
                        eidx += 1
                        isEntity = True
                        sentenceIdx = -1
            if isEntity and not ref and not isTagged:
                for bt in bestTitleMatches:
                    if isEqualName( name, bt):
                        tokens[ eidx].ref = titlesubject
                        ref = titlesubject
                        isEntity = True
                        break
            if isEntity and not ref and not isTagged:
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
                    if tokens[ eidx].nlptag == "NN":
                        tokens[ eidx].nlptag = "NNP"
                    else:
                        tokens[ eidx].nlptag = "NNPS"
                    if tokens[ eidx].strustag == "N":
                        tokens[ eidx].strustag = "E"
            if isEntity:
                key = ' '.join( ref or name)
                usedEntities.append( key)
            elif ref:
                tokens[ eidx].strustag = 'E'
                key = ' '.join( ref)
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

def lowerNnpWeightMap( map):
    delKeys = []
    for key,weight in map.items():
        map[ key] *= 0.85
        if weight < 0.1:
            delKeys.append( key)
    for key in delKeys:
        del map[ key]

def updateNnpWeightMap( map, namestr):
    if namestr in map:
        map[ namestr] += 1.0
    else:
        map[ namestr] = 1.0

# param sexSubjectMap: map sex:string -> Subject
def tagSentencePrpReferences( tokens, sentidx, sexSubjectMap, nnpSexMap, synonymNnpWeightMap, synonymCountMap):
    subjects = []
    eidx = 0
    lowerNnpWeightMap( synonymNnpWeightMap)
    while eidx < len( tokens):
        if tokens[ eidx].nlptag == "DT" and tokens[ eidx].value.lower() == "the":
            nidx = eidx + 1
            if nidx < len(tokens):
                if tokens[nidx].nlptag in ["NN","NNS"] and not tokens[nidx].ref:
                    noun = getMultipartNameStr( tokens, nidx)
                    nidx = skipMultipartName( tokens, nidx)
                    if nidx < len(tokens) and not tokens[nidx].nlptag in ['IN'] and noun in synonymCountMap:
                        nnpWeightMap = synonymCountMap[ noun]
                        best_weight = 0.0
                        best_nnp = None
                        for nnp,nnpweight in nnpWeightMap.items():
                            if nnp in synonymNnpWeightMap:
                                ww = nnpweight * synonymNnpWeightMap[ nnp]
                                if ww > best_weight:
                                    best_nnp = nnp
                                    best_weight = ww
                        if best_nnp and best_weight > 0.2:
                            tokens[ eidx+1].ref = best_nnp.split(' ')
        elif tokens[ eidx].nlptag[:2] == 'NN':
            name = tokens[ eidx].ref
            if not name and tokens[ eidx].nlptag in ["NNP","NNPS"]:
                name = getMultipartName( tokens, eidx)
            if name:
                namekey = ' '.join(name)
                updateNnpWeightMap( synonymNnpWeightMap, namekey)
                if tokens[ eidx].strusrole == "S":
                    if namekey in nnpSexMap:
                        sex = nnpSexMap[ namekey]
                    else:
                        sex = None
                    matchprev = False
                    if subjects:
                        for sb in subjects:
                            if sb.sex == sex and sb.strustag == tokens[ eidx].strustag and (matchNameSubset( name, sb.value) or matchNameSubset( sb.value, name)):
                                matchprev = True
                    if not matchprev:
                        subjects.append( Subject( sex, tokens[ eidx].strustag, name, sentidx))
            nidx = skipToOwnPrp( tokens, skipMultipartName( tokens, eidx))
            if nidx >= 0:
                eidx = nidx
        elif tokens[ eidx].nlptag[:3] == 'PRP' and not tokens[ eidx].ref:
           nidx = skipNeutralPrp( tokens, eidx)
           if nidx >= 0:
               eidx = nidx-1
           else:
               sex = getPrpSex( tokens[ eidx].value)
               if sex and sex in sexSubjectMap:
                   subj = sexSubjectMap[ sex]
                   if ',' in subj.value:
                       maxdiff = 2
                   else:
                       maxdiff = 10
                   if sentidx - subj.sentidx <= maxdiff:
                       subj.sentidx = sentidx
                       tokens[ eidx].ref = subj.value
                       nnpkey = ' '.join( subj.value)
                       updateNnpWeightMap( synonymNnpWeightMap, nnpkey)
        eidx += 1
    if subjects:
        if len(subjects) > 1:
            combined = subjects[0].value
            combinedtag = subjects[0].strustag
            si = 1
            while si < len(subjects):
                combined = joinStringList( combined, subjects[ si].value)
                if combinedtag != subjects[0].strustag:
                    combinedtag = None
                    break
                si += 1
            if ',' in combined:
                sx = 'P'
            else:
                sx = subjects[0].sex
            sexSubjectMap[ sx] = Subject( sx, combinedtag, combined, sentidx)
        else:
            sb = subjects[ 0]
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
                rt += ("%s (%s)\t%s\t%s\t%s\t%s\n" % (sr,nr,st,nt,vv,rr))
            else:
                rt += ("%s (%s)\t%s\t%s\t%s\n" % (sr,nr,st,nt,vv))
    else:
        for elem in sent:
            st = (elem.strustag or "")
            vv = (elem.value or "")
            if elem.ref:
                rr = ' '.join( elem.ref)
                rt += ("%s\t%s\t%s\n" % (st,vv,rr))
            else:
                rt += ("%s\t%s\n" % (st,vv))
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
            nlptag = node.tag_
            strustag = None
            if node.tag_[:3] == "NNP" and value.count('.') > 1:
                for abr in splitAbbrev( value):
                    elemList = getAlphaTokenList(abr)
                    if not elemList:
                        tokens.append( NlpToken( None, None, nlptag, node.dep_, abr, "", None))
                    else:
                        for elem in elemList:
                            tokens.append( NlpToken( strustag, None, nlptag, node.dep_, abr, elem, None))
                            abr = ""
                            nlptag = "_"
                            strustag = "_"
            elif value in alphaTokenDelimiters:
                tokens.append( NlpToken( None, None, nlptag, node.dep_, value, "-", None))
            else:
                elemList = getAlphaTokenList(value)
                if not elemList:
                    tokens.append( NlpToken( None, None, nlptag, node.dep_, value, "", None))
                else:
                    for elem in elemList:
                        tokens.append( NlpToken( strustag, None, nlptag, node.dep_, value, elem, None))
                        value = ""
                        nlptag = "_"
                        strustag = "_"
        if node.dep_ == "punct":
            if not eb:
                if value in [':',';','.']:
                    sentences.append( getSentence( tokens))
                    tokens = []
            else:
                if value == '.':
                    sentences.append( getSentence( tokens))
                    tokens = []
                    ebstk = []
                    eb = None
    if tokens:
        sentences.append( getSentence( tokens))
    return sentences

def getDocumentNlpTokCountMap( sentences):
    rt = {}
    for sent in sentences:
        if sent.type == "title":
            tidx = 0
            tstart = -1
            while tidx < len(sent.tokens):
                if sent.tokens[ tidx].nlprole == "punct":
                    if tstart >= 0:
                        if listIsTitle( sent.tokens[ tstart:tidx]):
                            name = getAlphaTokens( sent.tokens, tstart, tidx)
                            key = ' '.join( name)
                            if key in rt:
                                rt[ key] += 1.5
                            else:
                                rt[ key] = 1.5
                    tstart = -1
                elif tstart < 0:
                    tstart = tidx
                tidx += 1
            if tstart >= 0:
                if listIsTitle( sent.tokens[ tstart:tidx]):
                    name = getAlphaTokens( sent.tokens, tstart, tidx)
                    key = ' '.join( name)
                    if key in rt:
                        rt[ key] += 1.5
                    else:
                        rt[ key] = 1.5
        if sent.type == "sent":
            tidx = 0
            while tidx < len(sent.tokens):
                tok = sent.tokens[ tidx]
                if tok.nlptag in ["NNP","NNPS"]:
                    nidx = skipMultipartName( sent.tokens, tidx)
                    name = tok.ref or getAlphaTokens( sent.tokens, tidx, nidx)
                    key = ' '.join( name)
                    if key in rt:
                        rt[ key] += 1
                    else:
                        rt[ key] = 1
                    tidx = nidx
                else:
                    tidx += 1
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
    totalUsageCnt = 0
    for key,usage in usageMap.items():
        if usage >= 3:
             selMap[ key] = usage
             totalUsageCnt += usage
             if usage in usageCntMap:
                 usageCntMap[ usage] += 1
             else:
                 usageCntMap[ usage] = 1
    minusage = 3
    maxUsageCnt = totalUsageCnt * 0.6
    usageCnt = 0
    for usage in sorted( usageCntMap.keys(), reverse=True):
        usageCnt += usageCntMap[ usage] * usage
        if usageCnt > maxUsageCnt:
            minusage = usage
            break
    if minusage > 3:
        reduMap = {}
        for key,usage in selMap.items():
            if usage >= minusage or key.count(' ') > 3:
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
        if matchNameSubset( name, titlesubject):
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
            if matchNameSubset( name, titlesubject) and len(name) > len(bestName):
                rt.append( name)
    return rt

def addToNnpSexCountMap( map, key, sex, weight):
    # print( "** ADD SEX %s -> %s %.5f" % (key, sex, weight))
    if sex and key in map:
        if sex in map[ key]:
            map[ key][ sex] += weight
        else:
            map[ key][ sex] = weight
    else:
        map[ key] = { sex : weight }

def reduSubjectMap( map):
    delPastSubjects = []
    for subj,sexmap in map.items():
        delSexmap = []
        for sex,weight in sexmap.items():
            if weight < 0.3:
                delSexmap.append( sex)
            map[ subj][ sex] *= 0.80
        for sex in delSexmap:
            del map[ subj][ sex]
        if not map[ subj]:
            delPastSubjects.append( subj)
    for subj in delPastSubjects:
        del map[ subj]

def initSubjectMap( updatelist, map, subj):
    if subj in map:
        if "" not in map[ subj]:
            updatelist.append( [subj, "", 1.0] )
        for key,weight in map[ subj].items():
            updatelist.append( [subj, key, weight * 0.3 + 0.2] )
    else:
        updatelist.append( [subj, "", 1.0] )

def fireSubjectMap( updateList, map, sex, addweight):
    totweight = 0.0
    for subj,sexmap in map.items():
        for xx,ww in sexmap.items():
            totweight += ww
    # print( "** FIRE SEX %s TOT %.5f" % (sex, totweight))
    for subj,sexmap in map.items():
        if sex in sexmap:
            weight = map[ subj][ sex]
            updateList.append( [ subj, sex, addweight * weight / totweight] )
        if sex and "" in sexmap:
            weight = map[ subj][ ""]
            updateList.append( [ subj, sex, addweight * weight / totweight] )

def filterUpdateList( updateList):
    rt = []
    for sex in ["W","M","N","P"]:
        maxweight = 0.0
        for subj,updatesex,weight in updateList:
            if updatesex == sex and weight > maxweight:
                maxweight = weight
        for subj,updatesex,weight in updateList:
            if updatesex == sex and weight > maxweight/2:
                rt.append( [subj,updatesex,weight] )
    for subj,updatesex,weight in updateList:
        if not updatesex:
            rt.append( [subj,updatesex,weight] )
    return rt

def addUpdatesSubjectMap( map, updatelist):
    for subj,sex,weight in updatelist:
        if subj not in map:
            map[ subj] = {}
        if sex in map[ subj]:
            map[ subj][ sex] += weight
        else:
            map[ subj][ sex] = weight

# param titlesubject string[]
# param sentences Sentence[]
# return map NNP -> sex count  {'M':0, 'W':0, 'N':0, 'P':0} list 
def getDocumentNnpSexCountMap( titlesubject, sentences):
    rt = {}
    pastSubjects = {}
    for sent in sentences:
        updateList = []
        # print( "** SENT %s" % getTokenListString( sent.tokens))

        reduSubjectMap( pastSubjects)

        if sent.type != "sent":
            continue
        tidx = 0
        while tidx < len(sent.tokens):
            tok = sent.tokens[tidx]
            if tok.nlptag[0:3] == "NNP":
                name = tok.ref or getMultipartName( sent.tokens, tidx)
                namestr = ' '.join( name)
                nidx = skipToOwnPrp( sent.tokens, skipMultipartName( sent.tokens, tidx))
                sex = ""
                if nidx >= 0:
                    sex = getPrpSex( sent.tokens[nidx].value)
                    if sex in ['W','M']:
                        weightfactor = 1.0
                    else:
                        weightfactor = 0.7
                    if tok.strusrole == 'S':
                        weightfactor *= 1.7
                    if namestr:
                        updateList.append( [namestr, sex, weightfactor] )
                    tidx = nidx + 1
                elif tok.strusrole == 'S':
                    initSubjectMap( updateList, pastSubjects, namestr)
            elif tok.nlptag[0:3] == "PRP":
                nidx = skipNeutralPrp( sent.tokens, tidx)
                if nidx >= 0:
                    # print( "** SKIP NEUTRAL PRP %s" % getTokenListString( sent.tokens[tidx:nidx]))
                    tidx = nidx-1
                else:
                    sex = getPrpSex( tok.value)
                    # print( "** PRP %s SEX %s" % (tok.value,sex))
                    weight = 0.7
                    if tok.strusrole == 'S':
                        weight = 1.2
                    fireSubjectMap( updateList, pastSubjects, sex, weight)
            tidx += 1
        # for sbj,sexmap in pastSubjects.items():
        #     print( "** SEXMAP %s { %s}" % (sbj,' '.join(["%s:%.3f " % (key,val) for key,val in sexmap.items()])))
        # for sbj,sex,weight in updateList:
        #     print( "** UPDATE %s %s %.4f" % (sbj,sex,weight))
        normfactor = math.sqrt( len( updateList))
        for nnp,sex,weight in updateList:
            if sex:
                addToNnpSexCountMap( rt, nnp, sex, float(weight) / normfactor)
        addUpdatesSubjectMap( pastSubjects, filterUpdateList( updateList))
    return rt

def addWeightSynonymMap( synomap, nnp, synonym, weight):
    # print( "** ADD SYNONYM WEIGHT '%s' '%s' %.3f" % (synonym, nnp, weight))
    if nnp in synomap:
        if synonym in synomap[nnp]:
            synomap[nnp][synonym] += weight
        else:
            synomap[nnp][synonym] = weight
    else:
        synomap[nnp] = { synonym : weight }

# param sentences NlpToken[][]
# return map NNP -> { synonym : count }
def getDocumentSynonymCountMap( titlesubject, sentences):
    synomap = {}
    sidx = 0
    pastSubjects = {}
    pastSentSomeNouns = {}
    for sent in sentences:
        # if sent.type == "sent":
        #     print( "** SENT %s" % getTokenListString( sent.tokens))
        sentSubjects = []
        sentNNPs = []
        sentSomeNouns = []
        sentDetNouns = []
        sentSbjNouns = []
        nofSentSubjects = 0
        if sent.type == "sent":
            tidx = 0
            while tidx < len(sent.tokens):
                tok = sent.tokens[tidx]
                if tok.strusrole == 'S':
                    nofSentSubjects += 1
                if tok.nlptag[0:3] == "NNP":
                    name = tok.ref or getMultipartName( sent.tokens, tidx)
                    namestr = ' '.join( name)
                    sentNNPs.append( namestr)
                    if tok.strusrole == 'S':
                        sentSubjects.append( namestr)
                        nidx = skipMultipartName( sent.tokens, tidx)
                        if nidx < len(sent.tokens) and sent.tokens[nidx].nlptag in ["VBZ","VBP"] and sent.tokens[nidx].value in ["is","are"]:
                            nidx += 1
                            if nidx < len(sent.tokens):
                                if sent.tokens[nidx].nlptag == "DT":
                                    nidx += 1
                                elif sent.tokens[nidx].nlptag == "NNPS":
                                    nidx += 1
                                    if nidx < len(sent.tokens) and sent.tokens[nidx].nlptag == "POS":
                                        nidx += 1
                            while nidx < len(sent.tokens) and sent.tokens[nidx].nlptag in ["RBS","RBR","JJ","VBG"]:
                                nidx += 1
                            if nidx < len(sent.tokens) and sent.tokens[nidx].nlptag == "NN":
                                noun = getMultipartNameStr( sent.tokens, nidx)
                                tidx = nidx
                                addWeightSynonymMap( synomap, namestr, noun, 2.0)
                elif tok.nlptag == "NN":
                    noun = getMultipartNameStr( sent.tokens, tidx)
                    nidx = skipMultipartName( sent.tokens, tidx)
                    sentSomeNouns.append( noun)
                    if nidx < len(sent.tokens):
                        if sent.tokens[nidx].nlptag == "IN":
                            nidx += 1
                            if nidx < len(sent.tokens) and sent.tokens[nidx].nlptag == "DT":
                                nidx += 1
                                while nidx < len(sent.tokens) and sent.tokens[nidx].nlptag in ["RBS","RBR","JJ","VBG"]:
                                    nidx += 1
                                if nidx < len(sent.tokens) and sent.tokens[nidx].nlptag in ["NN","CD"]:
                                    noun2 = getMultipartNameStr( sent.tokens, nidx)
                                    nidx = skipMultipartName( sent.tokens, nidx)
                                    sentSomeNouns.append( noun2)
                                    if nidx < len(sent.tokens) and sent.tokens[nidx].nlptag == ",":
                                        nidx += 1
                                    if nidx < len(sent.tokens) and sent.tokens[nidx].nlptag == "NNP":
                                        name = sent.tokens[nidx].ref or getMultipartName( sent.tokens, nidx)
                                        namestr = ' '.join( name)
                                        addWeightSynonymMap( synomap, namestr, noun, 2.5)
                                        sentNNPs.append( namestr)
                                        tidx = nidx
                        elif sent.tokens[nidx].nlptag[:3] == "NNP":
                            name = sent.tokens[nidx].ref or getMultipartName( sent.tokens, nidx)
                            namestr = ' '.join( name)
                            addWeightSynonymMap( synomap, namestr, noun, 1.0)
                            tidx = nidx-1
                elif pastSubjects and tok.nlptag == "DT":
                    detClass = tok.value.lower() == "the"
                    nidx = tidx + 1
                    hasAdjectiv = False
                    while nidx < len(sent.tokens) and sent.tokens[nidx].nlptag in ["RBS","RBR","JJ","VBG"]:
                        hasAdjectiv = True
                        nidx += 1
                    if nidx < len(sent.tokens) and sent.tokens[nidx].nlptag == "NN":
                        noun = getMultipartNameStr( sent.tokens, nidx)
                        nidx = skipMultipartName( sent.tokens, nidx)
                        if nidx < len(sent.tokens):
                            if sent.tokens[nidx].strusrole == 'S':
                                sentSbjNouns.append( noun)
                            if detClass and not hasAdjectiv and not sent.tokens[nidx].nlptag in ['IN']:
                                sentDetNouns.append( noun)
                            else:
                                sentSomeNouns.append( noun)
                                if noun in pastSentSomeNouns:
                                    pastSentSomeNouns[ noun] += 1.1
                                else:
                                    pastSentSomeNouns[ noun] = 1.1
                        tidx = nidx-1
                elif tok.nlptag == "IN":
                    nidx = tidx + 1
                    while nidx < len(sent.tokens) and sent.tokens[nidx].nlptag in ["RBS","RBR","JJ","VBG"]:
                        nidx += 1
                    if nidx < len(sent.tokens) and sent.tokens[nidx].nlptag == "NN":
                        noun = getMultipartNameStr( sent.tokens, nidx)
                        nidx = skipMultipartName( sent.tokens, nidx)
                        if nidx < len(sent.tokens) and sent.tokens[nidx].nlptag[:3] == "NNP":
                            name = sent.tokens[nidx].ref or getMultipartName( sent.tokens, nidx)
                            namestr = ' '.join( name)
                            addWeightSynonymMap( synomap, namestr, noun, 2.7)
                            tidx = nidx-1
                tidx += 1
        delPastSentSomeNouns = []
        for noun,weight in pastSentSomeNouns.items():
            if weight < 0.1:
                delPastSentSomeNouns.append( noun)
            else:
                pastSentSomeNouns[ noun] *= 0.6
        for noun in delPastSentSomeNouns:
            del pastSentSomeNouns[ noun]

        pastSentSomeNounsStr = ""
        # for noun,weight in pastSentSomeNouns.items():
        #     if pastSentSomeNounsStr:
        #         pastSentSomeNounsStr += ", "
        #     pastSentSomeNounsStr += "SOME %s %.3f" % (noun,weight)
        # for synonym in sentDetNouns:
        #     if pastSentSomeNounsStr:
        #        pastSentSomeNounsStr += ", "
        #     pastSentSomeNounsStr += "DET NN %s" % (synonym)
        # for nnp in sentNNPs:
        #     if pastSentSomeNounsStr:
        #        pastSentSomeNounsStr += ", "
        #     if nnp in sentSubjects:
        #        pastSentSomeNounsStr += "SUBJ-"
        #     pastSentSomeNounsStr += "NNP %s" % (nnp)
        # if pastSentSomeNounsStr:
        #     print( "** OBJ %s" % (pastSentSomeNounsStr))

        if sentSubjects:
            for nnp,weight in pastSubjects.items():
                if nnp not in sentSubjects and weight > 0.0:
                    pastSubjects[ nnp] *= 0.4
        deleteSubjects = []
        for sbj,weight in pastSubjects.items():
            weight *= 0.8
            if weight < 0.3:
                deleteSubjects.append( sbj)
            else:
                pastSubjects[ sbj] = weight
        for sbj in deleteSubjects:
            del pastSubjects[ sbj]
        for synonym in sentDetNouns:
            if synonym not in sentSomeNouns and synonym not in pastSentSomeNouns:
                factor = 1.0
                if synonym not in sentSbjNouns:
                    factor = 0.6
                for sbj,weight in pastSubjects.items():
                    addWeightSynonymMap( synomap, sbj, synonym, weight * factor)
        for noun in sentSomeNouns:
            if noun not in sentDetNouns:
                for sbj in sentNNPs:
                    addWeightSynonymMap( synomap, sbj, noun, -0.7)
                for sbj,weight in pastSubjects.items():
                    addWeightSynonymMap( synomap, sbj, noun, -0.7 * weight)
        for sbj in sentSubjects:
            if sbj in pastSubjects:
                pastSubjects[ sbj] += 1.0 / nofSentSubjects ** 2
            else:
                pastSubjects[ sbj] = 2.0 / nofSentSubjects ** 2
            if pastSubjects[ sbj] > 2.5:
                pastSubjects[ sbj] = 2.5
    rt = {}
    for nnp,map in synomap.items():
        if matchNameSubset( nnp.split(' '), titlesubject):
            bestweight = 1.0
            bestcd = None
            for noun,weight in map.items():
                if weight > bestweight:
                    bestweight = weight
                    bestcd = noun
            if bestcd:
                map[ bestcd] *= 2
        minweight = None
        for noun,weight in sorted( map.items(), key=lambda item: item[1], reverse=True):
            if not minweight:
                minweight = max( weight / 2, 1.0)
            if weight > minweight:
                if noun in rt:
                    rt[ noun][ nnp] = weight
                else:
                    rt[ noun] = { nnp : weight }
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
    if len(values) >= 2 and values[0] <= values[1] * 2:
        return None
    if len(values) >= 1 and values[0] <= 2.0:
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

def printSentenceTagging( title, tokens):
    origtgstr = ""
    for tk in tokens:
        if origtgstr:
            origtgstr += " "
        attr = ""
        if tk.strusrole:
            attr += "%s:" % tk.strusrole
        if tk.nlptag:
            attr += tk.nlptag
        if tk.nlprole:
            attr += "(" + tk.nlprole + ")"
        if tk.strustag:
            attr += "/%s" % tk.strustag
        if tk.ref:
            attr += "-> %s" % ' '.join( tk.ref)
        origtgstr += "'%s' :%s" % (tk.value,attr)
    print( "* Tagging %s %s" % (title,origtgstr))

def tagDocument( title, text, entityMap, entityInvMap, accuvar, verbose, complete):
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
    if verbose:
        for sent in sentences:
            print( "* Sentence [%s] %s" % (sent.type, getTokenListString( sent.tokens)))
            printSentenceTagging( "", sent.tokens)
    start_time = time.time()
    
    for sent in sentences:
        countTokens( tokCntMap, sent.tokens)
        tokCntTotal += len(sent.tokens)
        tagSentenceLinkReferences( sent.tokens, entityFirstKeyMap)
        # printSentenceTagging( "STEP0", sent.tokens)
        if sent.type == "title":
            tagEntitySequenceStrusTags( sent.tokens, 0, len(sent.tokens))
            tagSentenceStrusTags( sent.tokens)
        elif sent.type == "sent":
            tagSentenceStrusTags( sent.tokens)

        tagSentenceEntityInvReferences( sent.tokens, entityInvMap)
        tagEntitySequenceStrusTagsInBrackets( sent.tokens)
        # printSentenceTagging( "STEP1", sent.tokens)
        tagSentenceNameReferences( sent.tokens, titlesubject)
        # printSentenceTagging( "STEP2", sent.tokens)

    tokCntWeightMap = tokenCountToWeightMap( tokCntMap, tokCntTotal)
    countNnp = getDocumentNlpTokCountMap( sentences)
    bestTitleMatches = getBestTitleMatches( titlesubject, countNnp)
    nounCandidateKeyMap = {}
    nounCandidates = {}
    titlekey = ' '.join( titlesubject)
    for sidx,sent in enumerate(sentences):
        if sent.type == "sent":
            tagSentenceSubjects( sent.tokens)
            tagSentenceCompleteNounReferences( sent.tokens, titlesubject, bestTitleMatches, nounCandidateKeyMap, nounCandidates, tokCntWeightMap, sidx)
            # printSentenceTagging( "STEP3", sent.tokens)
    countNnp = getDocumentNlpTokCountMap( sentences)
    splitNlpTokCountMap( countNnp)
    mostUsedNnp = getMostUsedMultipartList( countNnp)
    mostUsedNnpMap = getFirstKeyMap( mostUsedNnp, getEntityElements)
    for sidx,sent in enumerate(sentences):
        if sent.type == "sent" or sent.type == "title":
            tagSentenceNameMapReferences( sent.tokens, mostUsedNnpMap)
            # printSentenceTagging( "STEP4", sent.tokens)
    nnpSexCountMap = getDocumentNnpSexCountMap( titlesubject, sentences)
    synonymCountMap = getDocumentSynonymCountMap( titlesubject, sentences)
    nnpSexMap = getDocumentNnpSexMap( nnpSexCountMap)
    sexSubjectMap = {}
    synonymNnpWeightMap = {}
    titlesex = None
    if titlekey in nnpSexMap:
        titlesex = nnpSexMap[ titlekey]
        sexSubjectMap[ titlesex] = Subject( titlesex, "E", titlesubject, 0)
    sentidx = 0
    lsidx = -1
    for sidx,sent in enumerate(sentences):
        if sent.type == "sent":
            sentidx += 1
            lsidx = sidx
            tagSentencePrpReferences( sent.tokens, sentidx, sexSubjectMap, nnpSexMap, synonymNnpWeightMap, synonymCountMap)
        elif lsidx == sidx+1:
            sentidx += 1
    for sent in sentences:
        rt += printSentence( sent.tokens, complete)
    if verbose:
        for bm in bestTitleMatches:
            print( "* Best title match '%s'" % ' '.join(bm))
        for subj,sexmap in nnpSexCountMap.items():
            print( "* Entity sex stats %s -> %s" % (subj, sexmap))
        for subj,sex in nnpSexMap.items():
            print( "* Entity sex %s -> %s" % (subj, sex))
        for subj,sc in nnpSexCountMap.items():
            print( "* Subject %s -> %s" % (subj, sc))
        for noun,synmap in synonymCountMap.items():
            for entity, weight in synmap.items():
                print( "* Synonym %s -> %s %.3f" % (noun, entity, weight))
        for key in countNnp:
            print( "* Entity usage %s # %.3f" % (key, countNnp[ key]))
        for key,linklist in entityFirstKeyMap.items():
            for link in linklist:
                print( "* Link '%s'" % ' '.join(link))
        for key,weight in tokCntWeightMap.items():
            print( "* Token count %s # %.5f" % (key, weight))
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
            try:
                rt += chr( int( filename[fidx+1:fidx+3], 16))
            except ValueError:
                raise RuntimeError( "illegal characters in encoded file name reference: %s" % filename)
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
    print( "%s [-h|--help] [-V|--verbose] [-K|--complete] [-D|--duration] [-S|--status] [-C <chunksize> ]" % __file__)

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
    entityInvMap = {}
    locationMap = {}
    accuvar = {}
    entityReadState = False
    for line in sys.stdin:
        if len(line) > 6 and line[0:6] == '#FILE#':
            if filename:
                if content:
                    for location,count in locationMap.items():
                        if count >= 3:
                            if not location in entityMap:
                                entityMap[ location] = location
                            if not location in entityInvMap:
                                entityInvMap[ location] = location
                    title = getTitleFromFileName( filename)
                    result = tagDocument( title, content, entityMap, entityInvMap, accuvar, verbose, complete)
                    printOutput( filename, result)
                    content = ""
                else:
                    printOutput( filename, "")
            filename = line[6:].rstrip( "\r\n")
            entityReadState = True
            entityMap = {}
            entityInvMap = {}
        else:
            if entityReadState and line[:2] == '##':
                entitydef = line[2:].strip("\n\r\t ")
                spidx = entitydef.find('##')
                if spidx >= 0:
                    left = entitydef[ :spidx]
                    right = entitydef[ (spidx+2):]
                    eid = getNonBracketPrefix( left)
                    location = getLocationCandidate( left)
                    eval = right
                    entityMap[ eid] = eval
                    entityInvMap[ eval] = eid
                else:
                    location = getLocationCandidate( entitydef)
                    entityMap[ getNonBracketPrefix( entitydef)] = ""
                if location:
                    if location in locationMap:
                        locationMap[ location] += 1
                    else:
                        locationMap[ location] = 1
            else:
                entityReadState = False
                content += line
    if filename:
        if content:
            for location,count in locationMap.items():
                if count >= 3:
                    if not location in entityMap:
                        entityMap[ location] = location
                    if not location in entityInvMap:
                        entityInvMap[ location] = location
            title = getTitleFromFileName( filename)
            result = tagDocument( title, content, entityMap, entityInvMap, accuvar, verbose, complete)
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
    printTestResult( "MATCH 1", matchNameSubset( ["Giuliani"], ["Rudy","Giuliani"]))
    printTestResult( "MATCH 2", matchNameSubset( ["Rudy","Giuliani"], ["Rudolph", "William", "Louis", "Giuliani"]))
    printTestResult( "NOT MATCH 1", not matchNameSubset( ["Hugo","Chavez"], ["Hurto", "Castro"]))

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








