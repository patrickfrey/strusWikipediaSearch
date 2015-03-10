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
#include "strus/analyzerModule.hpp"
#include "tokenizers.hpp"

static const strus::TokenizerConstructor tokenizers[] =
{
	{"mw_url_id",&strus::tokenizerWikimediaUrlId},
	{"mw_url_content",&strus::tokenizerWikimediaUrlContent},
	{"mw_url_word",&strus::tokenizerWikimediaUrlWord},
	{"mw_url_split",&strus::tokenizerWikimediaUrlSplit},
	{"mw_link_id",&strus::tokenizerWikimediaLinkId},
	{"mw_link_content",&strus::tokenizerWikimediaLinkContent},
	{"mw_link_word",&strus::tokenizerWikimediaLinkWord},
	{"mw_link_split",&strus::tokenizerWikimediaLinkSplit},
	{"mw_citation_content",&strus::tokenizerWikimediaCitationContent},
	{"mw_citation_word",&strus::tokenizerWikimediaCitationWord},
	{"mw_citation_split",&strus::tokenizerWikimediaCitationSplit},
	{"mw_reference_content",&strus::tokenizerWikimediaReferenceContent},
	{"mw_reference_word",&strus::tokenizerWikimediaReferenceWord},
	{"mw_reference_split",&strus::tokenizerWikimediaReferenceSplit},
	{"mw_text_word",&strus::tokenizerWikimediaTextWord},
	{"mw_text_split",&strus::tokenizerWikimediaTextSplit},
	{0,0}
};

static const strus::NormalizerConstructor normalizers[] =
{
	{0,0}
};

extern "C" DLL_PUBLIC strus::AnalyzerModule entryPoint;

strus::AnalyzerModule entryPoint( tokenizers, normalizers);




