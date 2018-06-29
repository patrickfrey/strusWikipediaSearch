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
	XmlPrinter()
		:XmlPrinterBase(){}

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

static std::string collectQuoteText( std::vector<Paragraph>::const_iterator pi, const std::vector<Paragraph>::const_iterator& pe)
{
	std::string rt;
	for (; pi != pe && pi->type() != Paragraph::EntityEnd; ++pi)
	{
		if (pi->type() == Paragraph::PageLink || pi->type() == Paragraph::WebLink || pi->type() == Paragraph::Text || pi->type() == Paragraph::NoWiki || pi->type() == Paragraph::Math)
		{
			rt.append( pi->text());
		}
	}
	return rt;
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
			case Paragraph::HeadingStart:
			case Paragraph::ListItemStart:
			case Paragraph::CitationStart:
			case Paragraph::RefStart:
			case Paragraph::TableStart:
			case Paragraph::TableTitleStart:
			case Paragraph::TableHeadStart:
			case Paragraph::TableRowStart:
			case Paragraph::TableColStart:
				stk.push_back( ii->type());
				break;
			case Paragraph::EntityEnd:
			case Paragraph::HeadingEnd:
			case Paragraph::ListItemEnd:
			case Paragraph::CitationEnd:
			case Paragraph::RefEnd:
			case Paragraph::TableEnd:
			case Paragraph::TableTitleEnd:
			case Paragraph::TableHeadEnd:
			case Paragraph::TableRowEnd:
			case Paragraph::TableColEnd:
				if (stk.empty()) throw std::runtime_error( strus::string_format( "structure open/close not balanced: ...%s", ii->typeName()));
				if (stk.back() != Paragraph::invType( ii->type())) throw std::runtime_error( strus::string_format( "structure open/close not balanced: %s...%s", Paragraph::typeName( stk.back()), ii->typeName()));
				stk.pop_back();
				break;
			case Paragraph::Title:
			case Paragraph::Attribute:
			case Paragraph::Text:
			case Paragraph::NoWiki:
			case Paragraph::Math:
			case Paragraph::PageLink:
			case Paragraph::WebLink:
			case Paragraph::CitationLink:
				break;
		}
	}
	if (!stk.empty()) throw std::runtime_error( strus::string_format( "structure open/close not balanced: %s...%s", Paragraph::typeName( stk.back()), ""));
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
		else if (para.type() == Paragraph::HeadingStart)
		{
			if (starttype == Paragraph::EntityStart) return;
			finishStructure( startidx);
		}
		else if (para.type() == Paragraph::ListItemStart)
		{
			if (starttype == Paragraph::EntityStart) return;
			finishStructure( startidx);
		}
		else if (para.type() == Paragraph::CitationStart)
		{
			if (starttype == Paragraph::EntityStart) return;
			finishStructure( startidx);
		}
		else if (para.type() == Paragraph::RefStart)
		{
			if (starttype == Paragraph::EntityStart) return;
			finishStructure( startidx);
		}
		else if (para.type() == Paragraph::TableStart)
		{
			if (starttype == Paragraph::TableRowStart || starttype == Paragraph::EntityStart) return;
			finishStructure( startidx);
		}
		else if (para.type() == Paragraph::TableTitleStart)
		{
			if (starttype == Paragraph::EntityStart) return;
			finishStructure( startidx);
		}
		else if (para.type() == Paragraph::TableHeadStart)
		{
			if (starttype == Paragraph::EntityStart) return;
			finishStructure( startidx);
		}
		else if (para.type() == Paragraph::TableRowStart)
		{
			if (starttype == Paragraph::EntityStart) return;
			finishStructure( startidx);
		}
		else if (para.type() == Paragraph::TableColStart)
		{
			if (starttype == Paragraph::EntityStart) return;
			finishStructure( startidx);
		}
		else
		{
			throw std::runtime_error("internal: unknown structure");
		}
	}
}

void DocumentStructure::addQuoteItem( Paragraph::Type startType)
{
	Paragraph::Type endType = Paragraph::invType( startType);
	if (!m_structStack.empty() && m_parar[ m_structStack.back().start].type() == startType)
	{
		m_parar[ m_structStack.back().start].setId( collectQuoteText( m_parar.begin() + m_structStack.back().start, m_parar.end()));
		m_parar.push_back( Paragraph( endType, "", ""));
	}
	else
	{
		m_parar.push_back( Paragraph( startType, "", ""));
	}
}

void DocumentStructure::openAutoCloseItem( Paragraph::Type startType, const char* prefix, int lidx)
{
	Paragraph::Type endType = Paragraph::invType( startType);
	while (!m_structStack.empty() && m_parar[ m_structStack.back().start].type() == startType && m_structStack.back().idx <= lidx)
	{
		Paragraph para = m_parar[ m_structStack.back().start];
		m_parar.push_back( Paragraph( endType, para.id(), para.text()));
		m_structStack.pop_back();
	}
	m_structStack.push_back( StructRef( lidx, m_parar.size()));
	m_parar.push_back( Paragraph( startType, strus::string_format("%s%d", prefix, lidx), ""));
}

