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
#include <stdexcept>
#include <iostream>
#include <sstream>

typedef textwolf::XMLPrinter<textwolf::charset::UTF8,textwolf::charset::UTF8,std::string> XmlPrinterBase;

#ifdef UNUSED_FUNCTIONS
static bool isSpace( char ch)
{
	return ((unsigned char)ch <= 32);
}

static bool isAlpha( char ch)
{
	return (ch|32) >= 'a' && (ch|32) <= 'z';
}

static bool isDigit( char ch)
{
	return (ch) >= '0' && (ch) <= '9';
}

static bool isAlphaNum( char ch)
{
	return isAlpha(ch) || isDigit(ch);
}

static bool isAttributeString( const std::string& src)
{
	if (src.size() > 80) return false;
	std::string::const_iterator si = src.begin(), se = src.end();
	for (; si != se && (*si == ' ' || *si == '-' || *si == '_' || isAlphaNum(*si)); ++si){}
	return (si == se);
}
static bool isEmpty( const std::string& src)
{
	if (src.size() > 80) return false;
	std::string::const_iterator si = src.begin(), se = src.end();
	for (; si != se && isSpace(*si); ++si){}
	return (si == se);
}
static std::string makeAttributeName( const std::string& src)
{
	std::string rt;
	std::string::const_iterator si = src.begin(), se = src.end();
	for (; si != se && (*si == ' ' || *si == '-' || *si == '_' || isAlphaNum(*si)); ++si)
	{
		if (isAlpha(*si))
		{
			rt.push_back( *si|32);
		}
		else if (isDigit(*si))
		{
			rt.push_back( *si);
		}
		else if (*si == '_' || *si == '-')
		{
			rt.push_back( '_');
		}
	}
	return rt;
}
#endif

/// \brief strus toplevel namespace
using namespace strus;

