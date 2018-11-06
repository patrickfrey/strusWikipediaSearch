#!/usr/bin/python3
#
import nltk
from pprint import pprint
import sys
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

spacy_nlp = en_core_web_sm.load()
NlpToken = recordtype('NlpToken', ['strustag','strusrole','nlptag','nlprole', 'value', 'ref'])
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
        return "E" # [entity] personal pronoun
    if tagname == "PRP$":
        return "E" # [entity] possesive pronoun
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
    if tagname == "_SP":
        return "" # empty
    sys.stderr.write( "unknown NLP tag %s\n" % tagname)
    return "?"

tgmaplist = [
 "CC", "CD", "DT", "EX", "FW", "IN", "JJ", "JJR", "JJS", "LS", "MD", "TO",
 "NNS", "NN", "NNP", "NNPS", "PDT", "POS", "PRP", "PRP$", "RB", "RBR", "RBS", "RP", "HYPH", "NFP", "AFX",
 "S", "SBAR", "SBARQ", "SINV", "SQ", "SYM", "VBD", "VBG", "VBN", "VBP", "VBZ", "VB", "ADD",
 "WDT", "WP", "WP$", "WRB", ".", ";", ":", ",", "-LRB-", "-RRB-", "$", "#" , "UH", "", "''", "``", "XX", "_SP"
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
        rt.append( name[ :pi+1])
        name = name[ pi+1:]
    if name:
        rt.append( name)
    return rt

def getSex( prp):
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

def matchName( obj, candidate, relaxed):
    cd = deepcopy(candidate)
    if not obj:
        return False
    for nam in obj:
        if relaxed or nam[ -1:] == '.':
            if nam[ -1:] == '.':
                prefix = nam[ :-1]
            elif relaxed:
                prefix = nam[0:2]
            found = False
            for eidx,elem in enumerate(cd):
                if len(elem) > len(prefix) and elem[ :len(prefix)] == prefix:
                    del cd[ eidx]
                    found = True
                    break
            if found:
                continue
        if nam in cd:
            del cd[ cd.index( nam)]
        else:
            return False
    return True

def sentenceHasVerb( tokens):
    hasVerb = False
    for elem in tokens:
        if elem.nlptag and elem.nlptag[0] == 'V':
            hasVerb = True
    return hasVerb

def sentenceHasEntitiesOnly( tokens):
    hasEntitiesOnly = True
    for elem in tokens:
        if elem.nlptag[:3] != 'NNP' and elem.nlprole != "punct":
            hasEntitiesOnly = False
    return hasEntitiesOnly

def getMapBestWeight( name2weightMap, name):
    bestCd = None
    bestWeight = 0
    for cd,weight in name2weightMap.items():
        thisCd = cd.split(' ')
        if matchName( name, thisCd, False):
            if weight > bestWeight or (weight == bestWeight and (not bestCd or len(bestCd) < len(thisCd))):
                bestCd = thisCd
                bestWeight = weight
    return bestCd

def getTitleSubject( title):
    endtitle = title.find('(')
    if endtitle >= 0:
        titleparts = title[ :endtitle ].split(' ')
    else:
        titleparts = title.split(' ')
    rt = []
    for tp in titleparts:
        rt += splitAbbrev( tp)
    return rt

def getMultipartName( tokens, tidx):
    rt = [tokens[tidx].value]
    ti = tidx + 1
    while ti < len(tokens) and tokens[ ti].nlptag == '_':
        rt.append( tokens[ ti].value)
        ti += 1
    return rt

def getMultipartNameStr( tokens, tidx):
    rt = tokens[tidx].value
    ti = tidx + 1
    while ti < len(tokens) and tokens[ ti].nlptag == '_':
        rt += " " + tokens[ ti].value
        ti += 1
    return rt

def getStringListListKey( ar):
    rt = ""
    for ls in ar:
        if rt:
            rt += ","
        rt += ' '.join( ls)
    return rt

def tagSentenceStrusTags( tokens):
    prev = ""
    mapprev = ""
    for eidx,elem in enumerate(tokens):
        type = elem.nlptag
        utype = unifyType( type)
        maptype = mapTag( type)
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
        elif elem.nlptag == 'PRP' and elem.nlprole == "nsubj":
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
                if ei >= 0 and tokens[ei].nlprole == "nsubj":
                    elem.strusrole = "S"
            elif elem.nlprole == "nsubj":
                elem.strusrole = "S"
            if not elem.strusrole:
                ei = eidx+1
                while ei < len(tokens) and tokens[ei].nlptag == '_':
                    if tokens[ei].nlprole == "nsubj":
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

def tagSentenceNounReferences( tokens, titlesubject, bestTitleMatches, nounCandidates, sentenceIdx):
    for eidx,elem in enumerate( tokens):
        if elem.nlptag[:2] == 'NN':
           name = getMultipartName( tokens, eidx)
           if sentenceIdx == 0 and elem.nlptag[:3] == "NNP":
               if matchName( titlesubject, name, True):
                   elem.ref = titlesubject
                   sentenceIdx = -1
           if name in bestTitleMatches:
               elem.ref = titlesubject
           elem.ref = elem.ref or getMapBestWeight( nounCandidates, name)
           if elem.ref:
               key = ' '.join( elem.ref)
               nounCandidates[ key] += 1
               for cd in nounCandidates:
                   nounCandidates[ key] *= 0.8

# param sexSubjectMap: map sex:string -> Subject
def tagSentencePrpReferences( tokens, sentidx, sexSubjectMap, nnpSexMap):
    subjects = []
    for eidx,elem in enumerate( tokens):
        if elem.nlptag[:2] == 'NN' and elem.strusrole == "S":
            name = elem.ref or getMultipartName( tokens, eidx)
            namekey = ' '.join(name)
            sex = None
            if elem.nlptag == "NNPS" or elem.nlptag == "NNS":
                sex = "P"
            elif elem.nlptag == "NN":
                if elem.value == "man":
                    sex = "M"
                elif elem.value == "woman":
                    sex = "W"
                else:
                    sex = "N"
            elif namekey in nnpSexMap:
                sex = nnpSexMap[ namekey]
            matchprev = False
            if subjects:
                for sb in subjects:
                    if sb.sex == sex and sb.strustag == elem.strustag and (matchName( name, sb.value, False) or matchName( sb.value, name, False)):
                        matchprev = True
            if not matchprev:
                subjects.append( Subject( sex, elem.strustag, name, sentidx))
        elif elem.nlptag[:3] == 'PRP' and not elem.ref:
           sex = getSex( elem.value)
           if sex:
               if sex in sexSubjectMap:
                   subj = sexSubjectMap[ sex]
                   if sentidx - subj.sentidx < 7:
                       subj.sentidx = sentidx
                       elem.ref = subj.value
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
    inBracketLevel = 0
    inBracketToks = 0
    for node in tg:
        value = str(node).strip()
        if value == '(' and node.tag_ == '-LRB-':
            inBracketLevel += 1
            inBracketToks = 0
        elif value == ')' and node.tag_ == '-RRB-' and inBracketLevel > 0:
            inBracketLevel -= 1
            inBracketToks = 0
        else:
            inBracketToks += 1
        if value:
            if node.tag_[:3] == "NNP" and value.count('.') > 1:
                for abr in splitAbbrev( value):
                    tokens.append( NlpToken( None, None, node.tag_, node.dep_, abr, None))
            else:
                tokens.append( NlpToken( None, None, node.tag_, node.dep_, value, None))
        if node.dep_ == "punct":
            if inBracketLevel == 0 and inBracketToks < 10:
                if value in [';','.']:
                    if verbose:
                        print( "* Sentence %s" % ' '.join( [tk.value for tk in tokens]))
                    sentences.append( tokens)
                    tokens = []
                    inBracketToks = 0
                    inBracketLevel = 0
            else:
                if value == '.':
                    if verbose:
                        print( "* Sentence %s" % ' '.join( [tk.value for tk in tokens]))
                    sentences.append( tokens)
                    tokens = []
                    inBracketToks = 0
                    inBracketLevel = 0
    if tokens:
        sentences.append( tokens)
    return sentences

def getDocumentNlpTokCountMap( sentences, elements):
    rt = {}
    for sent in sentences:
        for tidx,tok in enumerate(sent):
            if tok.nlptag in elements:
                key = getMultipartNameStr( sent, tidx)
                if key in rt:
                    rt[ key] += 1
                else:
                    rt[ key] = 1
    return rt

def getBestTitleMatches( titlesubject, tokCountMap):
    maxTf = 0
    bestName = None
    for key,tf in tokCountMap.items():
        name = key.split(' ')
        if matchName( name, titlesubject, False) and (maxTf < tf or (maxTf == tf and len(key) < len(bestKey))):
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

# param titlesubject string[]
# param sentences NlpToken[][]
# return map NNP -> string[] = PRP list
def getDocumentPrpMap( titlesubject, sentences):
    rt = {}
    sidx = 0
    lastSentSubjects = [titlesubject]
    for sent in sentences:
        sentSubjects = []
        for tidx,tok in enumerate(sent):
            if tok.strusrole == 'S' and tok.nlptag[0:2] == "NN":
                subj = tok.ref or getMultipartName( sent, tidx)
                sentSubjects.append( subj)
            elif tok.nlptag[0:3] == "PRP":
                key = getStringListListKey( lastSentSubjects)
                if key in rt:
                    rt[ key].append( tok.value)
                else:
                    rt[ key] = [tok.value]
        lastSentSubjects = sentSubjects
    return rt

def getMaxSexCount( prplist):
    cntmap = {'M':0, 'W':0, 'N':0, 'P':0}
    for prp in prplist:
        sex = getSex(prp)
        if sex:
            cntmap[ sex] += 1
    max = 0
    rt = None
    for sex,cnt in cntmap.items():
        if cnt > max or (cnt == max and sex in ["M","W"]):
            max = cnt
            rt = sex
    return rt

def getDocumentNnpSexMap( prpmap):
    rt = {}
    for nnp,prp in prpmap.items():
        maxsex = getMaxSexCount( prp)
        if maxsex:
            rt[ nnp] = maxsex
    return rt

def tagDocument( title, text, verbose, complete):
    rt = ""
    titlesubject = getTitleSubject( title)
    if verbose:
        print( "* Document title %s" % ' '.join(titlesubject))
    sentences = getDocumentSentences( text, verbose)
    for sent in sentences:
        if sentenceHasVerb( sent) or sentenceHasEntitiesOnly( sent) or len(sent) > 6:
            tagSentenceStrusTags( sent)
    countNnp = getDocumentNlpTokCountMap( sentences, ["NNP","NNPS"])
    countNn = getDocumentNlpTokCountMap( sentences, ["NN","NNS"])
    bestTitleMatches = getBestTitleMatches( titlesubject, countNnp)
    if verbose:
        for bm in bestTitleMatches:
            print( "* Best title match %s" % ' '.join(bm))
    nounCandidates = {}
    titlekey = ' '.join( titlesubject)
    nounCandidates[ titlekey] = 5
    for sidx,sent in enumerate(sentences):
        if sentenceHasVerb( sent):
            tagSentenceSubjects( sent)
            tagSentenceNounReferences( sent, titlesubject, bestTitleMatches, nounCandidates, sidx)

    prpmap = getDocumentPrpMap( titlesubject, sentences)
    nnpSexMap = getDocumentNnpSexMap( prpmap)
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
        for subj,sex in nnpSexMap.items():
            print( "* Entity sex %s -> %s" % (subj, sex))
        for subj,prp in prpmap.items():
            print( "* Subject %s -> %s" % (subj, ','.join( prp)))
        for key in countNn:
            print( "* Noun usage %s # %d" % (key, countNn[ key]))
        for key in countNnp:
            print( "* Entity usage %s # %d" % (key, countNnp[ key]))
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
        opts, args = getopt.getopt( argv,"hTVKSC:",["chunksize=","status","verbose","complete","test"])
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
            elif opt in ("-T", "--test"):
                rt['T'] = True
            elif opt in ("-S", "--status"):
                rt['S'] = True
        return rt
    except getopt.GetoptError:
        sys.stderr.write( "bad arguments\n")
        printUsage()
        sys.exit(2)

def processStdin( verbose, complete):
    content = ""
    filename = ""
    for line in sys.stdin:
        if len(line) > 6 and line[0:6] == '#FILE#':
            if content:
                title = getTitleFromFileName( filename)
                result = tagDocument( title, content, verbose, complete)
                printOutput( filename, result)
                content = ""
            filename = line[6:].rstrip( "\r\n")
        else:
            content += line
    if content:
        title = getTitleFromFileName( filename)
        result = tagDocument( title, content, verbose, complete)
        printOutput( filename, result)
        content = ""

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

def runTest():
    printTestResult( "MATCH 1", matchName( ["Giuliani"], ["Rudy","Giuliani"], False))
    printTestResult( "MATCH 2", matchName( ["Rudy","Giuliani"], ["Rudolph", "William", "Louis", "Giuliani"], True))

if __name__ == "__main__":
    argmap = parseProgramArguments( sys.argv[ 1:])
    chunkSize = 0
    verbose = False
    complete = False

    if 'V' in argmap:
        verbose = True
    if 'K' in argmap:
        complete = True
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
        processStdin( verbose, complete)








