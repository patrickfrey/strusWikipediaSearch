/*
---------------------------------------------------------------------
    The C++ library strus implements basic operations to build
    a search engine for structured search on unstructured data.

    Copyright (C) 2013,2014 Patrick Frey

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA

--------------------------------------------------------------------

	The latest version of strus can be found at 'http://github.com/patrickfrey/strus'
	For documentation see 'http://patrickfrey.github.com/strus'

--------------------------------------------------------------------
*/
#ifndef _STRUS_WIKIPEDIA_SEARCH_WEIGHTING_HPP_INCLUDED
#define _STRUS_WIKIPEDIA_SEARCH_WEIGHTING_HPP_INCLUDED
#include "strus/weightingFunctionInterface.hpp"
#include "strus/weightingClosureInterface.hpp"
#include "strus/metaDataReaderInterface.hpp"
#include "strus/storageClientInterface.hpp"
#include "strus/index.hpp"
#include "strus/postingIteratorInterface.hpp"
#include <vector>

namespace strus
{

/// \brief Forward declaration
class WeightingFunction;

/// \note The weighting function introduced here was inspired by [1] http://research.microsoft.com/pubs/144542/ppm.pdf
///	I am using the reverse function (PPM-R) introduced there incremented with a document measure
///	calculated a the logarithm of relative number of citations a document has.

/// \class WeightingClosure
/// \brief Weighting function context
class WeightingClosure
	:public WeightingClosureInterface
{
public:
	WeightingClosure(
		const StorageClientInterface* storage,
		PostingIteratorInterface* itr_,
		MetaDataReaderInterface* metadata_,
		float k1_,		///... the BM25 factor 'k1'
		float b_,		///... the BM25 factor 'b'
		float a_,		///... the factor 'a' in 'g(x) = 1.0 / (a * x + 1.0)' (see [1])
		float avgDocLength_);	///... the BM25 average document length

	virtual float call( const Index& docno);

private:
	struct SubExpressionData
	{
		SubExpressionData( PostingIteratorInterface* postings_, float idf_)
			:postings(postings_),idf(idf_){}
		SubExpressionData( const SubExpressionData& o)
			:postings(o.postings),idf(o.idf){}

		PostingIteratorInterface* postings;
		float idf;
	};
	static float calc_idf( float nofCollectionDocuments, float nofMatches);

private:
	enum {MaxNofFeatures=128};
	float m_k1;
	float m_b;
	float m_a;
	float m_avgDocLength;
	float m_idf;
	PostingIteratorInterface* m_itr;
	std::vector<SubExpressionData> m_subexpressions;
	MetaDataReaderInterface* m_metadata;
	int m_metadata_doclen;
	int m_metadata_docweight;
};

/// \class WeightingFunction
/// \brief Weighting function
class WeightingFunction
	:public WeightingFunctionInterface
{
public:
	explicit WeightingFunction(){}

	virtual ~WeightingFunction(){}

	virtual const char** numericParameterNames() const
	{
		static const char* ar[] = {"k1","b","a","avgdoclen",0};
		return ar;
	}

	virtual WeightingClosureInterface* createClosure(
			const StorageClientInterface* storage_,
			PostingIteratorInterface* itr,
			MetaDataReaderInterface* metadata,
			const std::vector<ArithmeticVariant>& parameters) const
	{
		float k1	= parameters[0].defined()?(float)parameters[0]:1.5;
		float b		= parameters[1].defined()?(float)parameters[1]:0.75;
		float a		= parameters[2].defined()?(float)parameters[2]:0.5;
		float avgdoclen = parameters[3].defined()?(float)parameters[3]:1000;

		return new WeightingClosure( storage_, itr, metadata, b, a, k1, avgdoclen);
	}

	static WeightingFunctionInterface* create()
	{
		return new WeightingFunction();
	}
};

}//namespace
#endif

