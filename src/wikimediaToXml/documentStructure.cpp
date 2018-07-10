/*
 * Copyright (c) 2018 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

/// \brief Intermediate document format for converting "Wikimedia XML" to pure and simplified XML
/// \file documentStructure.cpp
#include "documentStructure.hpp"
#include "outputString.hpp"
#include "textwolf/istreamiterator.hpp"
#include "textwolf/xmlscanner.hpp"
#include "textwolf/xmlprinter.hpp"
#include "textwolf/charset_utf8.hpp"
#include "strus/base/string_format.hpp"
#include "strus/base/string_conv.hpp"
#include <stdexcept>
#include <iostream>
#include <sstream>
#include <set>

typedef textwolf::XMLPrinter<textwolf::charset::UTF8,textwolf::charset::UTF8,std::string> XmlPrinterBase;

/// \brief strus toplevel namespace
using namespace strus;

class XmlPrinter
	:public XmlPrinterBase
{
public:
	XmlPrinter()
		:XmlPrinterBase(){}

	bool isInTagDeclaration()
	{
		return (state() == XmlPrinterBase::TagElement);
	}

	void printHeader( std::string& buf)
	{
		if (!XmlPrinterBase::printHeader( "UTF-8", "yes", buf))
		{
			const char* errstr = XmlPrinterBase::lasterror();
			throw std::runtime_error( std::string( "xml print error: ") + (errstr?errstr:"") + " when printing XML header");
		}
	}

	void printOpenTag( const std::string& tag, std::string& buf)
	{
		if (!XmlPrinterBase::printOpenTag( tag.c_str(), tag.size(), buf))
		{
			const char* errstr = XmlPrinterBase::lasterror();
			throw std::runtime_error( std::string( "xml print error: ") + (errstr?errstr:"") + " when printing open tag: " + outputString(tag.c_str(),tag.c_str()+tag.size()));
		}
	}

	void printOpenTag( const char* name, std::string& buf)
	{
		if (!XmlPrinterBase::printOpenTag( name, std::strlen(name), buf))
		{
			const char* errstr = XmlPrinterBase::lasterror();
			throw std::runtime_error( std::string( "xml print error: ") + (errstr?errstr:"") + " when printing open tag: " + outputString(name,name+std::strlen(name)));
		}
	}

	void printAttribute( const std::string& name, std::string& buf)
	{
		if (!XmlPrinterBase::printAttribute( name.c_str(), name.size(), buf))
		{
			const char* errstr = XmlPrinterBase::lasterror();
			throw std::runtime_error( std::string( "xml print error: ") + (errstr?errstr:"") + " when printing attribute: " + outputString(name.c_str(),name.c_str()+name.size()));
		}
	}

	void printAttribute( const char* name, std::string& buf)
	{
		if (!XmlPrinterBase::printAttribute( name, std::strlen(name), buf))
		{
			const char* errstr = XmlPrinterBase::lasterror();
			throw std::runtime_error( std::string( "xml print error: ") + (errstr?errstr:"") + " when printing attribute: " + outputString(name,name+std::strlen(name)));
		}
	}

	void printValue( const std::string& val, std::string& buf)
	{
		if (!XmlPrinterBase::printValue( val.c_str(), val.size(), buf))
		{
			const char* err = XmlPrinterBase::lasterror();
			throw std::runtime_error( std::string( "xml print error: ") + (err?err:"") + " when printing: " + outputString(val.c_str(),val.c_str()+val.size()));
		}
	}

	void printValue( const char* si, const char* se, std::string& buf)
	{
		if (!XmlPrinterBase::printValue( si, se-si, buf))
		{
			const char* err = XmlPrinterBase::lasterror();
			throw std::runtime_error( std::string( "xml print error: ") + (err?err:"") + " when printing: " + outputString(si,se));
		}
	}

	void switchToContent( std::string& buf)
	{
		if (!XmlPrinterBase::printValue( "", 0, buf))
		{
			const char* err = XmlPrinterBase::lasterror();
			throw std::runtime_error( std::string( "xml print error: ") + (err?err:"") + " when switching to content");
		}
	}

	void printCloseTag( std::string& buf)
	{
		if (!XmlPrinterBase::printCloseTag( buf))
		{
			const char* err = XmlPrinterBase::lasterror();
			throw std::runtime_error( std::string( "xml print error: ") + (err?err:"") + " when printing close tag");
		}
	}
};

bool DocumentStructure::checkTableDefExists( const char* action)
{
	if (m_tableDefs.empty())
	{
		addError( strus::string_format( "%s without table defined", action));
		return false;
	}
	return true;
}

void DocumentStructure::checkStartEndSectionBalance( const std::vector<Paragraph>::const_iterator& start, const std::vector<Paragraph>::const_iterator& end)
{
	std::vector<Paragraph::Type> stk;
	std::vector<Paragraph>::const_iterator ii = start;
	for (; ii != end; ++ii)
	{
		switch (ii->type())
		{
			case Paragraph::EntityStart:
			case Paragraph::QuotationStart:
			case Paragraph::DoubleQuoteStart:
			case Paragraph::BlockQuoteStart:
			case Paragraph::DivStart:
			case Paragraph::PoemStart:
			case Paragraph::SpanStart:
			case Paragraph::FormatStart:
			case Paragraph::HeadingStart:
			case Paragraph::ListItemStart:
			case Paragraph::AttributeStart:
			case Paragraph::CitationStart:
			case Paragraph::RefStart:
			case Paragraph::PageLinkStart:
			case Paragraph::WebLinkStart:
			case Paragraph::TableStart:
			case Paragraph::TableTitleStart:
			case Paragraph::TableHeadStart:
			case Paragraph::TableCellStart:
				stk.push_back( ii->type());
				break;
			case Paragraph::EntityEnd:
			case Paragraph::QuotationEnd:
			case Paragraph::DoubleQuoteEnd:
			case Paragraph::BlockQuoteEnd:
			case Paragraph::DivEnd:
			case Paragraph::PoemEnd:
			case Paragraph::SpanEnd:
			case Paragraph::FormatEnd:
			case Paragraph::HeadingEnd:
			case Paragraph::ListItemEnd:
			case Paragraph::AttributeEnd:
			case Paragraph::CitationEnd:
			case Paragraph::RefEnd:
			case Paragraph::PageLinkEnd:
			case Paragraph::WebLinkEnd:
			case Paragraph::TableEnd:
			case Paragraph::TableTitleEnd:
			case Paragraph::TableHeadEnd:
			case Paragraph::TableCellEnd:
				if (stk.empty()) throw std::runtime_error( strus::string_format( "structure open/close not balanced: ...%s", ii->typeName()));
				if (stk.back() != Paragraph::invType( ii->type())) throw std::runtime_error( strus::string_format( "structure open/close not balanced: %s...%s", Paragraph::typeName( stk.back()), ii->typeName()));
				stk.pop_back();
				break;
			case Paragraph::Title:
			case Paragraph::Text:
			case Paragraph::Char:
			case Paragraph::BibRef:
			case Paragraph::NoWiki:
			case Paragraph::Code:
			case Paragraph::Math:
			case Paragraph::Timestamp:
			case Paragraph::CitationLink:
			case Paragraph::RefLink:
			case Paragraph::TableLink:
				break;
		}
	}
	if (!stk.empty()) throw std::runtime_error( strus::string_format( "structure open/close not balanced: %s...%s", Paragraph::typeName( stk.back()), ""));
}

void DocumentStructure::closeOpenQuoteItems()
{
	while (!m_structStack.empty())
	{
		int startidx = m_structStack.back().start;
		Paragraph para = m_parar[ startidx];
		if (para.type() == Paragraph::EntityStart || para.type() == Paragraph::QuotationStart || para.type() == Paragraph::DoubleQuoteStart)
		{
			finishStructure( startidx);
		}
		else
		{
			return;
		}
	}
}

void DocumentStructure::closeDanglingStructures( const Paragraph::Type& starttype)
{
	while (!m_structStack.empty())
	{
		int startidx = m_structStack.back().start;
		Paragraph para = m_parar[ startidx];
		if (para.type() == starttype)
		{
			return;
		}
		else if (para.type() == Paragraph::EntityStart)
		{
			m_parar[ m_structStack.back().start] = Paragraph( Paragraph::Text, "", "\"");
			m_structStack.pop_back();
		}
		else if (para.type() == Paragraph::QuotationStart)
		{
			finishStructure( startidx);
		}
		else if (para.type() == Paragraph::DoubleQuoteStart)
		{
			finishStructure( startidx);
		}
		else if (para.type() == Paragraph::BlockQuoteStart)
		{
			if (starttype == Paragraph::WebLinkStart) return;
			finishStructure( startidx);
		}
		else if (para.type() == Paragraph::DivStart)
		{
			if (starttype == Paragraph::WebLinkStart) return;
			finishStructure( startidx);
		}
		else if (para.type() == Paragraph::PoemStart)
		{
			if (starttype == Paragraph::WebLinkStart) return;
			finishStructure( startidx);
		}
		else if (para.type() == Paragraph::SpanStart)
		{
			if (starttype == Paragraph::WebLinkStart) return;
			finishStructure( startidx);
		}
		else if (para.type() == Paragraph::FormatStart)
		{
			if (starttype == Paragraph::WebLinkStart) return;
			finishStructure( startidx);
		}
		else if (para.type() == Paragraph::HeadingStart)
		{
			finishStructure( startidx);
		}
		else if (para.type() == Paragraph::ListItemStart)
		{
			if (starttype == Paragraph::WebLinkStart) return;
			finishStructure( startidx);
		}
		else if (para.type() == Paragraph::AttributeStart)
		{
			finishStructure( startidx);
		}
		else if (para.type() == Paragraph::CitationStart)
		{
			if (starttype == Paragraph::WebLinkStart) return;
			finishStructure( startidx);
		}
		else if (para.type() == Paragraph::RefStart)
		{
			if (starttype == Paragraph::WebLinkStart) return;
			finishStructure( startidx);
		}
		else if (para.type() == Paragraph::PageLinkStart)
		{
			if (starttype == Paragraph::WebLinkStart) return;
			finishStructure( startidx);
		}
		else if (para.type() == Paragraph::WebLinkStart)
		{
			finishStructure( startidx);
		}
		else if (para.type() == Paragraph::TableStart)
		{
			if (starttype == Paragraph::AttributeStart) return;
			if (starttype == Paragraph::BlockQuoteStart) return;
			if (starttype == Paragraph::DivStart) return;
			if (starttype == Paragraph::SpanStart) return;
			if (starttype == Paragraph::FormatStart) return;
			if (starttype == Paragraph::TableTitleStart) return;
			if (starttype == Paragraph::TableHeadStart) return;
			if (starttype == Paragraph::TableCellStart) return;
			if (starttype == Paragraph::RefStart) return;
			if (starttype == Paragraph::CitationStart) return;
			if (starttype == Paragraph::PageLinkStart) return;
			if (starttype == Paragraph::WebLinkStart) return;
			finishStructure( startidx);
		}
		else if (para.type() == Paragraph::TableTitleStart)
		{
			if (starttype == Paragraph::WebLinkStart) return;
			if (starttype == Paragraph::AttributeStart) return;
			finishStructure( startidx);
		}
		else if (para.type() == Paragraph::TableHeadStart)
		{
			if (starttype == Paragraph::WebLinkStart) return;
			finishStructure( startidx);
		}
		else if (para.type() == Paragraph::TableCellStart)
		{
			if (starttype == Paragraph::WebLinkStart) return;
			finishStructure( startidx);
		}
		else
		{
			throw std::runtime_error( strus::string_format( "internal: unknown structure %s", para.typeName()));
		}
	}
}

void DocumentStructure::addQuoteItem( Paragraph::Type startType)
{
	Paragraph::Type endType = Paragraph::invType( startType);
	if (!m_structStack.empty() && m_parar[ m_structStack.back().start].type() == startType)
	{
		m_parar.push_back( Paragraph( endType, "", ""));
		m_structStack.pop_back();
	}
	else
	{
		m_structStack.push_back( StructRef( 0, m_parar.size()));
		m_parar.push_back( Paragraph( startType, "", ""));
	}
}

void DocumentStructure::openAutoCloseItem( Paragraph::Type startType, const char* prefix, int lidx)
{
	Paragraph::Type endType = Paragraph::invType( startType);
	while (!m_structStack.empty() && m_parar[ m_structStack.back().start].type() == startType && m_structStack.back().idx <= lidx)
	{
		Paragraph para = m_parar[ m_structStack.back().start];
		m_parar.push_back( Paragraph( endType, "", ""));
		m_structStack.pop_back();
	}
	m_structStack.push_back( StructRef( lidx, m_parar.size()));
	if (lidx > 0)
	{
		m_parar.push_back( Paragraph( startType, strus::string_format("%s%d", prefix, lidx), ""));
	}
	else
	{
		m_parar.push_back( Paragraph( startType, prefix, ""));
	}
}

void DocumentStructure::closeAutoCloseItem( Paragraph::Type startType)
{
	Paragraph::Type endType = Paragraph::invType( startType);
	if (m_structStack.empty()) return;
	Paragraph para = m_parar[ m_structStack.back().start];
	if (para.type() == startType)
	{
		m_parar.push_back( Paragraph( endType, "", ""));
		m_structStack.pop_back();
	}
}

void DocumentStructure::openTableCell( Paragraph::Type startType, int rowspan, int colspan)
{
	if (!checkTableDefExists( "table open cell")) return;
	m_structStack.push_back( StructRef( 0, m_parar.size()));
	m_tableDefs.back().defineCell( m_parar.size(), rowspan, colspan);
	m_tableDefs.back().nextCol( colspan);
	m_parar.push_back( Paragraph( startType, "", ""));
}

void DocumentStructure::openStructure( Paragraph::Type startType, const char* prefix, int lidx)
{
	if (startType == Paragraph::TableStart)
	{
		m_tableDefs.push_back( TableDef( m_parar.size()));
	}
	m_structStack.push_back( StructRef( 0, m_parar.size()));
	if (lidx > 0)
	{
		m_parar.push_back( Paragraph( startType, strus::string_format("%s%d", prefix, lidx), ""));
	}
	else
	{
		m_parar.push_back( Paragraph( startType, prefix, ""));
	}
}

typedef std::pair<std::vector<Paragraph>::const_iterator,std::vector<Paragraph>::const_iterator> ParagraphRange;
ParagraphRange findParagraphRange( const std::vector<Paragraph>::const_iterator& startitr, const std::vector<Paragraph>::const_iterator& enditr, Paragraph::Type starttype, Paragraph::Type endtype)
{
	ParagraphRange rt( enditr, enditr);
	std::vector<Paragraph>::const_iterator pi = startitr, pe = enditr;
	for (; pi != pe; ++pi)
	{
		if (pi->type() == starttype)
		{
			rt.first = pi;
			++pi;
			for (; pi != pe; ++pi)
			{
				if (pi->type() == endtype)
				{
					++pi;
					rt.second = pi;
					return rt;
				}
			}
			break;
		}
	}
	return ParagraphRange( enditr, enditr);
}

void DocumentStructure::addTableCellIdentifierAttributes( const char* prefix, const std::set<int>& indices)
{
	std::set<int>::const_iterator ai = indices.begin(), ae = indices.end();
	for (; ai != ae; ++ai)
	{
		m_tables.push_back( Paragraph( Paragraph::AttributeStart, "id", strus::string_format( "%s%d", prefix, *ai)));
		m_tables.push_back( Paragraph( Paragraph::AttributeEnd, "", ""));
	}
}

std::string DocumentStructure::passageKey( const std::vector<Paragraph>::const_iterator& begin, const std::vector<Paragraph>::const_iterator& end)
{
	std::string rt;
	std::vector<Paragraph>::const_iterator itr = begin;
	for (; itr != end; ++itr)
	{
		rt.append( itr->tokey());
		rt.push_back( '\2');
	}
	return rt;
}

void DocumentStructure::finishTable( int startidx, const std::string& tableid)
{
	if (!checkTableDefExists( "finish table")) return;

	m_tables.push_back( m_parar[ startidx]);
	const TableDef& tableDef = m_tableDefs.back();

	// Print table title elements first:
	ParagraphRange titlerange = findParagraphRange( m_parar.begin() + startidx, m_parar.end(), Paragraph::TableTitleStart, Paragraph::TableTitleEnd);
	m_tables.insert( m_tables.end(), titlerange.first, titlerange.second);

	std::map<int,std::set<int> > start2rowmap;
	std::map<int,std::set<int> > start2colmap;
	std::set<int> dataRowSet;
	CellPositionStartMap::const_iterator ci,ce;

	// Define set of cells with data elements, they are assumed to address rows:
	ci = tableDef.cellmap.begin(), ce = tableDef.cellmap.end();
	for (; ci != ce; ++ci)
	{
		const Paragraph& para = m_parar[ ci->second];
		if (para.type() == Paragraph::TableCellStart)
		{
			dataRowSet.insert( ci->first.row);
		}
	}
	// Create maps with cell identifiers:
	ci = tableDef.cellmap.begin(), ce = tableDef.cellmap.end();
	for (; ci != ce; ++ci)
	{
		const Paragraph& para = m_parar[ ci->second];
		if (para.type() == Paragraph::TableHeadStart)
		{
			if (dataRowSet.find( ci->first.row) == dataRowSet.end())
			{
				// ... row containing no data elements assumed to be a column heading element
				start2colmap[ ci->second].insert( ci->first.col);
			}
			else
			{
				// ... row containing at least one data element assumed to be a row heading element
				start2rowmap[ ci->second].insert( ci->first.row);
			}
		}
		else if (para.type() == Paragraph::TableCellStart)
		{
			start2rowmap[ ci->second].insert( ci->first.row);
			start2colmap[ ci->second].insert( ci->first.col);
		}
		else
		{
			throw std::runtime_error("internal: corrupt table data structures");
		}
	}
	// Print table cells:
	Paragraph::Type types[ 2] = {Paragraph::TableHeadStart, Paragraph::TableCellStart};
	for (int ti = 0; ti < (int)((sizeof(types)/sizeof(types[0]))); ++ti)
	{
		std::vector<Paragraph>::const_iterator hi = m_parar.begin() + startidx + 1, he = m_parar.end();
		for (int hidx=startidx + 1; hi != he; ++hi,++hidx)
		{
			if (hi->type() == types[ ti])
			{
				m_tables.push_back( *hi);
	
				std::map<int,std::set<int> >::const_iterator xi;
				xi = start2colmap.find( hidx);
				if (xi != start2colmap.end())
				{
					addTableCellIdentifierAttributes( "C", xi->second);
				}
				xi = start2rowmap.find( hidx);
				if (xi != start2rowmap.end())
				{
					addTableCellIdentifierAttributes( "R", xi->second);
				}
				ParagraphRange range = findParagraphRange( hi, m_parar.end(), hi->type(), Paragraph::invType( hi->type()));
				if (hi != range.first) throw std::runtime_error("internal: corrupt table definition");
				m_tables.insert( m_tables.end(), range.first+1, range.second);

				int rangeSize = range.second - range.first;
				hidx += rangeSize - 1;
				hi += rangeSize -1;
			}
		}
	}
	// Print elements ouside of table separately for not loosing any text:
	std::vector<Paragraph> outsideTableItems;
	std::vector<Paragraph>::const_iterator hi = m_parar.begin() + startidx + 1, he = m_parar.end();
	while (hi != he)
	{
		if (hi->type() == Paragraph::TableHeadStart || hi->type() == Paragraph::TableCellStart || hi->type() == Paragraph::TableTitleStart)
		{
			ParagraphRange range = findParagraphRange( hi, m_parar.end(), hi->type(), Paragraph::invType( hi->type()));
			if (hi != range.first) throw std::runtime_error("internal: corrupt table definition");
			hi = range.second;
		}
		else if (hi->type() == Paragraph::TableEnd)
		{
			break;
		}
		else
		{
			outsideTableItems.push_back( *hi++);
		}
	}
	m_tables.push_back( Paragraph( Paragraph::TableEnd, "", ""));
	// Remove table written to buffer and add table link:
	m_parar.resize( startidx);
	m_parar.push_back( Paragraph( Paragraph::TableLink, tableid, ""));
	m_parar.insert( m_parar.end(), outsideTableItems.begin(), outsideTableItems.end());
	m_tableDefs.pop_back();
}

static bool isInternalLink( const Paragraph& para)
{
	return (para.type() == Paragraph::CitationLink || para.type() == Paragraph::RefLink || para.type() == Paragraph::TableLink);
}

void DocumentStructure::finishRef( int startidx, const std::string& refid)
{
	if ((std::size_t)startidx + 3 == m_parar.size() && isInternalLink( m_parar[ startidx + 1]))
	{
		Paragraph para = m_parar[ startidx + 1];
		m_parar.resize( startidx);
		m_parar.push_back( para);
	}
	else
	{
		std::string key( passageKey( m_parar.begin() + startidx, m_parar.end()));
		std::map<std::string,std::string>::const_iterator ki = m_refmap.find( key);
		if (ki == m_refmap.end())
		{
			m_refs.insert( m_refs.end(), m_parar.begin() + startidx, m_parar.end());
			m_refmap[ key] = refid;
			m_parar.resize( startidx);
			m_parar.push_back( Paragraph( Paragraph::RefLink, refid, ""));
		}
		else
		{
			m_parar.resize( startidx);
			m_parar.push_back( Paragraph( Paragraph::RefLink, ki->second, ""));
		}
	}
}

void DocumentStructure::finishCitation( int startidx, const std::string& citid)
{
	if ((std::size_t)startidx + 3 == m_parar.size() && isInternalLink( m_parar[ startidx + 1]))
	{
		Paragraph para = m_parar[ startidx + 1];
		m_parar.resize( startidx);
		m_parar.push_back( para);
	}
	else
	{
		std::string key( passageKey( m_parar.begin() + startidx, m_parar.end()));
		std::map<std::string,std::string>::const_iterator ki = m_citationmap.find( key);
		if (ki == m_citationmap.end())
		{
			m_citations.insert( m_citations.end(), m_parar.begin() + startidx, m_parar.end());
			m_citationmap[ key] = citid;
			m_parar.resize( startidx);
			m_parar.push_back( Paragraph( Paragraph::CitationLink, citid, ""));
		}
		else
		{
			m_parar.resize( startidx);
			m_parar.push_back( Paragraph( Paragraph::CitationLink, ki->second, ""));
		}
	}
}

void DocumentStructure::finishStructure( int startidx)
{
	Paragraph para = m_parar[ startidx];
	if (para.type() == Paragraph::PageLinkStart)
	{
		if (startidx == (int)m_parar.size()-1 && para.text().empty())
		{
			m_parar[ startidx].setText( para.id());
		}
	}
	Paragraph::Type endType = Paragraph::invType( para.type());
	m_parar.push_back( Paragraph( endType, "", ""));

	if (endType == Paragraph::CitationEnd)
	{
		finishCitation( startidx, para.id());
	}
	if (endType == Paragraph::RefEnd)
	{
		finishRef( startidx, para.id());
	}
	else if (endType == Paragraph::TableEnd)
	{
		finishTable( startidx, para.id());
	}
	m_structStack.pop_back();
}

void DocumentStructure::closeStructure( Paragraph::Type startType, const std::string& alt_text)
{
	closeDanglingStructures( startType);
	if (m_structStack.empty())
	{
		addError( strus::string_format( "close of %s structure called without open ()", Paragraph::structTypeName( Paragraph::structType( startType))));
		return;
	}
	int startidx = m_structStack.back().start;
	const Paragraph& para = m_parar[ startidx];
	if (para.type() == startType)
	{
		finishStructure( startidx);
	}
	else if (alt_text.empty())
	{
		const char* stnam = Paragraph::structTypeName( Paragraph::structType( para.type()));
		addError( strus::string_format( "close of %s structure called without open (%s)", Paragraph::structTypeName( Paragraph::structType( startType)), stnam));
		return;
	}
	else
	{
		addSingleItem( Paragraph::Text, "", alt_text, true);
	}
}

void DocumentStructure::closeOpenStructures()
{
	closeDanglingStructures( Paragraph::Title/*no match => remove all*/);
	m_parar.insert( m_parar.end(), m_refs.begin(), m_refs.end());
	m_refs.clear();
	m_parar.insert( m_parar.end(), m_tables.begin(), m_tables.end());
	m_tables.clear();
	m_parar.insert( m_parar.end(), m_citations.begin(), m_citations.end());
	m_citations.clear();
}

