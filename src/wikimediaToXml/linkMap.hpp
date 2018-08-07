/*
 * Copyright (c) 2018 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

/// \brief Map for evaluating link identifiers, handling transitive redirects
/// \file linkMap.hpp
#ifndef _STRUS_WIKIPEDIA_LINK_MAP_HPP_INCLUDED
#define _STRUS_WIKIPEDIA_LINK_MAP_HPP_INCLUDED
#include "strus/base/symbolTable.hpp"
#include <string>
#include <map>
#include <set>
#include <vector>
#include <utility>
#include <cstring>
#include <sstream>
#include <iostream>

namespace strus {

/// \brief Forward declaration
class ErrorBufferInterface;

class LinkMap
{
public:
	
	explicit LinkMap( ErrorBufferInterface* errorhnd_)
		:m_errorhnd(errorhnd_),m_symtab(errorhnd_),m_map(){}

	void init( const SymbolTable& symtab_, const std::map<int,int>& map_);
	void load( const std::string& filename);

	void write( const std::string& filename) const;

	void define( const std::string& key, const std::string& value);
	const char* get( const std::string& key) const;

public:
	static std::string normalizeKey( const std::string& kk);
	static std::string normalizeValue( const std::string& vv);
	static std::pair<std::string,std::string> getLinkParts( const std::string& linkid);

private:
	void addLine( const std::string& ln);

private:
	ErrorBufferInterface* m_errorhnd;
	SymbolTable m_symtab;
	std::map<int,int> m_map;
};


class LinkMapBuilder
{
public:
	explicit LinkMapBuilder( ErrorBufferInterface* errorhnd_)
		:m_errorhnd(errorhnd_),m_symtab(errorhnd_),m_lnkdefmap(),m_idset(),m_unresolved(){}

	void build( LinkMap& res);

	std::vector<const char*> unresolved() const;

	void redirect( const std::string& key, const std::string& value);
	void define( const std::string& key);

private:
	enum {TransitiveSearchDepth=6};
	const char* transitiveFindValue( int keyidx, int validx, int depth) const;

private:
	struct LnkDef
	{
		int key;
		int val;
		int valkey;

		LnkDef()
			:key(0),val(0),valkey(0){}
		LnkDef( int key_, int val_, int valkey_)
			:key(key_),val(val_),valkey(valkey_){}
		LnkDef( const LnkDef& o)
			:key(o.key),val(o.val),valkey(o.valkey){}

		bool operator < ( const LnkDef& o) const
		{
			if (key < o.key) return true;
			if (key > o.key) return false;
			if (val < o.val) return true;
			if (val > o.val) return false;
			return valkey < o.valkey;
		}
	};
	ErrorBufferInterface* m_errorhnd;
	SymbolTable m_symtab;
	typedef std::set<LnkDef> LnkDefSet;

	LnkDefSet m_lnkdefmap;
	std::set<int> m_idset;
	std::set<int> m_unresolved;
};

}//namespace
#endif
