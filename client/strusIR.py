import strus
import itertools
import heapq
import re

class Backend:
    # Create a query evaluation scheme:
    def createQueryEval( self, scheme):
        rt = self.context.createQueryEval()
        # Declare the sentence marker feature needed for abstracting:
        rt.addTerm( "sentence", "sent", "")
        rt.addTerm( "para", "para", "")

        # Declare the feature used for selecting result candidates:
        rt.addSelectionFeature( "selfeat")

        # Query evaluation scheme:
        if scheme == "BM25pff":
            rt.addWeightingFunction( "BM25pff", {
                     "k1": 1.2, "b": 0.75, "avgdoclen": 1000,
                     "metadata_doclen": "doclen",
                     "titleinc": 2.4, "tidocnorm": 100, "windowsize": 40, 'cardinality': 2,
                     "ffbase": 0.1, "fftie": 10,
                     "proxffbias": 0.3, "proxfftie": 20, "maxdf": 0.2,
                     ".para": "para", ".struct": "sentence", ".match": "docfeat", ".title": "titlefield"
            })
            rt.addWeightingFunction( "constant", {".match": "lnkfeat" } )
            rt.addWeightingFormula( "d * _0 * _1 + (1 - d) * _0", {"d": 0.4} )

        elif scheme == "BM25" or scheme == "BM25pg":
            rt.addWeightingFunction( "BM25", {
                     "k1": 1.2, "b": 0.75, "avgdoclen": 1000,
                     "metadata_doclen": "doclen",
                     ".match": "docfeat"
            })
            if scheme == "BM25pg":
                rt.addWeightingFunction( "metadata", {"name": "pageweight" } )
                rt.addWeightingFormula( "d * _0 * (_1 / 10) + (1 - d) * _0", {"d": 0.2} )

        elif scheme == "NBLNK" or scheme == "TILNK":
            rt.addWeightingFunction( "BM25", {
                     "k1": 1.2, "b": 0.75, "avgdoclen": 500,
                     "metadata_doclen": "doclen",
                     ".match": "docfeat"
            })
            rt.addWeightingFunction( "metadata", {"name": "pageweight" } )
        else:
            raise Exception( "unknown query evaluation scheme %s" % scheme)

        if scheme == "NBLNK":
            rt.addSummarizer( "accuvariable", {
                  "norm": 0.0001, "pairmul": 3.0, "var": "LINK", "type": "linkid",
                  ".match": "sumfeat"
            })
        elif scheme == "TILNK":
                rt.addSummarizer( "accuvariable", {
                      "norm": 0.0001, "var": "LINK",
                      ".match": "sumfeat",
                      "type": "veclfeat",
                })
        else:
            # Summarizer for getting the document title:
            rt.addSummarizer( "attribute", { "name": "docid" })
            # Summarizer for abstracting:
            rt.addSummarizer( "matchphrase", {
                  "type": "orig",
                  "windowsize": 40, "sentencesize": 100, "cardinality": 3, "maxdf": 0.2,
                  "matchmark": '$<b>$</b>',
                  ".struct": "sentence", ".match": "docfeat", ".para": "para", ".title": "titlefield"
            })
        return rt

    # Constructor. Initializes the query evaluation schemes and the query and document analyzers:
    def __init__(self, config):
        # Open local storage on file with configuration specified:
        self.context = strus.Context()
        self.storage = self.context.createStorageClient( config )
        self.queryeval = {}
        for scheme in [ "BM25", "BM25pg", "BM25pff", "NBLNK", "TILNK" ]:
            self.queryeval[ scheme] = self.createQueryEval( scheme)

    # Get pairs (a,b) of a and b in [0..N-1] with a < b:
    @staticmethod
    def getAscendingIndexPairs( N):
        for i in range(N):
            for k in range(i+1,N):
                yield [i,k]

    # Define features for weighting and summarization:
    def defineFeatures( self, scheme, query, seltitle, terms, links, collectionsize):
        for term in terms:
            print "++++ TERM %s %s %u %u %u" % (term.type,term.value,term.pos,term.len,term.cover)
        selexpr1 = []
        selexpr2 = []
        if seltitle == True:
            selfeat = []
            for term in terms:
                if term.df > 0.0:
                    selfeat.append( ["tist", term.value] )
            cardinality = 0
            if len( selfeat) >= 3:
                if len( selfeat) == 3:
                    cardinality = 2
                else:
                    cardinality = 3
            if selfeat:
                selexpr1 = ["contains",0,cardinality] + selfeat
        else:
            selfeat1 = []
            selfeat2 = []
            for term in terms:
                if term.df > 0.0:
                    if term.cover:
                        selfeat1.append( [term.type, term.value] )
                        if term.type == "stem":
                            selfeat2.append( [term.type, term.value] )
                    else:
                        selfeat2.append( [term.type, term.value] )
            if selfeat1:
                selexpr1 = ["contains"] + selfeat1
            if scheme != "NBLNK" and scheme != "TILNK" and selfeat2:
                selexpr2 = ["contains"] + selfeat2

        if not selexpr1 and not selexpr2:
            raise Exception( "query features not found in the collection")

        if scheme == "NBLNK" or scheme == "TILNK":
            queryterms = []
            for term in terms:
                if term.cover:
                    queryterms.append( term)
                    print "+++ CALL defineFeature %s %s" % (term.type, term.value)
                    query.defineFeature( "docfeat", [term.type, term.value], term.weight)
                    if term.df > 0.0:
                        query.defineTermStatistics( term.type, term.value, {'df' : int(term.df)} )
            terms = queryterms
        else:
            for term in terms:
                print "+++ CALL defineFeature %s %s" % (term.type, term.value)
                query.defineFeature( "docfeat", [term.type, term.value], term.weight)
                if term.df > 0.0:
                    query.defineTermStatistics( term.type, term.value, {'df' : int(term.df)} )

        for link in links:
            query.defineFeature( "lnkfeat", [link.type, link.value], link.weight)

        print "+++ SEL EXPR 1: %s" % selexpr1
        print "+++ SEL EXPR 2: %s" % selexpr2
        query.defineFeature( "selfeat", selexpr1, 1.0 )
        if selexpr2:
            query.defineFeature( "selfeat", selexpr2, 1.0 )

        query.defineDocFieldFeature( "titlefield", "title_start", "title_end" )
        query.addMetaDataRestrictionCondition( "==", "redirect", 0, True);

        if scheme == "NBLNK" or scheme == "TILNK" and len( terms) > 0:
            if len( terms) > 1:
                for pair in self.getAscendingIndexPairs( len( terms)):
                    term1 = terms[ pair[ 0]]
                    term2 = terms[ pair[ 1]]
                    if pair[0]+1 == pair[1]:
                        # ... subsequent terms in query:
                        expr = [
                            [ "sequence_struct", 3, ["sent"], [term1.type,term1.value], [term2.type,term2.value]],
                            [ "sequence_struct", 3, ["sent"], [term2.type,term2.value], [term1.type,term1.value]],
                            [ "within_struct",  5,  ["sent"], [term1.type,term1.value], [term2.type,term2.value]],
                            [ "within_struct", 20,  ["sent"], [term1.type,term1.value], [term2.type,term2.value]]
                        ]
                        weight = [ 4.0, 2.5, 2.0, 1.5 ]
                        for ii in range(4):
                            sumexpr = [ "chain_struct", 50, ["sent"], ["=LINK", "linkvar"], expr[ ii] ]
                            query.defineFeature( "sumfeat", sumexpr, weight[ ii] )
                            sumexpr = [ "sequence_struct", -50, ["sent"], expr[ ii], ["=LINK", "linkvar"] ]
                            query.defineFeature( "sumfeat", sumexpr, weight[ ii] )

                    elif pair[0]+2 < pair[1]:
                        # ... far away terms in query:
                        expr = [ "within_struct", 20, ["sent"], [term1.type,term1.value], [term2.type,term2.value]]
                        weight = 1.1
                        sumexpr = [ "inrange_struct", 50, ["sent"], ["=LINK", "linkvar"], expr ]
                        query.defineFeature( "sumfeat", sumexpr, weight )
                    else:
                        expr = [
                            [ "within_struct",  5,  ["sent"], [term1.type,term1.value], [term2.type,term2.value]],
                            [ "within_struct", 20,  ["sent"], [term1.type,term1.value], [term2.type,term2.value]]
                        ]
                        weight = [ 1.9, 1.4 ]
                        for ii in range(2):
                            sumexpr = [ "chain_struct", 50, ["sent"], ["=LINK", "linkvar"], expr[ ii] ]
                            query.defineFeature( "sumfeat", sumexpr, weight[ ii] )
                            sumexpr = [ "sequence_struct", -50, ["sent"], expr[ ii], ["=LINK", "linkvar"] ]
                            query.defineFeature( "sumfeat", sumexpr, weight[ ii] )
            else:
                # ... single term query
                expr = [ terms[0].type, terms[0].value ]
                sumexpr = [ "chain_struct", 50, ["sent"], ["=LINK","linkvar"], expr ]
                query.defineFeature( "sumfeat", sumexpr, 1.0 )
                sumexpr = [ "sequence_struct", -50, ["sent"], expr, ["=LINK","linkvar"] ]
                query.defineFeature( "sumfeat", sumexpr, 1.0 )

    # Query evaluation scheme for a classical information retrieval query with BM25:
    def evaluateQuery( self, scheme, seltitle, terms, links, collectionsize, firstrank, nofranks, restrictset, debugtrace):
        if not scheme in self.queryeval:
            raise Exception( "unknown query evaluation scheme %s" % scheme)
        queryeval = self.queryeval[ scheme]
        query = queryeval.createQuery( self.storage)
        if len( terms) == 0:
            # Return empty result for empty query:
            return []

        self.defineFeatures( scheme, query, seltitle, terms, links, collectionsize)
        query.setMaxNofRanks( nofranks)
        query.setMinRank( firstrank)
        query.defineGlobalStatistics( {'nofdocs' : collectionsize} )
        if restrictset:
            query.addDocumentEvaluationSet( restrictset )
        # Evaluate the query:
        result = query.evaluate()
        rt = []
        # Rewrite the results:
        if scheme == "NBLNK" or scheme == "TILNK":
            for rank in result.ranks():
                links = []
                for sumelem in rank.summaryElements():
                    if sumelem.name() == 'LINK':
                        links.append( [sumelem.value().strip(), sumelem.weight()])
                rt.append( {
                       'docno':rank.docno(), 'weight':rank.weight(), 'links':links})
        else:
            if (debugtrace):
                print( "pass %u, nof matches %u" %(result.evaluationPass(), result.nofDocumentsRanked()))
            for rank in result.ranks():
                content = ""
                title = ""
                paratitle = ""
                for sumelem in rank.summaryElements():
                    if sumelem.name() == 'phrase' or sumelem.name() == 'docstart':
                        if content != "":
                            content += ' --- '
                        content += sumelem.value()
                    elif sumelem.name() == 'para':
                        paratitle = sumelem.value()
                    elif sumelem.name() == 'docid':
                        title = sumelem.value()
                rt.append( {
                       'docno':rank.docno(), 'title':title, 'paratitle':paratitle,
                       'weight':rank.weight(), 'abstract':content
                })
        return rt

    # Get an iterator on all absolute statistics of the storage
    def getInitStatisticsIterator( self):
        return self.storage.createInitStatisticsIterator( True)

    # Get an iterator on all absolute statistics of the storage
    def getDoneStatisticsIterator( self):
        return self.storage.createInitStatisticsIterator( False)

    # Get an iterator on statistic updates of the storage
    def getUpdateStatisticsIterator( self):
        return self.storage.createUpdateStatisticsIterator()

    # Load the list of docno of documents with a pageweight higher than specified
    def getMinimumPageweightDocnos( self, minpageweight):
        rt = []
        browse = self.storage.createDocumentBrowser()
        browse.addMetaDataRestrictionCondition( ">=", "pageweight", minpageweight, True)
        docno = browse.skipDoc( 0)
        while (docno > 0):
            rt.append( docno)
            docno = browse.skipDoc( docno+1)
        return rt