void DocumentStructure::closeWebLink()
{
	std::vector<Paragraph>::const_iterator pi = m_parar.end(), pe = m_parar.begin();
	for (; pi != pe; --pi)
	{
		const Paragraph& para = *(pi-1);
		if (para.type() == Paragraph::Text)
		{
			if (!para.text().empty() && 0!=std::strchr( para.text().c_str(), '['))
			{
				char const* se = para.text().c_str();
				char const* si = se + para.text().size();
				char ch = 0;
				for (; si != se; --si)
				{
					ch = *(si-1);
					if (ch == '[' || ch == ']') break;
				}
				if (ch == '[')
				{
					addText( "]");
					return;
				}
			}
		}
		else if (para.type() == Paragraph::Char)
		{}
		else if (para.type() == Paragraph::Math)
		{}
		else if (para.type() == Paragraph::EntityStart || para.type() == Paragraph::EntityEnd)
		{}
		else if (para.type() == Paragraph::QuotationStart || para.type() == Paragraph::QuotationEnd)
		{}
		else if (para.type() == Paragraph::DoubleQuoteStart || para.type() == Paragraph::DoubleQuoteEnd)
		{}
		else
		{
			break;
		}
	}
	closeStructure( Paragraph::WebLinkStart, "]");
}

static std::string getDocidFromTitle( const std::string& txt)
{
	std::string rt;
	char const* si = txt.c_str();
	while (*si && (unsigned char)*si <= 32) ++si;
	while (*si)
	{
		char ch = *si | 32;
		if ((unsigned char)*si <= 32)
		{
			while (*si && (unsigned char)*si <= 32) ++si;
			rt.push_back( '_');
		}
		else if ((ch >= 'a' && ch <= 'z') || (*si >= '0' && *si <= '9') || *si == '-' || *si == '_')
		{
			rt.push_back( *si++);
		}
		else if ((unsigned char)*si >= 128)
		{
			rt.push_back( '#');
			while ((unsigned char)*si >= 128)
			{
				char buf[ 16];
				std::snprintf( buf, sizeof(buf), "%02x", (unsigned int)(unsigned char)*si);
				rt.append( buf);
				++si;
			}
		}
		else
		{
			rt.push_back( '#');
			char buf[ 16];
			std::snprintf( buf, sizeof(buf), "%02x", (unsigned int)(unsigned char)*si);
			rt.append( buf);
			++si;
		}
	}
	if (!rt.empty() && rt[ rt.size()-1] == '_') rt.resize( rt.size()-1);
	return rt;
}

