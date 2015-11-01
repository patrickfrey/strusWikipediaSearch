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
#include "strus/private/dll_tags.hpp"
#include "strus/storageModule.hpp"
#include "weightingBM25_dpfc.hpp"

#define CATCH_ERROR_MAP_RETURN( MSG, HND, VALUE)\
	catch( const std::bad_alloc&)\
	{\
		(HND).report( "out of memory in wikipedia tokenizer");\
		return VALUE;\
	}\
	catch( const std::runtime_error& err)\
	{\
		(HND).report( "error in wikipedia tokenizer: %s", err.what());\
		return VALUE;\
	}\

strus::WeightingFunctionInterface* createWeightingFunction_BM25_dpfc( strus::ErrorBufferInterface* errorhnd)
{
	return new strus::WeightingFunctionBM25_dpfc( errorhnd);
}


struct WeightingFunctionConstructor
{
	typedef strus::WeightingFunctionInterface* (*Create)( strus::ErrorBufferInterface* errorhnd);
	const char* name;				///< name of the weighting function
	Create create;					///< constructor of the function
};

static const strus::WeightingFunctionConstructor weightingFunctions[] =
{
	{"BM25_dpfc"},
	{0,0}
};

static const strus::SummarizerFunctionConstructor summarizers[] =
{
	{0,0}
};

extern "C" DLL_PUBLIC strus::StorageModule entryPoint;

strus::StorageModule entryPoint( 0, weightingFunctions, summarizers);




