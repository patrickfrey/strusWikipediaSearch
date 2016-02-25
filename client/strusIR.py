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
            rt.addWeightingFunction( 1.0, "BM25pff", {
                     "k1": 1.5, "b": 0.75, "avgdoclen": 500,
                     "metadata_title_maxpos": "maxpos_title", "metadata_doclen": "doclen",
                     "titleinc": 2.4, "windowsize": 40, 'cardinality': 0, "ffbase": 0.4,
                     "maxdf": 0.4,
                     ".para": "para", ".struct": "sentence", ".match": "docfeat"
            })
            rt.addWeightingFunction( 1.0, "metadata", {"name": "pageweight" } )

        elif scheme == "BM25":
            rt.addWeightingFunction( 1.0, "BM25", {
                     "k1": 1.5, "b": 0.75, "avgdoclen": 500,
                     "metadata_doclen": "doclen",
                     ".match": "docfeat"
            })
            rt.addWeightingFunction( 1.0, "metadata", {"name": "pageweight" } )

        elif scheme == "NBLNK":
                rt.addWeightingFunction( 1.0, "BM25", {
                         "k1": 1.5, "b": 0.75, "avgdoclen": 500,
                         "metadata_doclen": "doclen",
                         ".match": "docfeat"
                })
        else:
            raise Exception( "unknown query evaluation scheme %s" % scheme)

        if scheme == "NBLNK":
            rt.addSummarizer( "accuvariable", {
                  "norm": 0.001, "var": "LINK", "type": "linkid",
                  ".match": "sumfeat"
            })
        else:
            # Summarizer for getting the document title:
            rt.addSummarizer( "attribute", { "name": "docid" })
            # Summarizer for abstracting:
            rt.addSummarizer( "matchphrase", {
                  "type": "orig", "metadata_title_maxpos": "maxpos_title",
                  "windowsize": 40, "sentencesize": 100, "cardinality": 2,
                  "matchmark": '$<b>$</b>',
                  ".struct": "sentence", ".match": "docfeat", ".para": "para"
            })
        return rt

    # Constructor. Initializes the query evaluation schemes and the query and document analyzers:
    def __init__(self, config):
        # Open local storage on file with configuration specified:
        self.context = strus.Context()
        self.context.defineStatisticsProcessor( "standard")
        self.storage = self.context.createStorageClient( config )
        self.queryeval = {}
        for scheme in [ "BM25", "BM25pff", "NBLNK" ]:
            self.queryeval[ scheme] = self.createQueryEval( scheme)

    # Get pairs (a,b) of a and b in [0..N-1] with a < b:
    @staticmethod
    def getAscendingIndexPairs( N):
        for i in range(N):
            for k in range(i+1,N):
                yield [i,k]

    # Define features for weighting and summarization:
    def defineFeatures( self, scheme, query, terms):
        selexpr = ["contains"]
        for term in terms:
            selexpr.append( [term.type, term.value] )
            query.defineFeature( "docfeat", [term.type, term.value], 1.0)
            query.defineTermStatistics( term.type, term.value, {'df' : int(term.df)} )

        query.defineFeature( "selfeat", selexpr, 1.0 )
        if scheme == "NBLNK" and len( terms) > 0:
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
                        weight = [ 3.0, 2.0, 2.0, 1.5 ]
                        for ii in range(4):
                            sumexpr = [ "chain_struct", 50, ["sent"], ["=LINK", "linkvar"], expr[ ii] ]
                            query.defineFeature( "sumfeat", sumexpr, weight[ ii] )
                            sumexpr = [ "sequence_struct", -50, ["sent"], expr[ ii], ["=LINK", "linkvar"] ]
                            query.defineFeature( "sumfeat", sumexpr, weight[ ii] )

                    elif pair[0]+2 < pair[1]:
                        # ... far away terms in query:
                        expr = [ "within_struct", 20, ["sent"], [term1.type,term1.value] [term2.type,term2.value]]
                        weight = 1.1
                        sumexpr = [ "inrange_struct", 50, ["sent"], ["=LINK", "linkvar"], expr ]
                        query.defineFeature( "sumfeat", sumexpr, weight )
                    else:
                        expr = [
                            [ "within_struct",  5,  ["sent"], [term1.type,term1.value], [term2.type,term2.value]],
                            [ "within_struct", 20,  ["sent"], [term1.type,term1.value], [term2.type,term2.value]]
                        ]
                        weight = [ 1.6, 1.2 ]
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
    def evaluateQuery( self, scheme, terms, collectionsize, firstrank, nofranks):
        if not scheme in self.queryeval:
            raise Exception( "unknown query evaluation scheme %s" % scheme)
        queryeval = self.queryeval[ scheme]
        query = queryeval.createQuery( self.storage)
        if len( terms) == 0:
            # Return empty result for empty query:
            return []

        self.defineFeatures( scheme, query, terms)
        query.setMaxNofRanks( nofranks)
        query.setMinRank( firstrank)
        query.defineGlobalStatistics( {'nofdocs' : int(collectionsize)} )

        # Evaluate the query:
        result = query.evaluate()
        rt = []
        # Rewrite the results:
        if scheme == 'NBLNK':
            for rank in result.ranks():
                links = []
                for sumelem in rank.summaryElements():
                    if sumelem.name() == 'LINK':
                        links.append( [sumelem.value().strip(), sumelem.weight()])
                rt.append( {
                       'docno':rank.docno(), 'weight':rank.weight(), 'links':links})
        else:
            for rank in result.ranks():
                content = ""
                title = ""
                for sumelem in rank.summaryElements():
                    if sumelem.name() == 'phrase' or sumelem.name() == 'docstart':
                        if content != "":
                            content += ' --- '
                        content += sumelem.value()
                    elif sumelem.name() == 'docid':
                        title = sumelem.value()
                rt.append( {
                       'docno':rank.docno(), 'title':title,
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


