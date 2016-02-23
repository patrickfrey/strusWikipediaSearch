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
        elif scheme == "BM25":
            rt.addWeightingFunction( 1.0, "BM25", {
                     "k1": 1.5, "b": 0.75, "avgdoclen": 500,
                     "metadata_doclen": "doclen",
                     ".match": "docfeat"
            })
        else:
            raise Exception( "unknown query evaluation scheme")
        # Summarizer for getting the document title:
        rt.addSummarizer( "DOCID", "attribute", { "name": "docid" })
        rt.addSummarizer( "TITLE", "attribute", { "name": "title" })
        # Summarizer for abstracting:
        rt.addSummarizer( "CONTENT", "matchphrase", {
                  "type": "orig", "metadata_title_maxpos": "maxpos_title",
                  "windowsize": 40, "sentencesize": 100, "cardinality": 3,
                  "matchmark": '$<b>$</b>',
                  ".struct": "sentence", ".match": "docfeat", ".para": "para"
        })
        return rt

    # Constructor. Initializes the query evaluation schemes and the query and document analyzers:
    def __init__(self, config):
        # Open local storage on file with configuration specified:
        self.context = strus.Context()
        self.context.defineStatisticsProcessor( "standard");
        self.storage = self.context.createStorageClient( config )
        self.queryeval = {}
        for scheme in [ "BM25", "BM25pff"]:
            self.queryeval[ scheme] = self.createQueryEval( scheme)

    # Query evaluation scheme for a classical information retrieval query with BM25:
    def evaluateQuery( self, scheme, terms, collectionsize, firstrank, nofranks):
        queryeval = self.queryeval[ scheme]
        if not queryeval:
            raise Exception( "unknown query evaluation scheme")
        query = queryeval.createQuery( self.storage)
        if len( terms) == 0:
            # Return empty result for empty query:
            return []

        selexpr = ["contains"]
        for term in terms:
            selexpr.append( [term.type, term.value] )
            query.defineFeature( "docfeat", [term.type, term.value], 1.0)
            query.defineTermStatistics( term.type, term.value, {'df' : int(term.df)} )
        query.defineFeature( "selfeat", selexpr, 1.0 )
        query.setMaxNofRanks( nofranks)
        query.setMinRank( firstrank)
        query.defineGlobalStatistics( {'nofdocs' : int(collectionsize)} )
        # Evaluate the query:
        result = query.evaluate()
        # Rewrite the results:
        rt = []
        for rank in result.ranks():
            content = ""
            title = ""
            docid = ""
            for attribute in rank.attributes():
                if attribute.name() == 'CONTENT':
                    if content != "":
                        content += ' ... '
                    content += attribute.value()
                elif attribute.name() == 'DOCID':
                        docid = attribute.value()
                elif attribute.name() == 'TITLE':
                        title = attribute.value()
            rt.append( {
                   'docno':rank.docno(),
                   'docid':docid,
                   'title':title,
                   'weight':rank.weight(),
                   'abstract':content })
        return rt

    # Get an iterator on all absolute statistics of the storage
    def getInitStatisticsIterator( self):
        return self.storage.createInitStatisticsIterator( True)

    # Get an iterator on all absolute statistics of the storage
    def getDoneStatisticsIterator( self):
        return self.storage.createInitPeerMessageIterator( False)
    
    # Get an iterator on statistic updates of the storage
    def getUpdateStatisticsIterator( self):
        return self.storage.createUpdateStatisticsIterator()