class XmlPrinter
	:public XmlPrinterBase
{
public:
	XmlPrinter( bool subDocument=false)
		:XmlPrinterBase(subDocument){}

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

void DocumentStructure::checkStartEndSectionBalance( const std::vector<Paragraph>::const_iterator& start, const std::vector<Paragraph>::const_iterator& end)
{
	std::vector<Paragraph::Type> stk;
	std::vector<Paragraph>::const_iterator ii = start;
	for (; ii != end; ++ii)
	{
		switch (ii->type())
		{
			case Paragraph::Title:
				break;
			case Paragraph::Heading:
				break;
			case Paragraph::TableStart:
			case Paragraph::ListItemStart:
			case Paragraph::CitationStart:
			case Paragraph::TableRowStart:
				stk.push_back( ii->type());
				break;
			case Paragraph::TableEnd:
				if (stk.empty()) throw std::runtime_error( strus::string_format( "structure open/close not balanced: %s...%s", "", ii->typeName()));
				if (stk.back() != Paragraph::TableStart) throw std::runtime_error( strus::string_format( "structure open/close not balanced: %s...%s", Paragraph::typeName( stk.back()), ii->typeName()));
			case Paragraph::ListItemEnd:
				if (stk.empty()) throw std::runtime_error( strus::string_format( "structure open/close not balanced: %s...%s", "", ii->typeName()));
				if (stk.back() != Paragraph::ListItemStart) throw std::runtime_error( strus::string_format( "structure open/close not balanced: %s...%s", Paragraph::typeName( stk.back()), ii->typeName()));
			case Paragraph::CitationEnd:
				if (stk.empty()) throw std::runtime_error( strus::string_format( "structure open/close not balanced: %s...%s", "", ii->typeName()));
				if (stk.back() != Paragraph::CitationStart) throw std::runtime_error( strus::string_format( "structure open/close not balanced: %s...%s", Paragraph::typeName( stk.back()), ii->typeName()));
				break;
			case Paragraph::TableRowEnd:
				if (stk.empty()) throw std::runtime_error( strus::string_format( "structure open/close not balanced: %s...%s", "", ii->typeName()));
				if (stk.back() != Paragraph::TableRowStart) throw std::runtime_error( strus::string_format( "structure open/close not balanced: %s...%s", Paragraph::typeName( stk.back()), ii->typeName()));
				break;
			case Paragraph::TableCol:
				break;
			case Paragraph::Text:
				break;
			case Paragraph::PageLink:
				break;
			case Paragraph::WebLink:
				break;
			case Paragraph::CitationLink:
				break;
		}
	}
	if (!stk.empty()) throw std::runtime_error( strus::string_format( "structure open/close not balanced: %s...%s", Paragraph::typeName( stk.back()), ""));
}

void DocumentStructure::openListItem( int lidx)
{
	while (!m_structStack.empty() && m_parar[ m_structStack.back().start].type() == Paragraph::ListItemStart && m_structStack.back().idx <= lidx)
	{
		Paragraph para = m_parar[ m_structStack.back().start];
		m_parar.push_back( Paragraph( Paragraph::ListItemEnd, para.id(), para.text()));
		m_structStack.pop_back();
	}
	m_structStack.push_back( StructRef( lidx, m_parar.size()));
	m_parar.push_back( Paragraph( Paragraph::ListItemStart, strus::string_format("l%d", lidx), ""));
}

void DocumentStructure::closeOpenListItem()
{
	if (m_structStack.empty()) return;
	Paragraph para = m_parar[ m_structStack.back().start];
	if (para.type() == Paragraph::ListItemStart)
	{
		m_parar.push_back( Paragraph( Paragraph::ListItemEnd, para.id(), para.text()));
		m_structStack.pop_back();
	}
}

void DocumentStructure::closeOpenCitation()
{
	if (m_structStack.empty()) return;
	int start = m_structStack.back().start;
	Paragraph para = m_parar[ start];
	if (para.type() == Paragraph::CitationStart)
	{
		m_parar.push_back( Paragraph( Paragraph::CitationEnd, para.id(), ""));
		m_citations.insert( m_citations.end(), m_parar.begin() + start, m_parar.end());
		m_parar.resize( start);
		m_parar.push_back( Paragraph( Paragraph::CitationLink, para.id(), ""));
		m_structStack.pop_back();
	}
}

void DocumentStructure::closeDanglingStructures( const Paragraph::Type& starttype)
{
	while (!m_structStack.empty())
	{
		Paragraph para = m_parar[ m_structStack.back().start];
		if (para.type() == starttype)
		{
			return;
		}
		else if (para.type() == Paragraph::TableRowStart)
		{
			m_parar.push_back( Paragraph( Paragraph::TableRowEnd, para.id(), para.text()));
			m_structStack.pop_back();
			if (!m_tableDefs.empty()) m_tableDefs.back().coliter = 0;
		}
		else if (para.type() == Paragraph::TableStart)
		{
			m_parar.push_back( Paragraph( Paragraph::TableEnd, para.id(), para.text()));
			m_structStack.pop_back();
			if (!m_tableDefs.empty()) m_tableDefs.pop_back();
		}
		else if (para.type() == Paragraph::ListItemStart)
		{
			m_parar.push_back( Paragraph( Paragraph::ListItemEnd, para.id(), para.text()));
			m_structStack.pop_back();
		}
		else if (para.type() == Paragraph::CitationStart)
		{
			closeOpenCitation();
		}
		else
		{
			throw std::runtime_error("internal: unknown structure");
		}
	}
}

void DocumentStructure::closeOpenStructures()
{
	closeDanglingStructures( Paragraph::Title/*no match => remove all*/);
}

void DocumentStructure::openCitation()
{
	m_structStack.push_back( StructRef( 0, m_parar.size()));
	m_parar.push_back( Paragraph( Paragraph::CitationStart, strus::string_format("cit%d", ++m_citationCnt), ""));
}

void DocumentStructure::closeCitation()
{
	closeDanglingStructures( Paragraph::CitationStart);
	if (m_structStack.empty())
	{
		m_errors.push_back( "close citation called without open");
		return;
	}
	Paragraph para = m_parar[ m_structStack.back().start];
	if (para.type() != Paragraph::CitationStart)
	{
		m_errors.push_back( "close citation called without open");
		return;
	}
	closeOpenCitation();
}

static std::string getIdentifier( const std::string& txt)
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
		else if ((ch >= 'a' && ch <= 'z') || (*si >= '0' && *si <= '9') || *si == '-')
		{
			rt.push_back( *si++);
		}
		else if ((unsigned char)*si >= 128)
		{
			rt.push_back( '#');
			while ((unsigned char)*si >= 128)
			{
				char buf[ 16];
				std::snprintf( buf, sizeof(buf), "%02u", (unsigned int)(unsigned char)*si);
				rt.append( buf);
				++si;
			}
		}
		else
		{
			rt.push_back( '#');
			char buf[ 16];
			std::snprintf( buf, sizeof(buf), "%02u", (unsigned int)(unsigned char)*si);
			rt.append( buf);
			++si;
		}
	}
	if (!rt.empty() && rt[ rt.size()-1] == '_') rt.pop_back();
	return rt;
}