static std::string encodeXmlContentString( const std::string& txt, bool encodeEoln)
{
	std::string rt;
	char const* si = txt.c_str();
	for (; *si; ++si)
	{
		if ((unsigned char)*si < 32 && encodeEoln)
		{
			rt.push_back( ' ');
		}
		else if (*si == '\"')
		{
			rt.append( "&quot;");
		}
		else if (*si == '&')
		{
			rt.append( "&amp;");
		}
		else if (*si == '<')
		{
			rt.append( "&lt;");
		}
		else if (*si == '>')
		{
			rt.append( "&gt;");
		}
		else
		{
			rt.push_back( *si);
		}
	}
	return rt;
}

void DocumentStructure::setTitle( const std::string& text)
{
	Paragraph para( Paragraph::Title, m_id = getDocidFromTitle( text), text);
	if (!m_parar.empty() && m_parar[0].type() == Paragraph::Title)
	{
		m_parar[0] = para;
	}
	else
	{
		m_parar.insert( m_parar.begin(), para);
	}
}

Paragraph::StructType DocumentStructure::currentStructType() const
{
	if (m_structStack.empty()) return Paragraph::StructNone;
	return m_parar[ m_structStack.back().start].structType();
}

int DocumentStructure::currentStructIndex() const
{
	if (m_structStack.empty()) return 0;
	return m_structStack.back().idx;
}

