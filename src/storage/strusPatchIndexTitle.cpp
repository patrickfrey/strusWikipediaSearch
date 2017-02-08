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
#include "strus/storageDocumentUpdateInterface.hpp"
#include "strus/storageTransactionInterface.hpp"
#include "strus/attributeReaderInterface.hpp"
#include "strus/documentTermIteratorInterface.hpp"
#include "strus/moduleLoaderInterface.hpp"
#include "strus/forwardIteratorInterface.hpp"
#include "strus/errorBufferInterface.hpp"
#include <vector>
#include <string>
#include <map>
#include <set>
#include <iostream>
#include <stdexcept>
#include <cstring>
#include <cstdio>
#include <memory>
#include <cstdlib>

#undef STRUS_LOWLEVEL_DEBUG

#define DOC_ATTRIBUTE_DOCID        "docid"
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
	std::cerr << "    -c <SIZE>   : size of updates per transaction (default 50000)" << std::endl;
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
	std::string titleid;
	strus::Index docno;
	std::string titlefeat;
	std::vector<TitleReference> reflist;

	DocumentDef( const std::string& titleid_, const strus::Index& docno_, const std::string& titlefeat_, const std::vector<TitleReference>& reflist_)
		:titleid(titleid_),docno(docno_),titlefeat(titlefeat_),reflist(reflist_){}
	DocumentDef( const DocumentDef& o)
		:titleid(o.titleid),docno(o.docno),titlefeat(o.titlefeat),reflist(o.reflist){}
};

struct PatchIndexData
{
	std::string storageConfig;
	std::vector<DocumentDef> documentar;
	std::map<std::string,strus::Index> dfmap;

	PatchIndexData( const std::string& storageConfig_)
		:storageConfig(storageConfig_),documentar(),dfmap(){}
	PatchIndexData( const std::string& storageConfig_, const TitleDocidxMap& titleDocidxMap_, const std::vector<DocumentDef>& documentar_, const std::map<std::string,strus::Index>& dfmap_)
		:storageConfig(storageConfig_),documentar(documentar_),dfmap(dfmap_){}
	PatchIndexData( const PatchIndexData& o)
		:storageConfig(o.storageConfig),documentar(o.documentar),dfmap(o.dfmap){}
};

static void buildData( PatchIndexData& data, strus::StorageClientInterface* storage)
{
	std::auto_ptr<strus::AttributeReaderInterface> attreader( storage->createAttributeReader());
	if (!attreader.get()) throw std::runtime_error( "failed to create attribute reader");
	strus::Index titleattr = attreader->elementHandle( DOC_ATTRIBUTE_TITLEID);
	if (titleattr == 0) throw std::runtime_error( "title attribute not defined in storage");
	strus::Index docidattr = attreader->elementHandle( DOC_ATTRIBUTE_DOCID);
	if (docidattr == 0) throw std::runtime_error( "docid attribute not defined in storage");
	std::auto_ptr<strus::DocumentTermIteratorInterface> search_title_itr( storage->createDocumentTermIterator( DOC_SEARCH_TYPE_TITLE));
	if (!search_title_itr.get()) throw std::runtime_error( "failed to create seach index title iterator");
	std::auto_ptr<strus::ForwardIteratorInterface> forward_titleref_itr( storage->createForwardIterator( DOC_FORWARD_TYPE_TITLEREF));
	if (!forward_titleref_itr.get()) throw std::runtime_error( "failed to create forward index title ref iterator");
	std::auto_ptr<strus::ForwardIteratorInterface> forward_linkid_itr( storage->createForwardIterator( DOC_FORWARD_TYPE_LINKID));
	if (!forward_linkid_itr.get()) throw std::runtime_error( "failed to create forward index linkid iterator");

	strus::Index docno = 1, maxdocno = storage->maxDocumentNumber();
	for (; docno <= maxdocno; ++docno)
	{
		attreader->skipDoc( docno);

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
		if (titlefeat.empty())
		{
			std::string docid( attreader->getValue( docidattr));
			std::cerr << "no title found for: '" << docid << "'" << std::endl;
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
				std::cerr << "link id without associated title: '" << forward_linkid_itr->fetch() << "'" << std::endl;
				linkid_pos  = forward_linkid_itr->skipPos( linkid_pos+1);
			}
			else
			{
				// ... we select only the title references that align with a linkid reference in the forward index
				std::set<std::string> occurrencies;
				std::string featname( forward_titleref_itr->fetch());
				occurrencies.insert( featname);
				reflist.push_back( TitleReference( featname, titleref_pos));
				titleref_pos = forward_titleref_itr->skipPos( titleref_pos+1);
				linkid_pos  = forward_linkid_itr->skipPos( linkid_pos+1);

				std::set<std::string>::const_iterator oi = occurrencies.begin(), oe = occurrencies.end();
				for (; oi != oe; ++oi)
				{
					data.dfmap[ *oi] += 1;
				}
			}
		}
		data.documentar.push_back( DocumentDef( titleid, docno, titlefeat, reflist));
	}
}