static std::string encodeString( const std::string& txt, bool encodeEoln)
{
	std::string rt;
	char const* si = txt.c_str();
	for (; *si; ++si)
	{
		if ((unsigned char)*si < 32 && encodeEoln)
		{
			rt.push_back( ' ');
		}
		if (*si == '\"')
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
	Paragraph para( Paragraph::Title, m_id = getIdentifier(text), text);
	if (!m_parar.empty() && m_parar[0].type() == Paragraph::Title)
	{
		m_parar[0] = para;
	}
	else
	{
		m_parar.insert( m_parar.begin(), para);
	}
}

void DocumentStructure::addHeading( int idx, const std::string& text)
{
	closeOpenStructures();
	m_parar.push_back( Paragraph( Paragraph::Heading, strus::string_format("h%d", idx), text));
}

void DocumentStructure::addText( const std::string& text)
{
	m_parar.push_back( Paragraph( Paragraph::Text, "", text));
}

void DocumentStructure::addPageLink( const std::string& id, const std::string& text)
{
	m_parar.push_back( Paragraph( Paragraph::PageLink, id, text));
}

void DocumentStructure::addWebLink( const std::string& id, const std::string& text)
{
	m_parar.push_back( Paragraph( Paragraph::WebLink, id, text));
}

void DocumentStructure::openTable( const std::string& title, const std::vector<std::string>& coltitles)
{
	m_tableDefs.push_back( TableDef( coltitles));
	m_structStack.push_back( StructRef( 0, m_parar.size()));
	m_parar.push_back( Paragraph( Paragraph::TableStart, strus::string_format("tab%d", (int)m_tableDefs.size()), title));
}

void DocumentStructure::addTableRow()
{
	if (m_tableDefs.empty())
	{
		m_errors.push_back( "add table row called without open table");
		return;
	}
	closeOpenTableRow();
	int rowiter = m_tableDefs.back().rowiter++;
	m_structStack.push_back( StructRef( rowiter, m_parar.size()));
	m_parar.push_back( Paragraph( Paragraph::TableRowStart, strus::string_format("row%d", rowiter), ""));
}

void DocumentStructure::closeOpenTableRow()
{
	if (m_tableDefs.empty())
	{
		m_errors.push_back( "close open table row called without open table");
		return;
	}
	closeDanglingStructures( Paragraph::TableRowStart);
	if (m_tableDefs.back().rowiter)
	{
		Paragraph para = m_parar[ m_structStack.back().start];
		if (para.type() == Paragraph::TableRowStart)
		{
			m_parar.push_back( Paragraph( Paragraph::TableRowEnd, para.id(), para.text()));
			m_structStack.pop_back();
		}
		else
		{
			m_errors.push_back( "close open table row called without open table structure");
		}
	}
	m_tableDefs.back().coliter = 0;
}

void DocumentStructure::addTableCol( const std::string& text)
{
	if (m_tableDefs.empty())
	{
		m_errors.push_back( "add table row called without open table");
		return;
	}
	int coliter = m_tableDefs.back().coliter++;
	if (coliter >= (int)m_tableDefs.back().coltitles.size())
	{
		m_errors.push_back( "column index of table exceeds number of columns");
		return;
	}
	m_parar.push_back( Paragraph( Paragraph::TableCol, m_tableDefs.back().coltitles[ coliter], text));
	
}

void DocumentStructure::closeTable()
{
	closeOpenTableRow();
	closeDanglingStructures( Paragraph::TableStart);
	if (m_structStack.empty())
	{
		m_errors.push_back( "close table called without open");
		return;
	}
	Paragraph para = m_parar[ m_structStack.back().start];
	if (para.type() == Paragraph::TableStart)
	{
		m_tableDefs.pop_back();
		m_parar.push_back( Paragraph( Paragraph::TableEnd, para.id(), para.text()));
		m_structStack.pop_back();
		m_tableDefs.pop_back();
	}
	else
	{
		m_errors.push_back( "close table called without open");
		return;
	}
}

void DocumentStructure::addAttribute( const std::string& id, const std::string& text)
{
	if (m_parar.empty())
	{
		m_errors.push_back( "add attribute without context");
		return;
	}
	if (m_parar.back().type() == Paragraph::CitationStart)
	{
		m_parar.back().addAttribute( id, text);
	}
	else
	{
		m_errors.push_back( "add attribute outside of a context");
	}
}

void DocumentStructure::finish()
{
	m_parar.insert( m_parar.end(), m_citations.begin(), m_citations.end());
	m_citations.clear();
	checkStartEndSectionBalance( m_parar.begin(), m_parar.end());
}

std::string DocumentStructure::toxml( bool subDocument) const
{
	std::string rt;
	XmlPrinter output( subDocument);
	output.printOpenTag( "doc", rt);
	std::vector<Paragraph>::const_iterator pi = m_parar.begin(), pe = m_parar.end();
	for(; pi != pe; ++pi)
	{
		switch (pi->type())
		{
			case Paragraph::Title:
				output.printOpenTag( "docid", rt);
				output.printValue( pi->id(), rt);
				output.printCloseTag( rt);
				output.printOpenTag( "title", rt);
				output.printValue( pi->text(), rt);
				output.printCloseTag( rt);
				break;
			case Paragraph::Heading:
				output.printOpenTag( pi->id(), rt);
				output.printValue( pi->text(), rt);
				output.printCloseTag( rt);
				break;
			case Paragraph::TableStart:
				output.printOpenTag( pi->id(), rt);
				output.printAttribute( "title", rt);
				output.printValue( pi->text(), rt);
				output.switchToContent( rt);
				break;
			case Paragraph::TableEnd:
				output.printCloseTag( rt);
				break;
			case Paragraph::TableRowStart:
				output.printOpenTag( pi->id(), rt);
				break;
			case Paragraph::TableRowEnd:
				output.printCloseTag( rt);
				break;
			case Paragraph::TableCol:
				output.printOpenTag( pi->id(), rt);
				output.printCloseTag( rt);
				output.printAttribute( "title", rt);
				output.printValue( pi->text(), rt);
				output.switchToContent( rt);
				break;
			case Paragraph::ListItemStart:
				output.printOpenTag( pi->id(), rt);
				break;
			case Paragraph::ListItemEnd:
				output.printCloseTag( rt);
				break;
			case Paragraph::CitationStart:
				output.printOpenTag( pi->id(), rt);
				break;
			case Paragraph::CitationEnd:
				output.printCloseTag( rt);
				break;
			case Paragraph::Text:
				output.printValue( pi->text(), rt);
				break;
			case Paragraph::PageLink:
				output.printOpenTag( "link", rt);
				output.printAttribute( "id", rt);
				output.printValue( pi->id(), rt);
				output.switchToContent( rt);
				output.printValue( pi->text(), rt);
				output.printCloseTag( rt);
				break;
			case Paragraph::WebLink:
				output.printOpenTag( "href", rt);
				output.printAttribute( "id", rt);
				output.printValue( pi->id(), rt);
				output.switchToContent( rt);
				output.printValue( pi->text(), rt);
				output.printCloseTag( rt);
				break;
			case Paragraph::CitationLink:
				output.printOpenTag( "cit", rt);
				output.printAttribute( "id", rt);
				output.printValue( pi->id(), rt);
				output.switchToContent( rt);
				output.printValue( pi->text(), rt);
				output.printCloseTag( rt);
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
	for(; pi != pe; ++pi)
	{
		out << pi->typeName() << " " << encodeString( pi->id(), true) << " \"" << encodeString( pi->text(), true) << "\"\n";
	}
	return out.str();
}

