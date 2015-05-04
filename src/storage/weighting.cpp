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
#include "weighting.hpp"
#include "strus/constants.hpp"
#include <cmath>
#include <stdexcept>
#include <ctime>

using namespace strus;

float WeightingExecutionContext::calc_idf( float nofCollectionDocuments, float nofMatches)
{
	float idf = 0.0;
	if (nofCollectionDocuments > nofMatches * 2)
	{
		idf = logf(
			(nofCollectionDocuments - nofMatches + 0.5)
			/ (nofMatches + 0.5));
	}
	if (idf < 0.00001)
	{
		idf = 0.00001;
	}
	return idf;
}

WeightingExecutionContext::WeightingExecutionContext(
		const StorageClientInterface* storage,
		PostingIteratorInterface* itr_,
		MetaDataReaderInterface* metadata_,
		float k1_,
		float b_,
		float a_,
		float avgDocLength_)
	:m_k1(k1_),m_b(b_),m_a(a_),m_avgDocLength(avgDocLength_)
	,m_idf(0.0f),m_itr(itr_),m_metadata(metadata_)
	,m_metadata_doclen(metadata_->elementHandle( Constants::metadata_doclen()))
{
	float nofCollectionDocuments = storage->globalNofDocumentsInserted();

	std::vector<const PostingIteratorInterface*> sear = m_itr->subExpressions( true);
	if (sear.size() > MaxNofFeatures)
	{
		throw std::runtime_error( "query to complex for positional weighting");
	}
	m_idf = calc_idf( nofCollectionDocuments, m_itr->documentFrequency());

	std::vector<const PostingIteratorInterface*>::const_iterator pi = sear.begin(), pe = sear.end();
	for (; pi != pe; ++pi)
	{
		float nofMatches = (*pi)->documentFrequency();
		float idf = calc_idf( nofCollectionDocuments, nofMatches);
		m_subexpressions.push_back( SubExpressionData( const_cast<PostingIteratorInterface*>(*pi), idf));
	}
}


float WeightingExecutionContext::call( const Index& docno)
{
	if (m_itr->skipDoc( docno) != docno) return 0.0;
	m_metadata->skipDoc( docno);
	float doclen = m_metadata->getValue( m_metadata_doclen);
	float rel_doclen = (doclen+1) / m_avgDocLength;

	if (m_subexpressions.empty())
	{
		// For single term calculate BM25 and multiply it with the document weight:
		float ff = m_itr->frequency();
		float weight = m_idf
				* (ff * (m_k1 + 1.0))
				/ (ff + m_k1 * (1.0 - m_b + m_b * rel_doclen));
		return weight;
	}
	else
	{
		// Calculate base ff for each subterm:
		float ff[ MaxNofFeatures];
		std::size_t fi = 0, fe = m_subexpressions.size();

		for (; fi != fe; ++fi)
		{
			ff[ fi] = m_subexpressions[fi].postings->frequency();
		}
		float featpos[ MaxNofFeatures];
		Index pos = 0;
		while (0!=(pos=m_itr->skipPos(pos+1)))
		{
			// Calculate ff increment for each expression occurrence:
			float* fp = featpos;
			std::vector<SubExpressionData>::const_iterator
				si = m_subexpressions.begin(), se = m_subexpressions.end();
			for (; si != se; ++si)
			{
				*fp++ = si->postings->posno();
			}
			for (fi=0; fi != fe; ++fi)
			{
				std::size_t ofi = 0;
				for (; ofi != fe; ++ofi)
				{
					if (ofi != fi)
					{
						float dist = (featpos[fi] - featpos[ofi]) - (fi - ofi);
						ff[ fi] += 1.0 / (m_a * dist + 1.0);
					}
				}
			}
		}

		// Calculate BM25 with the ff having the positional weight increment
		// and multiply it with the document weight:
		std::size_t sidx = 0;
		float weight = 0.0;
		std::vector<SubExpressionData>::const_iterator
			si = m_subexpressions.begin(), se = m_subexpressions.end();
		for (si = m_subexpressions.begin(); si != se; ++sidx,++si)
		{
			weight += si->idf
				* (ff[sidx] * (m_k1 + 1.0))
				/ (ff[sidx] + m_k1 * (1.0 - m_b + m_b * rel_doclen));
		}
		return weight;
	}
}


