/*
 * Copyright (c) 2014 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#include "strus/base/dll_tags.hpp"
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

static const strus::WeightingFunctionConstructor weightingFunctions[] =
{
	{"BM25_dpfc", createWeightingFunction_BM25_dpfc},
	{0,0}
};

static const strus::SummarizerFunctionConstructor summarizers[] =
{
	{0,0}
};

extern "C" DLL_PUBLIC strus::StorageModule entryPoint;

strus::StorageModule entryPoint( 0, weightingFunctions, summarizers);