static bool isSpaceOnlyText( const std::string& text)
{
	char const* si = text.c_str();
	for (; *si && (unsigned char)*si <= 32; ++si){}
	return !*si;
}

void DocumentStructure::clearOpenText()
{
	if (!m_parar.empty() && m_parar.back().type() == Paragraph::Text)
	{
		m_parar.pop_back();
	}
}

static bool isJoinLinkChar( unsigned char ch)
{
	if (ch >= 128) return true;
	if (ch <= 32) return false;
	if ((ch|32) >= 'a' && (ch|32) <= 'z') return true;
	if (ch >= '0' && ch <= '9') return true;
	if (ch == '-') return false;
	if (ch == '_') return true;
	return false;
}

static bool isJoinLinkText( const std::string& text)
{
	if (text.empty()) return true;
	return isJoinLinkChar( (unsigned char)text[0]);
}

std::pair<std::string,std::string> splitJoinLinkWords( const std::string& text)
{
	std::pair<std::string,std::string> rt;
	char const* si = text.c_str();
	while (*si && isJoinLinkChar(*si)) ++si;
	rt.first.append( text.c_str(), si - text.c_str());
	rt.second.append( si);
	return rt;
}

void DocumentStructure::addSingleItem( Paragraph::Type type, const std::string& id, const std::string& text, bool joinText)
{
	if (joinText)
	{
		if (!m_parar.empty() && m_parar.back().type() == Paragraph::AttributeStart && type == Paragraph::Text)
		{
			m_parar.back().addText( text);
		}
		else if (m_parar.back().type() == Paragraph::Text && isSpaceOnlyText( text) && (type == Paragraph::Text || type == Paragraph::Char || type == Paragraph::BibRef || type == Paragraph::NoWiki || type == Paragraph::Math || type == Paragraph::Timestamp))
		{
			if (!text.empty() && !m_parar.empty())
			{
				if (0!=std::strchr( text.c_str(), '\n'))
				{
					m_parar.back().addText( "\n");
				}
				else
				{
					m_parar.back().addText( " ");
				}
			}
		}
		else if (!m_parar.empty() && m_parar.back().type() == Paragraph::Text && type == Paragraph::Text)
		{
			m_parar.back().addText( text);
		}
		else if (m_parar.size() >= 2
		&&	(
				(m_parar[ m_parar.size()-1].type() == Paragraph::PageLinkEnd
				 && (m_parar[ m_parar.size()-2].type() == Paragraph::Text
					|| m_parar[ m_parar.size()-2].type() == Paragraph::PageLinkStart)
				 && type == Paragraph::Text && isJoinLinkText(text))

			||	(m_parar[ m_parar.size()-1].type() == Paragraph::WebLinkEnd
				 && (m_parar[ m_parar.size()-2].type() == Paragraph::Text
					|| m_parar[ m_parar.size()-2].type() == Paragraph::PageLinkStart)
				 && type == Paragraph::Text && isJoinLinkText(text))
			))
		{
			std::pair<std::string,std::string> sptext = splitJoinLinkWords( text);
			m_parar[ m_parar.size()-2].addText( sptext.first);
			m_parar.push_back( Paragraph( type, id, sptext.second));
		}
		else
		{
			m_parar.push_back( Paragraph( type, id, text));
		}
	}
	else
	{
		m_parar.push_back( Paragraph( type, id, text));
	}
}