void DocumentStructure::closeAutoCloseItem( Paragraph::Type startType)
{
	Paragraph::Type endType = Paragraph::invType( startType);
	if (m_structStack.empty()) return;
	Paragraph para = m_parar[ m_structStack.back().start];
	if (para.type() == startType)
	{
		m_parar.push_back( Paragraph( endType, para.id(), para.text()));
		m_structStack.pop_back();
	}
}

void DocumentStructure::openStructure( Paragraph::Type startType, const char* prefix, int lidx)
{
	if (startType == Paragraph::TableStart)
	{
		m_tableDefs.push_back( TableDef( m_parar.size()));
	}
	m_structStack.push_back( StructRef( 0, m_parar.size()));
	m_parar.push_back( Paragraph( startType, strus::string_format("%s%d", prefix, lidx), ""));
}

void DocumentStructure::finishStructure( int startidx)
{
	Paragraph para = m_parar[ startidx];
	Paragraph::Type endType = Paragraph::invType( para.type());
	m_parar.push_back( Paragraph( endType, para.id(), ""));
	if (endType == Paragraph::CitationEnd)
	{
		m_citations.insert( m_citations.end(), m_parar.begin() + startidx, m_parar.end());
		m_parar.resize( startidx);
		m_parar.push_back( Paragraph( Paragraph::CitationLink, para.id(), ""));
	}
	else if (endType == Paragraph::TableEnd)
	{
		if (!m_tableDefs.empty())
		{
			m_tableDefs.pop_back();
		}
		else
		{
			m_errors.push_back( strus::string_format( "close of table called without table definition"));
		}
	}
	else if (endType == Paragraph::TableRowEnd)
	{
		if (!m_tableDefs.empty())
		{
			m_tableDefs.back().coliter = 0;
		}
		else
		{
			m_errors.push_back( strus::string_format( "close of table row called without table definition"));
		}
	}
	m_structStack.pop_back();
}

void DocumentStructure::closeStructure( Paragraph::Type startType)
{
	closeDanglingStructures( startType);
	if (m_structStack.empty())
	{
		m_errors.push_back( strus::string_format( "close of %s structure called without open ()", Paragraph::structTypeName( Paragraph::structType( startType))));
		return;
	}
	int startidx = m_structStack.back().start;
	const Paragraph& para = m_parar[ startidx];
	if (para.type() == startType)
	{
		finishStructure( startidx);
	}
	else
	{
		const char* stnam = Paragraph::structTypeName( Paragraph::structType( para.type()));
		m_errors.push_back( strus::string_format( "close of %s structure called without open (%s)", Paragraph::structTypeName( Paragraph::structType( startType)), stnam));
		return;
	}
}