static void rewriteIndex( strus::StorageClientInterface* storage, const PatchIndexData& data, unsigned int transactionSize)
{
	std::cerr << "update title references of storage " << data.storageConfig << std::endl;
	unsigned int doccnt = 0;
	std::vector<DocumentDef>::const_iterator ti = data.documentar.begin(), te = data.documentar.end();
	while (ti != te)
	{
		std::auto_ptr<strus::StorageTransactionInterface> transaction( storage->createTransaction());
		unsigned int ci = 0, ce = transactionSize;
		for (; ti != te && ci < ce; ++ti,++ci)
		{
			const DocumentDef& def = *ti;
			std::auto_ptr<strus::StorageDocumentUpdateInterface> document(
					transaction->createDocumentUpdate( def.docno));
			document->addSearchIndexTerm( DOC_SEARCH_TYPE_TITLE, def.titlefeat, 1);
			std::vector<TitleReference>::const_iterator ri = def.reflist.begin(), re = def.reflist.end();
			for (; ri != re; ++ri)
			{
				document->addForwardIndexTerm( DOC_FORWARD_TYPE_TITLEREF, ri->featname, ri->pos);
			}
			document->done();
			++doccnt;
		}
		if (!transaction->commit())
		{
			throw std::runtime_error( "transaction failed");
		}
		fprintf( stderr, "\rupdated %u documents          ", doccnt);
	}
	std::cerr << std::endl;
	doccnt = 0;
	std::cerr << "update title reference df's of storage " << data.storageConfig << std::endl;
	std::map<std::string,strus::Index>::const_iterator di = data.dfmap.begin(), de = data.dfmap.end();
	while (di != de)
	{
		std::auto_ptr<strus::StorageTransactionInterface> transaction( storage->createTransaction());
		unsigned int ci = 0, ce = transactionSize;
		for (; di != de && ci < ce; ++di,++ci)
		{
			strus::Index old_df = storage->documentFrequency( DOC_FORWARD_TYPE_TITLEREF, di->first);
			transaction->updateDocumentFrequency( DOC_FORWARD_TYPE_TITLEREF, di->first, di->second - old_df);
			++doccnt;
		}
		transaction->commit();
		fprintf( stderr, "\rupdated %u df's             ", doccnt);
	}
	std::cerr << std::endl;
}

static void printData( std::ostream& out, const PatchIndexData& data)
{
	std::vector<DocumentDef>::const_iterator ti = data.documentar.begin(), te = data.documentar.end();
	for (; ti != te; ++ti)
	{
		const DocumentDef& def = *ti;
		out << def.titleid << std::endl;
		out << "\t" << DOC_SEARCH_TYPE_TITLE << " " << def.titlefeat << " 1" << std::endl;
		std::vector<TitleReference>::const_iterator ri = def.reflist.begin(), re = def.reflist.end();
		for (; ri != re; ++ri)
		{
			out << "\t" << DOC_FORWARD_TYPE_TITLEREF << " " << ri->featname << " " << ri->pos << std::endl;
		}
	}
	std::map<std::string,strus::Index>::const_iterator di = data.dfmap.begin(), de = data.dfmap.end();
	for (; di != de; ++di)
	{
		out << "# " << DOC_FORWARD_TYPE_TITLEREF << " " << di->first << " " << di->second << std::endl;
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
		unsigned int transactionSize = 50000;
		std::vector<std::string> storageconfigs;
		bool doPrintOnly = false;

		for (; argi < argc; ++argi)
		{
			if (std::strcmp( argv[ argi], "-h") == 0 || std::strcmp( argv[ argi], "--help") == 0)
			{
				printUsage();
				return 0;
			}
			else if (std::strcmp( argv[ argi], "-p") == 0 || std::strcmp( argv[ argi], "--print") == 0)
			{
				doPrintOnly = true;
			}
			else if (std::strcmp( argv[ argi], "-c") == 0 || std::strcmp( argv[ argi], "--commit") == 0)
			{
				if (argi == argc || argv[ argi][0] == '-') throw std::runtime_error("argument (commit size) expected for option -c");
				transactionSize = atoi( argv[ argi]);
				if (!transactionSize) throw std::runtime_error("positive number expected as argument for option -c");
			}
			else if (std::strcmp( argv[ argi], "-s") == 0 || std::strcmp( argv[ argi], "--storage") == 0)
			{
				++argi;
				if (argi == argc || argv[ argi][0] == '-') throw std::runtime_error("argument (storage configuration string) expected for option -s");
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

		std::vector<std::string>::const_iterator ci = storageconfigs.begin(), ce = storageconfigs.end();
		for (; ci != ce; ++ci)
		{
			std::auto_ptr<strus::StorageClientInterface>
				storage( strus::createStorageClient( storageBuilder.get(), errorBuffer.get(), *ci));
			if (!storage.get()) throw std::runtime_error( "failed to create storage client");

			PatchIndexData procdata( *ci);
			buildData( procdata, storage.get());
			if (doPrintOnly)
			{
				printData( std::cout, procdata);
			}
			else
			{
				rewriteIndex( storage.get(), procdata, transactionSize);
			}
		}
		std::cerr << "done" << std::endl;
		return 0;
	}
	catch (const std::runtime_error& err)
	{
		if (g_errorhnd->hasError())
		{
			std::cerr << "error: " << err.what() << ": " << g_errorhnd->fetchError() << std::endl;
		}
		else
		{
			std::cerr << "error: " << err.what() << std::endl;
		}
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


