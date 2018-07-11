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
#include "strus/base/string_format.hpp"
#include "strus/base/fileio.hpp"
#include <string>
#include <map>
#include <set>
#include <vector>
#include <utility>
#include <cstring>
#include <sstream>
#include <iostream>

namespace strus {

class LinkMap
{
public:
	LinkMap( const LinkMap& o)
		:m_map(o.m_map){}
	LinkMap( const std::map<std::string,std::string>& map_)
		:m_map(map_){}
	LinkMap(){}

	void load( const std::string& filename);

	void write( const std::string& filename) const;

	const char* get( const std::string& key) const;

	static std::pair<std::string,std::string> getLinkParts( const std::string& linkid);

public:
	static std::string normalizeKey( const std::string& kk);
	static std::string normalizeValue( const std::string& vv);

private:
	void addLine( const std::string& ln);

private:
	std::map<std::string,std::string> m_map;
};


class LinkMapBuilder
{
public:
	LinkMapBuilder( const LinkMapBuilder& o)
		:m_map(o.m_map){}
	LinkMapBuilder( const std::multimap<std::string,std::string>& map_)
		:m_map(map_){}
	LinkMapBuilder(){}

	std::map<std::string,std::string> build();

	void redirect( const std::string& key, const std::string& value);
	void define( const std::string& key);

private:
	std::string transitiveValue( const std::vector<std::string>& valuelist, const std::set<std::string> idset) const;

private:
	std::multimap<std::string,std::string> m_map;
	std::set<std::string> m_idset;
	std::set<std::string> m_unresolved;
};

}//namespace
#endif