void DocumentStructure::finish()
{
	closeOpenStructures();
	checkStartEndSectionBalance( m_parar.begin(), m_parar.end());
}

std::string DocumentStructure::getInputXML( const std::string& title, const std::string& content)
{
	std::string rt;
	std::vector<Paragraph::StructType> stk;
	XmlPrinter output;
	output.printHeader( rt);
	output.printOpenTag( "mediawiki", rt);
	output.printOpenTag( "page", rt);
	output.printOpenTag( "title", rt);
	output.printValue( title, rt);
	output.printCloseTag(rt);//title
	output.printOpenTag( "revision", rt);
	output.printOpenTag( "text", rt);
	output.printValue( content, rt);
	output.printCloseTag(rt);//text
	output.printCloseTag(rt);//revision
	output.printCloseTag(rt);//page
	output.printCloseTag(rt);//mediawiki
	return rt;
}

static void printTagOpen( XmlPrinter& output, std::string& rt, const char* tagnam, const std::string& id, const std::string& text)
{
	output.printOpenTag( tagnam, rt);
	if (!id.empty())
	{
		output.printAttribute( "id", rt);
		output.printValue( id, rt);
	}
	if (!text.empty())
	{
		output.switchToContent( rt);
		output.printValue( text, rt);
	}
}