void DocumentStructure::closeOpenStructures()
{
	closeDanglingStructures( Paragraph::Title/*no match => remove all*/);
	m_parar.insert( m_parar.end(), m_citations.begin(), m_citations.end());
	m_citations.clear();
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
	if (!rt.empty() && rt[ rt.size()-1] == '_') rt.resize( rt.size()-1);
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

Paragraph::StructType DocumentStructure::currentStructType() const
{
	if (m_structStack.empty()) return Paragraph::StructNone;
	return m_parar[ m_structStack.back().start].structType();
}

void DocumentStructure::addSingleItem( Paragraph::Type type, const std::string& id, const std::string& text, bool joinText)
{
	if (!m_parar.empty())
	{
		Paragraph::Type tp = m_parar.back().type();
		if (tp == Paragraph::Text)
		{
			m_parar.back().addText( text);
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
	m_parar.push_back( Paragraph( type, id, text));
}

void DocumentStructure::finish()
{
	closeOpenStructures();
	checkStartEndSectionBalance( m_parar.begin(), m_parar.end());
}

std::string DocumentStructure::toxml() const
{
	std::string rt;
	std::vector<Paragraph::StructType> stk;
	XmlPrinter output;
	output.printHeader( rt);
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
			case Paragraph::EntityStart:
				stk.push_back( Paragraph::StructEntity);
				output.printOpenTag( "entity", rt);
				output.printAttribute( "id", rt);
				output.printValue( pi->text(), rt);
				output.switchToContent( rt);
				break;
			case Paragraph::EntityEnd:
				stk.pop_back();
				output.printCloseTag( rt);
				break;
			case Paragraph::HeadingStart:
				stk.push_back( Paragraph::StructHeading);
				output.printOpenTag( "heading", rt);
				output.printAttribute( "id", rt);
				output.printValue( pi->id(), rt);
				output.switchToContent( rt);
				break;
			case Paragraph::HeadingEnd:
				stk.pop_back();
				output.printCloseTag( rt);
				break;
			case Paragraph::ListItemStart:
				stk.push_back( Paragraph::StructList);
				output.printOpenTag( "list", rt);
				output.printAttribute( "id", rt);
				output.printValue( pi->id(), rt);
				output.switchToContent( rt);
				break;
			case Paragraph::ListItemEnd:
				stk.pop_back();
				output.printCloseTag( rt);
				break;
			case Paragraph::CitationStart:
				stk.push_back( Paragraph::StructCitation);
				output.printOpenTag( "citation", rt);
				output.printAttribute( "id", rt);
				output.printValue( pi->id(), rt);
				output.switchToContent( rt);
				break;
			case Paragraph::CitationEnd:
				stk.pop_back();
				output.printCloseTag( rt);
				break;
			case Paragraph::RefStart:
				stk.push_back( Paragraph::StructRef);
				output.printOpenTag( "ref", rt);
				output.switchToContent( rt);
				break;
			case Paragraph::RefEnd:
				stk.pop_back();
				output.printCloseTag( rt);
				break;
			case Paragraph::TableStart:
				stk.push_back( Paragraph::StructTable);
				output.printOpenTag( "table", rt);
				output.printAttribute( "id", rt);
				output.printValue( pi->id(), rt);
				output.switchToContent( rt);
				break;
			case Paragraph::TableEnd:
				stk.pop_back();
				output.printCloseTag( rt);
				break;
			case Paragraph::TableTitleStart:
				stk.push_back( Paragraph::StructTableTitle);
				output.printOpenTag( "heading", rt);
				output.printAttribute( "id", rt);
				output.printValue( pi->id(), rt);
				output.switchToContent( rt);
				break;
			case Paragraph::TableTitleEnd:
				stk.pop_back();
				output.printCloseTag( rt);
				break;
			case Paragraph::TableHeadStart:
				stk.push_back( Paragraph::StructTableHead);
				output.printOpenTag( "head", rt);
				output.printAttribute( "id", rt);
				output.printValue( pi->id(), rt);
				output.switchToContent( rt);
				break;
			case Paragraph::TableHeadEnd:
				stk.pop_back();
				output.printCloseTag( rt);
				break;
			case Paragraph::TableRowStart:
				stk.push_back( Paragraph::StructTableRow);
				output.printOpenTag( "row", rt);
				output.printAttribute( "id", rt);
				output.printValue( pi->id(), rt);
				output.switchToContent( rt);
				break;
			case Paragraph::TableRowEnd:
				stk.pop_back();
				output.printCloseTag( rt);
				break;
			case Paragraph::TableColStart:
				stk.push_back( Paragraph::StructTableCol);
				output.printOpenTag( "column", rt);
				output.printAttribute( "id", rt);
				output.printValue( pi->id(), rt);
				output.switchToContent( rt);
				break;
			case Paragraph::TableColEnd:
				stk.pop_back();
				output.printCloseTag( rt);
				break;
			case Paragraph::Attribute:
				output.printOpenTag( "attr", rt);
				output.printAttribute( "id", rt);
				output.printValue( pi->id(), rt);
				output.switchToContent( rt);
				output.printValue( pi->text(), rt);
				output.printCloseTag( rt);
				break;
			case Paragraph::Text:
				if (stk.empty())
				{
					output.printOpenTag( "text", rt);
					output.printValue( pi->text(), rt);
					output.printCloseTag( rt);
				}
				else
				{
					output.printValue( pi->text(), rt);
				}
				break;
			case Paragraph::NoWiki:
				if (stk.empty())
				{
					output.printOpenTag( "nowiki", rt);
					output.printValue( pi->text(), rt);
					output.printCloseTag( rt);
				}
				else
				{
					output.printValue( pi->text(), rt);
				}
				break;
			case Paragraph::Math:
				if (stk.empty())
				{
					output.printOpenTag( "math", rt);
					output.printValue( pi->text(), rt);
					output.printCloseTag( rt);
				}
				else
				{
					output.printValue( pi->text(), rt);
				}
				break;
			case Paragraph::PageLink:
				output.printOpenTag( "pagelink", rt);
				output.printAttribute( "id", rt);
				output.printValue( pi->id(), rt);
				output.switchToContent( rt);
				output.printValue( pi->text().empty() ? pi->id() : pi->text(), rt);
				output.printCloseTag( rt);
				break;
			case Paragraph::WebLink:
				output.printOpenTag( "wwwlink", rt);
				output.printAttribute( "href", rt);
				output.printValue( pi->id(), rt);
				output.switchToContent( rt);
				output.printValue( pi->text().empty() ? pi->id() : pi->text(), rt);
				output.printCloseTag( rt);
				break;
			case Paragraph::CitationLink:
				output.printOpenTag( "citlink", rt);
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
