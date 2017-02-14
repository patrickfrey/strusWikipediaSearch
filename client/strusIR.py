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
        if scheme == "BM25pff" or scheme == "BM25std":
            rt.addWeightingFunction( "BM25pff", {
                     "k1": 1.2, "b": 0.75, "avgdoclen": 1000,
                     "metadata_doclen": "doclen",
                     "titleinc": 4.0, "windowsize": 40, 'cardinality': 2,
                     "ffbase": 0.4, "fftie": 20,
                     "proxffbias": 0.3, "proxfftie": 30, "maxdf": 0.2,
                     ".para": "para", ".struct": "sentence", ".match": "docfeat", ".title": "titlefield"
            })
            if scheme == "BM25std":
                rt.addWeightingFunction( "constant", {"precalc":1, ".match": "lnkfeat" } )
                rt.addWeightingFormula( "d * _1 + (1 - d) * _0", {"d": 0.4} )

        elif scheme == "BM25" or scheme == "BM25pg":
            rt.addWeightingFunction( "BM25", {
                     "k1": 1.2, "b": 0.75, "avgdoclen": 1000,
                     "metadata_doclen": "doclen",
                     ".match": "docfeat"
            })
            if scheme == "BM25pg":
                rt.addWeightingFunction( "metadata", {"name": "pageweight" } )
                rt.addWeightingFormula( "d * _0 * (_1 / 10) + (1 - d) * _0", {"d": 0.2} )

        elif scheme == "NBLNK" or scheme == "TILNK" or scheme == "VCLNK" or scheme == "STDLNK":
            rt.addWeightingFunction( "BM25", {
                     "k1": 1.2, "b": 0.75, "avgdoclen": 500,
                     "metadata_doclen": "doclen",
                     ".match": "docfeat"
            })
            rt.addWeightingFunction( "metadata", {"name": "pageweight" } )
            rt.addWeightingFormula( "d * _0 * (_1 / 10) + (1 - d) * _0", {"d": 0.2} )
        else:
            raise Exception( "unknown query evaluation scheme %s" % scheme)

        if scheme == "NBLNK":
            rt.addSummarizer( "accunear", {
                  "cofactor": 2.5, "type": "linkid", "range": 30, "cardinality": "75%",
                  "nofranks":20, "result":"LINK",
                  ".match": "docfeat"
            })
        elif scheme == "TILNK":
            rt.addSummarizer( "accunear", {
                  "cofactor": 2.5, "type": "veclfeat", "range": 30, "cardinality": "75%",
                  "nofranks":20, "result":"LINK",
                  ".match": "docfeat"
            })
        elif scheme == "VCLNK":
            rt.addSummarizer( "accunear", {
                  "cofactor": 2.5, "type": "vecfname", "range": 30, "cardinality": "75%",
                  "nofranks":20, "result":"LINK",
                  ".match": "docfeat"
            })
        elif scheme == "STDLNK":
            rt.addSummarizer( "accunear", {
                  "cofactor": 2.5, "type": "linkid", "range": 30, "cardinality": "75%",
                  "nofranks":20, "result":"TITLE",
                  ".match": "docfeat"
            })
            rt.addSummarizer( "accunear", {
                  "cofactor": 2.5, "type": "veclfeat", "range": 30, "cardinality": "75%",
                  "nofranks":20, "result":"LINK",
                  ".match": "docfeat"
            })
            # rt.addSummarizer( "accunear", {
            #      "cofactor": 1.2, "type": "vecfname", "range": 30, "cardinality": "75%",
            #      "nofranks":20, "result":"VECTOR",
            #      ".match": "docfeat"
            # })
        else:
            # Summarizer for getting the document title:
            rt.addSummarizer( "attribute", { "name": "docid" })
            # Summarizer for abstracting:
            rt.addSummarizer( "matchphrase", {
                  "type": "orig",
                  "windowsize": 40, "sentencesize": 100, "cardinality": "60%", "maxdf": 0.2,
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
        for scheme in [ "BM25", "BM25pg", "BM25pff", "BM25std", "NBLNK", "TILNK", "VCLNK", "STDLNK" ]:
            self.queryeval[ scheme] = self.createQueryEval( scheme)

    # Get pairs (a,b) of a and b in [0..N-1] with a < b:
    @staticmethod
    def getAscendingIndexPairs( N):
        for i in range(N):
            for k in range(i+1,N):
                yield [i,k]

    # Define features for weighting and summarization:
    def defineFeatures( self, scheme, query, seltitle, terms, links, collectionsize):
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
            if scheme != "NBLNK" and scheme != "TILNK" and scheme != "VCLNK" and scheme != "STDLNK" and selfeat2:
                selexpr2 = ["contains"] + selfeat2

        if not selexpr1 and not selexpr2:
            raise Exception( "query features not found in the collection")

        for term in terms:
            if term.cover:
                query.defineFeature( "docfeat", [term.type, term.value, term.length], term.weight)
                if term.df > 0.0:
                    query.defineTermStatistics( term.type, term.value, {'df' : int(term.df)} )

        for link in links:
            query.defineFeature( "lnkfeat", [link.type, link.value], link.weight)

        if selexpr1:
            query.defineFeature( "selfeat", selexpr1, 1.0 )
        if selexpr2:
            query.defineFeature( "selfeat", selexpr2, 1.0 )

        query.defineDocFieldFeature( "titlefield", "title_start", "title_end" )
        query.addMetaDataRestrictionCondition( "==", "redirect", 0, True)

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
        if scheme == "NBLNK" or scheme == "TILNK" or scheme == "VCLNK":
            for rank in result.ranks():
                links = []
                for sumelem in rank.summaryElements():
                    if sumelem.name() == 'LINK':
                        links.append( [sumelem.value().strip(), sumelem.weight()])
                rt.append( {
                       'docno':rank.docno(), 'weight':rank.weight(), 'links':links})
        elif scheme == "STDLNK":
            for rank in result.ranks():
                links = []
                vectors = []
                titles = []
                for sumelem in rank.summaryElements():
                    if sumelem.name() == 'LINK':
                        links.append( [sumelem.value().strip(), sumelem.weight()])
                    elif sumelem.name() == 'VECTOR':
                        vectors.append( [sumelem.value().strip(), sumelem.weight()])
                    elif sumelem.name() == 'TITLE':
                        titles.append( [sumelem.value().strip(), sumelem.weight()])
                rt.append( {
                       'docno':rank.docno(), 'weight':rank.weight(), 'links':links, 'vectors':vectors, 'titles':titles})
        else:
            if (debugtrace):
                print "pass %u, nof matches %u" %(result.evaluationPass(), result.nofDocumentsRanked())
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