static void printTagContent( XmlPrinter& output, std::string& rt, const char* tagnam, const std::string& id, const std::string& text)
{
	printTagOpen( output, rt, tagnam, id, text);
	output.printCloseTag( rt);
}

static std::string collectIntagAttributeValue( std::vector<Paragraph>::const_iterator pi, const std::vector<Paragraph>::const_iterator& pe)
{
	std::string rt;
	if (pi->type() == Paragraph::AttributeStart)
	{
		rt = pi->text();
	}
	++pi;
	if (pi->type() == Paragraph::AttributeEnd)
	{
		return rt;
	}
	else
	{
		if (!pi->id().empty()) return std::string();
		if (pi->type() == Paragraph::Text || pi->type() == Paragraph::BibRef || pi->type() == Paragraph::Timestamp)
		{
			if (!rt.empty()) return std::string();
			rt = pi->text();
		}
		else
		{
			return std::string();
		}
		++pi;
		if (pi->type() == Paragraph::AttributeEnd) return rt;
	}
	return std::string();
}

static std::string collectAttributeText( std::vector<Paragraph>::const_iterator pi, const std::vector<Paragraph>::const_iterator& pe)
{
	std::string rt;
	if (pi->type() == Paragraph::AttributeStart)
	{
		rt = pi->text();
	}
	++pi;
	if (pi->type() == Paragraph::AttributeEnd)
	{
		return rt;
	}
	else
	{
		if (!pi->id().empty()) return std::string();
		if (pi->type() == Paragraph::Text)
		{
			rt.append( pi->text());
		}
		else
		{
			return std::string();
		}
		++pi;
		if (pi->type() == Paragraph::AttributeEnd) return rt;
	}
	return std::string();
}

