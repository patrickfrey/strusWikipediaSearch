import strus

class Backend:
	def __init__(self, storageconfig):
		self.context = strus.Context()
		self.context.addResourcePath("./resources")
		self.storage = self.context.createStorageClient( storageconfig )
		self.analyzer = self.context.createQueryAnalyzer()
		self.queryeval = self.context.createQueryEval()
		self.analyzer.definePhraseType(
			"text", "stem", "word", 
			["lc", ["dictmap", "irregular_verbs_en.txt"], ["stem", "en"], ["convdia", "en"], "lc"]
		)
		self.queryeval.addTerm( "sentence", "sent", "")
		self.queryeval.addSelectionFeature( "selfeat")

		self.queryeval.addWeightingFunction(
				1.0, "BM25", {
					"k1": 0.75, "b": 2.1, "avgdoclen": 500,
					".match": "docfeat"
				})
		self.queryeval.addSummarizer(
				"TITLE", "attribute", {
					"name": "title"
				})
		self.queryeval.addSummarizer(
				"CONTENT", "matchphrase", {
					"type": "orig", "len": 40, "nof": 3, "structseek": 30, "mark": '<b>$</b>',
					".struct": "sentence", ".match": "docfeat"
				})

	def evaluateQuery( self, querystr, firstrank, nofranks):
		query = self.queryeval.createQuery( self.storage)
		terms = self.analyzer.analyzePhrase( "text", querystr)

		if len( terms) > 0:
			for term in terms:
				query.pushTerm( term.type(), term.value())
				query.pushDuplicate()
				query.defineFeature( "docfeat")

			query.pushExpression( "contains", len(terms))
			query.defineFeature( "selfeat")

		query.setMaxNofRanks( nofranks)
		query.setMinRank( firstrank)
		return query.evaluate()


