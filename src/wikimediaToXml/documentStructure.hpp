/*
 * Copyright (c) 2018 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

/// \brief Intermediate document format for converting "Wikimedia XML" to pure and simplified XML
/// \file documentStructure.hpp
#ifndef _STRUS_WIKIPEDIA_DOCUMENT_STRUCTURE_HPP_INCLUDED
#define _STRUS_WIKIPEDIA_DOCUMENT_STRUCTURE_HPP_INCLUDED
#include <string>
#include <map>
#include <vector>
#include <utility>

/// \brief strus toplevel namespace
namespace strus {

class Paragraph
{
public:
	enum Type {Title,Heading,TableStart,TableEnd,ListItemStart,ListItemEnd,CitationStart,CitationEnd,TableRowStart,TableRowEnd,TableCol,Text,PageLink,WebLink,CitationLink};
	static const char* typeName( Type tp)
	{
		static const char* ar[] = {"Title","Heading","TableStart","TableEnd","ListItemStart","ListItemEnd","CitationStart","CitationEnd","TableRowStart","TableRowEnd","TableCol","Text","PageLink","WebLink","CitationLink","Redirect",0};
		return ar[tp];
	}
	const char* typeName() const
	{
		return typeName( m_type);
	}

	typedef std::pair<std::string,std::string> Attribute;

	Paragraph()
		:m_type(Text),m_id(),m_text(),m_attributes(){}
	Paragraph( Type type_, const std::string& id_, const std::string& text_, const std::vector<Attribute>& attributes_=std::vector<Attribute>())
		:m_type(type_),m_id(id_),m_text(text_),m_attributes(attributes_){}
	Paragraph( const Paragraph& o)
		:m_type(o.m_type),m_id(o.m_id),m_text(o.m_text),m_attributes(o.m_attributes){}

	Type type() const					{return m_type;}
	const std::string& id() const				{return m_id;}
	const std::string& text() const				{return m_text;}
	const std::vector<Attribute>& attributes() const	{return m_attributes;}

	void addAttribute( const std::string& id_, const std::string& value_)
	{
		m_attributes.push_back( Attribute( id_, value_));
	}

private:
	Type m_type;
	std::string m_id;
	std::string m_text;
	std::vector<Attribute> m_attributes;
};


class DocumentStructure
{
public:
	explicit DocumentStructure()
		:m_id(),m_parar(),m_citations(),m_structStack(),m_tableDefs(),m_errors(),m_citationCnt(0){}
	DocumentStructure( const DocumentStructure& o)
		:m_id(o.m_id),m_parar(o.m_parar),m_citations(o.m_citations),m_structStack(o.m_structStack),m_tableDefs(o.m_tableDefs),m_errors(o.m_errors),m_citationCnt(o.m_citationCnt){}

	const std::string& id() const
	{
		return m_id;
	}

	void setTitle( const std::string& text);
	void addAltTitle( const std::string& text);
	void addHeading( int idx, const std::string& text);
	void addText( const std::string& text);
	void addPageLink( const std::string& id, const std::string& text);
	void addWebLink( const std::string& id, const std::string& text);
	void openTable( const std::string& title, const std::vector<std::string>& coltitles);
	void addTableRow();
	void addTableCol( const std::string& text);
	void closeTable();
	void openListItem( int lidx);
	void closeOpenListItem();
	void openCitation();
	void closeCitation();

	void addError( const std::string& msg)
	{
		m_errors.push_back( msg);
	}

	void addAttribute( const std::string& id, const std::string& text);
	const std::vector<std::string>& errors() const
	{
		return m_errors;
	}
	void finish();

	std::string toxml( bool subDocument=false) const;
	std::string tostring() const;

private:
	void closeOpenCitation();
	void closeOpenStructures();
	void closeDanglingStructures( const Paragraph::Type& starttype);
	void closeOpenTableRow();
	void checkStartEndSectionBalance( const std::vector<Paragraph>::const_iterator& start, const std::vector<Paragraph>::const_iterator& end);

private:
	struct StructRef
	{
		int idx;
		int start;

		StructRef( int idx_, int start_)
			:idx(idx_),start(start_){}
		StructRef( const StructRef& o)
			:idx(o.idx),start(o.start){}
	};
	struct TableDef
	{
		std::vector<std::string> coltitles;
		int rowiter;
		int coliter;

		explicit TableDef( const std::vector<std::string>& coltitles_, int coliter_=0)
			:coltitles(coltitles_),rowiter(0),coliter(coliter_){}
		TableDef( const TableDef& o)
			:coltitles(o.coltitles),rowiter(o.rowiter),coliter(o.coliter){}
	};

private:
	std::string m_id;
	std::vector<Paragraph> m_parar;
	std::vector<Paragraph> m_citations;
	std::vector<StructRef> m_structStack;
	std::vector<TableDef> m_tableDefs;
	std::vector<std::string> m_errors;
	int m_citationCnt;
};

}//namespace
#endif