std::string DocumentStructure::toxml( bool beautified) const
{
	std::string rt;
	std::vector<Paragraph::StructType> stk;
	XmlPrinter output;
	output.printHeader( rt);
	output.printOpenTag( "doc", rt);
	std::vector<Paragraph>::const_iterator pi = m_parar.begin(), pe = m_parar.end();
	for(int pidx=0; pi != pe; ++pi,++pidx)
	{
		if (beautified && !output.isInTagDeclaration())
		{
			output.printValue( std::string("\n") + std::string( 2*stk.size(), ' '), rt);
		}
		switch (pi->type())
		{
			case Paragraph::Title:
				printTagContent( output, rt, "docid", "", pi->id());
				if (beautified) output.printValue( std::string("\n") + std::string( 2*stk.size(), ' '), rt);
				printTagContent( output, rt, "title", "", pi->text());
				break;
			case Paragraph::EntityStart:
				stk.push_back( Paragraph::StructEntity);
				printTagOpen( output, rt, "entity", pi->id(), pi->text());
				break;
			case Paragraph::EntityEnd:
				stk.pop_back();
				output.printCloseTag( rt);
				break;
			case Paragraph::QuotationStart:
				stk.push_back( Paragraph::StructQuotation);
				printTagOpen( output, rt, "quot", pi->id(), pi->text());
				break;
			case Paragraph::QuotationEnd:
				stk.pop_back();
				output.printCloseTag( rt);
				break;
			case Paragraph::DoubleQuoteStart:
				stk.push_back( Paragraph::StructDoubleQuote);
				printTagOpen( output, rt, "dquot", pi->id(), pi->text());
				break;
			case Paragraph::DoubleQuoteEnd:
				stk.pop_back();
				output.printCloseTag( rt);
				break;
			case Paragraph::BlockQuoteStart:
				stk.push_back( Paragraph::StructBlockQuote);
				break;
			case Paragraph::BlockQuoteEnd:
				stk.pop_back();
				break;
			case Paragraph::DivStart:
				stk.push_back( Paragraph::StructDiv);
				break;
			case Paragraph::DivEnd:
				stk.pop_back();
				break;
			case Paragraph::PoemStart:
				stk.push_back( Paragraph::StructPoem);
				break;
			case Paragraph::PoemEnd:
				stk.pop_back();
				break;
			case Paragraph::SpanStart:
				stk.push_back( Paragraph::StructSpan);
				break;
			case Paragraph::SpanEnd:
				stk.pop_back();
				break;
			case Paragraph::FormatStart:
				stk.push_back( Paragraph::StructFormat);
				break;
			case Paragraph::FormatEnd:
				stk.pop_back();
				break;
			case Paragraph::HeadingStart:
				stk.push_back( Paragraph::StructHeading);
				printTagOpen( output, rt, "heading", pi->id(), pi->text());
				break;
			case Paragraph::HeadingEnd:
				stk.pop_back();
				output.printCloseTag( rt);
				break;
			case Paragraph::ListItemStart:
				stk.push_back( Paragraph::StructList);
				printTagOpen( output, rt, "list", pi->id(), pi->text());
				break;
			case Paragraph::ListItemEnd:
				stk.pop_back();
				output.printCloseTag( rt);
				break;
			case Paragraph::AttributeStart:
			{
				if (output.isInTagDeclaration() && !pi->id().empty())
				{
					std::string attrid = pi->id();
					std::string attrtext = string_conv::trim( collectIntagAttributeValue( pi, pe));
					if (!attrtext.empty())
					{
						while (pi->type() != Paragraph::AttributeEnd) {++pi,++pidx;}
						output.printAttribute( attrid, rt);
						output.printValue( attrtext, rt);
					}
					else
					{
						stk.push_back( Paragraph::StructAttribute);
						printTagOpen( output, rt, "attr", pi->id(), pi->text());
					}
				}
				else
				{
					std::string text = collectAttributeText( pi, pe);
					if (!text.empty())
					{
						while (pi->type() != Paragraph::AttributeEnd) {++pi,++pidx;}
						printTagContent( output, rt, "text", pi->id(), text);
					}
					else
					{
						stk.push_back( Paragraph::StructAttribute);
						printTagOpen( output, rt, "attr", pi->id(), pi->text());
					}
				}
				break;
			}
			case Paragraph::AttributeEnd:
				stk.pop_back();
				output.printCloseTag( rt);
				break;
			case Paragraph::CitationStart:
				stk.push_back( Paragraph::StructCitation);
				printTagOpen( output, rt, "citation", pi->id(), pi->text());
				break;
			case Paragraph::CitationEnd:
				stk.pop_back();
				output.printCloseTag( rt);
				break;
			case Paragraph::RefStart:
				stk.push_back( Paragraph::StructRef);
				printTagOpen( output, rt, "ref", pi->id(), pi->text());
				break;
			case Paragraph::RefEnd:
				stk.pop_back();
				output.printCloseTag( rt);
				break;
			case Paragraph::PageLinkStart:
				stk.push_back( Paragraph::StructPageLink);
				printTagOpen( output, rt, "pagelink", pi->id(), pi->text());
				break;
			case Paragraph::PageLinkEnd:
				stk.pop_back();
				output.printCloseTag( rt);
				break;
			case Paragraph::WebLinkStart:
				stk.push_back( Paragraph::StructWebLink);
				printTagOpen( output, rt, "weblink", pi->id(), pi->text());
				break;
			case Paragraph::WebLinkEnd:
				stk.pop_back();
				output.printCloseTag( rt);
				break;
			case Paragraph::TableStart:
				stk.push_back( Paragraph::StructTable);
				printTagOpen( output, rt, "table", pi->id(), pi->text());
				break;
			case Paragraph::TableEnd:
				stk.pop_back();
				output.printCloseTag( rt);
				break;
			case Paragraph::TableTitleStart:
				stk.push_back( Paragraph::StructTableTitle);
				printTagOpen( output, rt, "description", pi->id(), pi->text());
				break;
			case Paragraph::TableTitleEnd:
				stk.pop_back();
				output.printCloseTag( rt);
				break;
			case Paragraph::TableHeadStart:
				stk.push_back( Paragraph::StructTableHead);
				printTagOpen( output, rt, "head", "", "");
				break;
			case Paragraph::TableHeadEnd:
				stk.pop_back();
				output.printCloseTag( rt);
				break;
			case Paragraph::TableCellStart:
				stk.push_back( Paragraph::StructTableCell);
				printTagOpen( output, rt, "cell", "", "");
				break;
			case Paragraph::TableCellEnd:
				stk.pop_back();
				output.printCloseTag( rt);
				break;
			case Paragraph::Text:
				printTagContent( output, rt, "text", pi->id(), pi->text());
				break;
			case Paragraph::Char:
				printTagContent( output, rt, "char", pi->id(), pi->text());
				break;
			case Paragraph::BibRef: 
				printTagContent( output, rt, "bibref", pi->id(), pi->text());
				break;
			case Paragraph::NoWiki:
				printTagContent( output, rt, "nowiki", pi->id(), pi->text());
				break;
			case Paragraph::Code:
				printTagContent( output, rt, "code", pi->id(), pi->text());
				break;
			case Paragraph::Math:
				printTagContent( output, rt, "math", pi->id(), pi->text());
				break;
			case Paragraph::Timestamp:
				printTagContent( output, rt, "time", pi->id(), pi->text());
				break;
			case Paragraph::CitationLink:
				printTagContent( output, rt, "citlink", pi->id(), pi->text());
				break;
			case Paragraph::RefLink:
				printTagContent( output, rt, "reflink", pi->id(), pi->text());
				break;
			case Paragraph::TableLink:
				printTagContent( output, rt, "tablink", pi->id(), pi->text());
				break;
		}
	}
	output.printCloseTag( rt);
	return rt;
}

