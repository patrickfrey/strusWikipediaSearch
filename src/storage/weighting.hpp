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
#include "strus/weightingFunctionInstanceInterface.hpp"
#include "strus/weightingExecutionContextInterface.hpp"
#include "strus/metaDataReaderInterface.hpp"
#include "strus/storageClientInterface.hpp"
#include "strus/index.hpp"
#include "strus/postingIteratorInterface.hpp"
#include <vector>
#include <string>
#include <stdexcept>
#include <iostream>
#include <sstream>
#include <iomanip>
#include <stdlib.h>
#include <strings.h>

namespace strus
{

/// \note The weighting function introduced here was inspired by [1] http://research.microsoft.com/pubs/144542/ppm.pdf
///	I am using the reverse function (PPM-R) introduced there incremented with a document measure
///	calculated a the logarithm of relative number of citations a document has.

/// \class WeightingExecutionContext
/// \brief Weighting function context
class WeightingExecutionContext
	:public WeightingExecutionContextInterface
{
public:
	WeightingExecutionContext(
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


/// \class WeightingFunctionInstance
/// \brief Weighting function instance 
class WeightingFunctionInstance
	:public WeightingFunctionInstanceInterface
{
public:
	explicit WeightingFunctionInstance()
		:m_a(0.5),m_b(0.75),m_k1(1.5),m_avgdoclen(1000){}

	virtual ~WeightingFunctionInstance(){}

	static float getFloatValue( const std::string& val)
	{
		std::string::const_iterator vi = val.begin(), ve = val.end();
		if (vi != ve && *vi == '-') ++vi;
		for (; vi != ve && (*vi >= '0' && *vi <= '9'); ++vi){}
		if (vi != ve && *vi == '.') ++vi;
		for (; vi != ve && (*vi >= '0' && *vi <= '9'); ++vi){}
		if (vi != ve) throw std::runtime_error( "floating point argument expected");
		return (float)atof(val.c_str());
	}
	static bool caseInsensitiveEquals( const std::string& aa, const std::string& bb)
	{
		return (strcasecmp(aa.c_str(), bb.c_str()) == 0);
	}

	virtual void addStringParameter( const std::string& name, const std::string& value)
	{
		addNumericParameter( name, getFloatValue( value));
	}

	virtual void addNumericParameter( const std::string& name, const ArithmeticVariant& value)
	{
		if (caseInsensitiveEquals( name, "k1"))
		{
			m_k1 = (float)value;
		}
		else if (caseInsensitiveEquals( name, "a"))
		{
			m_a = (float)value;
		}
		else if (caseInsensitiveEquals( name, "b"))
		{
			m_b = (float)value;
		}
		else if (caseInsensitiveEquals( name, "avgdoclen"))
		{
			m_avgdoclen = (unsigned int)value;
		}
		else
		{
			throw std::runtime_error( "unknown weighting function parameter name");
		}
	}

	virtual WeightingExecutionContextInterface* createExecutionContext(
			const StorageClientInterface* storage_,
			PostingIteratorInterface* itr,
			MetaDataReaderInterface* metadata) const
	{
		return new WeightingExecutionContext( storage_, itr, metadata, m_a, m_b, m_k1, m_avgdoclen);
	}

	virtual std::string tostring() const
	{
		std::ostringstream rt;
		rt << std::setw(2) << std::setprecision(5)
			<< "a=" << m_a << "b=" << m_a << ", k1=" << m_k1 << ", avgdoclen=" << m_avgdoclen;
		return rt.str();
	}

private:
	float m_a;
	float m_b;
	float m_k1;
	unsigned int m_avgdoclen;
};

/// \class WeightingFunction
/// \brief Weighting function
class WeightingFunction
	:public WeightingFunctionInterface
{
public:
	explicit WeightingFunction(){}

	virtual ~WeightingFunction(){}

	virtual WeightingFunctionInstanceInterface* createInstance() const
	{
		return new WeightingFunctionInstance();
	}

	static WeightingFunctionInterface* create()
	{
		return new WeightingFunction();
	}
};

}//namespace
#endif

