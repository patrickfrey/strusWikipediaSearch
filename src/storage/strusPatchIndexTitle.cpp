/*
 * Copyright (c) 2016 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
/// \brief Program calculating pagerank for a list of link definitions read from stdin
#include "strus/lib/module.hpp"
#include "strus/lib/error.hpp"
#include "strus/lib/storage_objbuild.hpp"
#include "strus/storageInterface.hpp"
#include "strus/storageClientInterface.hpp"
#include "strus/attributeReaderInterface.hpp"
#include "strus/documentTermIteratorInterface.hpp"
#include "strus/moduleLoaderInterface.hpp"
#include "strus/forwardIteratorInterface.hpp"
#include "strus/errorBufferInterface.hpp"
#include <vector>
#include <string>
#include <map>
#include <iostream>
#include <stdexcept>
#include <cstring>
#include <cstdio>
#include <memory>

#undef STRUS_LOWLEVEL_DEBUG

#define DOC_ATTRIBUTE_TITLEID      "titid"
#define DOC_SEARCH_TYPE_TITLE      "vectfeat"
#define DOC_FORWARD_TYPE_TITLEREF  "veclfeat"
#define DOC_FORWARD_TYPE_LINKID    "linkid"

static strus::ErrorBufferInterface* g_errorhnd = 0;

static void printUsage()
{
	std::cerr << "usage: strusPatchIndexTitle [options]" << std::endl;
	std::cerr << "    options     :" << std::endl;
	std::cerr << "    -h          : print this usage" << std::endl;
	std::cerr << "    -s <CFG>    : process storage with configuration <CFG>" << std::endl;
	std::cerr << "    -p          : print patches only without applying them" << std::endl;
}

typedef std::map<std::string,unsigned int> TitleDocidxMap;

struct TitleReference
{
	std::string featname;
	strus::Index pos;

	TitleReference( const std::string& featname_, const strus::Index& pos_)
		:featname(featname_),pos(pos_){}
	TitleReference( const TitleReference& o)
		:featname(o.featname),pos(o.pos){}
};

struct DocumentDef
{
	std::string titlefeat;
	std::vector<TitleReference> reflist;

	DocumentDef( const std::string& titlefeat_, const std::vector<TitleReference>& reflist_)
		:titlefeat(titlefeat_),reflist(reflist_){}
	DocumentDef( const DocumentDef& o)
		:titlefeat(o.titlefeat),reflist(o.reflist){}
};

struct PatchIndexData
{
	TitleDocidxMap titleDocidxMap;
	std::vector<DocumentDef> documentar;
};

static void buildData( PatchIndexData& data, strus::StorageClientInterface* storage)
{
	std::auto_ptr<strus::AttributeReaderInterface> attreader( storage->createAttributeReader());
	if (!attreader.get()) throw std::runtime_error( "failed to create attribute reader");
	strus::Index titleattr = attreader->elementHandle( DOC_ATTRIBUTE_TITLEID);
	if (titleattr == 0) throw std::runtime_error( "title attribute not defined in storage");
	std::auto_ptr<strus::DocumentTermIteratorInterface> search_title_itr( storage->createDocumentTermIterator( DOC_SEARCH_TYPE_TITLE));
	if (!search_title_itr.get()) throw std::runtime_error( "failed to create seach index title iterator");
	std::auto_ptr<strus::ForwardIteratorInterface> forward_titleref_itr( storage->createForwardIterator( DOC_FORWARD_TYPE_TITLEREF));
	if (!forward_titleref_itr.get()) throw std::runtime_error( "failed to create forward index title ref iterator");
	std::auto_ptr<strus::ForwardIteratorInterface> forward_linkid_itr( storage->createForwardIterator( DOC_FORWARD_TYPE_LINKID));
	if (!forward_linkid_itr.get()) throw std::runtime_error( "failed to create forward index linkid iterator");

	strus::Index docno = 1, maxdocno = storage->maxDocumentNumber();
	for (; docno <= maxdocno; ++docno)
	{
		std::string titleid = attreader->getValue( titleattr);
		std::string titlefeat;
		std::vector<TitleReference> reflist;

		if (docno == search_title_itr->skipDoc( docno))
		{
			strus::DocumentTermIteratorInterface::Term term;
			while (search_title_itr->nextTerm( term))
			{
				if (term.firstpos == 1)
				{
					titlefeat = search_title_itr->termValue( term.termno);
				}
			}
		}
		forward_titleref_itr->skipDoc( docno);
		forward_linkid_itr->skipDoc( docno);
		strus::Index titleref_pos = forward_titleref_itr->skipPos(0);
		strus::Index linkid_pos = forward_linkid_itr->skipPos(0);

		while (titleref_pos && linkid_pos)
		{
			if (titleref_pos < linkid_pos)
			{
				titleref_pos = forward_titleref_itr->skipPos( titleref_pos+1);
			}
			else if (linkid_pos < titleref_pos)
			{
				linkid_pos  = forward_linkid_itr->skipPos( linkid_pos+1);
			}
			else
			{
				reflist.push_back( TitleReference( forward_titleref_itr->fetch(), titleref_pos));
				titleref_pos = forward_titleref_itr->skipPos( titleref_pos+1);
				linkid_pos  = forward_linkid_itr->skipPos( linkid_pos+1);
			}
		}
		data.titleDocidxMap[ titleid] = data.documentar.size();
		data.documentar.push_back( DocumentDef( titlefeat, reflist));
	}
}

static void printData( std::ostream& out, const PatchIndexData& data)
{
	TitleDocidxMap::const_iterator ti = data.titleDocidxMap.begin(), te = data.titleDocidxMap.end();
	for (; ti != te; ++ti)
	{
		const DocumentDef& def = data.documentar[ ti->second];
		out << ti->first;
		out << "\t" << DOC_SEARCH_TYPE_TITLE << " " << def.titlefeat << " 1" << std::endl;
		std::vector<TitleReference>::const_iterator ri = def.reflist.begin(), re = def.reflist.end();
		for (; ri != re; ++ri)
		{
			out << "\t" << DOC_FORWARD_TYPE_TITLEREF << " " << ri->featname << " " << ri->pos << std::endl;
		}
	}
}

int main( int argc, const char** argv)
{
	try
	{
		std::auto_ptr<strus::ErrorBufferInterface> errorBuffer( strus::createErrorBuffer_standard( 0, 2));
		if (!errorBuffer.get())
		{
			std::cerr << "failed to create error buffer" << std::endl;
			return -1;
		}
		g_errorhnd = errorBuffer.get();

		if (argc <= 1)
		{
			std::cerr << "too few arguments" << std::endl;
			printUsage();
			return 0;
		}
		int argi = 1;
		std::vector<std::string> storageconfigs;
		bool doPrintOnly = false;

		for (; argi < argc; ++argi)
		{
			if (std::strcmp( argv[ argi], "-h") == 0 || std::strcmp( argv[ argi], "--help") == 0)
			{
				printUsage();
				return 0;
			}
			if (std::strcmp( argv[ argi], "-p") == 0 || std::strcmp( argv[ argi], "--print") == 0)
			{
				doPrintOnly = true;
			}
			if (std::strcmp( argv[ argi], "-s") == 0 || std::strcmp( argv[ argi], "--storage") == 0)
			{
				++argi;
				if (argi == argc) throw std::runtime_error("argument (storage configuration string) expected for option -s");
				storageconfigs.push_back( argv[ argi]);
			}
			else if (std::strcmp( argv[ argi], "-") == 0)
			{
				break;
			}
			else if (std::strcmp( argv[ argi], "--") == 0)
			{
				++argi;
				break;
			}
			else if (argv[ argi][0] == '-')
			{
				std::cerr << "unknown option: " << argv[argi] << std::endl;
				printUsage();
				return -1;
			}
			else
			{
				break;
			}
		}
		if (argi < argc)
		{
			std::cerr << "too many arguments (no arguments expected)" << std::endl;
			printUsage();
			return -1;
		}

		std::auto_ptr<strus::ModuleLoaderInterface> moduleLoader( strus::createModuleLoader( errorBuffer.get()));
		if (!moduleLoader.get()) throw std::runtime_error( "failed to create module loader");
		
		std::auto_ptr<strus::StorageObjectBuilderInterface> storageBuilder;
		storageBuilder.reset( moduleLoader->createStorageObjectBuilder());
		if (!storageBuilder.get()) throw std::runtime_error( "failed to create storage object builder");

		PatchIndexData procdata;
		std::vector<std::string>::const_iterator ci = storageconfigs.begin(), ce = storageconfigs.end();
		for (; ci != ce; ++ci)
		{
			std::auto_ptr<strus::StorageClientInterface>
				storage( strus::createStorageClient( storageBuilder.get(), errorBuffer.get(), *ci));
			if (!storage.get()) throw std::runtime_error( "failed to create storage client");

			buildData( procdata, storage.get());
		}
		if (doPrintOnly)
		{
			printData( std::cout, procdata);
		}
		std::cerr << "done" << std::endl;
		return 0;
	}
	catch (const std::runtime_error& err)
	{
		std::cerr << "error: " << err.what() << std::endl;
		return -1;
	}
	catch (const std::bad_alloc& )
	{
		std::cerr << "out of memory" << std::endl;
		return -1;
	}
	catch (const std::logic_error& err)
	{
		std::cerr << "error: " << err.what() << std::endl;
		return -1;
	}
}