std::string DocumentStructure::tostring() const
{
	std::ostringstream out;
	std::vector<Paragraph>::const_iterator pi = m_parar.begin(), pe = m_parar.end();
	for(int pidx=0; pi != pe; ++pi,++pidx)
	{
		out << pidx << " " << pi->typeName() << " " << encodeXmlContentString( pi->id(), true) << " \"" << encodeXmlContentString( pi->text(), true) << "\"\n";
	}
	return out.str();
}

std::string DocumentStructure::reportStrangeFeatures() const
{
	std::ostringstream out;
	std::vector<Paragraph>::const_iterator pi = m_parar.begin(), pe = m_parar.end();
	for(int pidx=0; pi != pe; ++pi,++pidx)
	{
		if (pi->type() == Paragraph::Text)
		{
			char const* si = pi->text().c_str();
			if (0!=std::strstr( si, "bgcolor="))
			{
				const char* featptr = std::strstr( si, "bgcolor=");
				std::string feat( featptr, 8);
				out << pidx << " " << pi->typeName() << " " << feat << " [" << encodeXmlContentString( pi->text(), true) << "]\n";
			}
			else if (0!=std::strstr( si, "align="))
			{
				const char* featptr = std::strstr( si, "align=");
				std::string feat( featptr, 6);
				out << pidx << " " << pi->typeName() << " " << feat << " [" << encodeXmlContentString( pi->text(), true) << "]\n";
			}
			else if (0!=std::strstr( si, "width="))
			{
				const char* featptr = std::strstr( si, "width=");
				std::string feat( featptr, 6);
				out << pidx << " " << pi->typeName() << " " << feat << " [" << encodeXmlContentString( pi->text(), true) << "]\n";
			}
			else if (0!=std::strstr( si, "style="))
			{
				const char* featptr = std::strstr( si, "style=");
				std::string feat( featptr, 6);
				out << pidx << " " << pi->typeName() << " " << feat << " [" << encodeXmlContentString( pi->text(), true) << "]\n";
			}
			else if (0!=std::strstr( si, "class="))
			{
				const char* featptr = std::strstr( si, "class=");
				std::string feat( featptr, 6);
				out << pidx << " " << pi->typeName() << " " << feat << " [" << encodeXmlContentString( pi->text(), true) << "]\n";
			}
			else while (*si)
			{
				for (; *si && (unsigned char)*si <= 32; ++si){}
				int sidx = 0;
				char cls = 0;
				char prev_cls = 0;
				int cls_chg = 0;
				char const* start = si;
				for (; *si; ++si,++sidx)
				{
					prev_cls = cls;
	
					if ((*si|32) <= 'z' && (*si|32) >= 'a') cls = 'a';
					else if (*si >= '0' && *si <= '9') cls = 'd';
					else break;
	
					if (prev_cls != cls)
					{
						cls_chg += 1;
					}
				}
				if (sidx * cls_chg > 48)
				{
					std::string feat( start, si-start);
					out << pidx << " " << pi->typeName() << " " << feat << " [" << encodeXmlContentString( pi->text(), true) << "]\n";
				}
				if (*si) ++si;
			}
		}
	}
	return out.str();
}

std::string DocumentStructure::statestring() const
{
	std::string rt;
	std::vector<StructRef>::const_iterator ri = m_structStack.begin(), re = m_structStack.end();
	for (int ridx=0; re != ri; --re,++ridx)
	{
		if (ridx) rt.append( ", ");
		rt.append( Paragraph::structTypeName( Paragraph::structType( m_parar[ (*(re-1)).start].type())));
	}
	if (!m_tableDefs.empty())
	{
		rt.append( strus::string_format( " [tables %d]", (int)m_tableDefs.size()));
	}
	return rt;
}

void DocumentStructure::setErrorsSourceInfo( const std::string& msg)
{
	for (; m_nofErrors < (int)m_errors.size(); ++m_nofErrors)
	{
		std::string& err = m_errors[ m_nofErrors];
		if (!err.empty() && err[ err.size()-1] == ']')
		{
			err[ err.size()-1] = ',';
			err.push_back( ' ');
		}
		else
		{
			err.append( " [");
		}
		err.append( " source: (");
		err.append( msg);
		err.append( "...)]");
	}
	
}

