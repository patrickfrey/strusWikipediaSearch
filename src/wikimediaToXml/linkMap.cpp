/*
* Copyright (c) 2018 Patrick P. Frey
*
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/
/// \brief Map for evaluating link identifiers, handling transitive redirects
/// \file linkMap.hpp
#include "linkMap.hpp"

using namespace strus;

void LinkMap::addLine( const std::string& ln)
{
	char const* mid = std::strchr( ln.c_str(), '\t');
	if (!mid) throw std::runtime_error( strus::string_format("missing tab separator in linkmap file in line '%s'", ln.c_str()));
	m_map[ std::string( ln.c_str(), mid-ln.c_str())] = std::string(mid+1);
}

void LinkMap::load( const std::string& filename)
{
	std::string content;
	int ec = strus::readFile( filename, content);
	if (ec) throw std::runtime_error( strus::string_format( "error reading link map file %s: %s", filename.c_str(), ::strerror(ec)));
	char const* li = content.c_str();
	char const* ln = std::strchr( li, '\n');
	for (; ln; li=ln+1,ln = std::strchr( li, '\n'))
	{
		if (ln-li>0) addLine( std::string( li, ln-li));
	}
	if (*li) addLine( li);
}

void LinkMap::write( const std::string& filename) const
{
	std::ostringstream out;
	std::map<std::string,std::string>::const_iterator mi = m_map.begin(), me = m_map.end();
	for (; mi != me; ++mi)
	{
		out << mi->first << '\t' << mi->second << "\n";
	}
	int ec = strus::writeFile( filename, out.str());
	if (ec) throw std::runtime_error( strus::string_format( "error writing link map file %s: %s", filename.c_str(), ::strerror(ec)));
}

const char* LinkMap::get( const std::string& key) const
{
	std::string normkey = normalizeKey( key);
	std::map<std::string,std::string>::const_iterator mi = m_map.find( normkey);
	if (mi == m_map.end()) return 0;
	return mi->second.c_str();
}

std::pair<std::string,std::string> LinkMap::getLinkParts( const std::string& linkid)
{
	char const* mid = std::strchr( linkid.c_str(), '#');
	if (mid)
	{
		return std::pair<std::string,std::string>( std::string( linkid.c_str(), mid-linkid.c_str()), mid+1);
	}
	else
	{
		return std::pair<std::string,std::string>( linkid, "");
	}
}

std::string LinkMap::normalizeValue( const std::string& vv)
{
	std::string rt;
	char const* vi = vv.c_str();

	for (; *vi; ++vi)
	{
		char back = rt.empty() ? ' ' : rt[ rt.size()-1];
		if ((unsigned char)*vi <= 32)
		{
			if (back != ' ') rt.push_back(' ');
		}
		else
		{
			rt.push_back( *vi);
		}
	}
	char back = rt.empty() ? '\0' : rt[ rt.size()-1];
	if (back == ' ') rt.resize( rt.size()-1);
	return rt;
}

std::string LinkMap::normalizeKey( const std::string& kk)
{
	std::string rt;
	char const* ki = kk.c_str();

	for (; *ki; ++ki)
	{
		char ch = *ki | 32;
		char back = rt.empty() ? ' ' : rt[ rt.size()-1];
		if (*ki == '(' || *ki == '-')
		{
			if (back != ' ') rt.push_back(' ');
			rt.push_back( *ki);
		}
		else if ((unsigned char)*ki <= 32)
		{
			if (back != ' ') rt.push_back(' ');
		}
		else if (ch >= 'a' && ch <= 'z')
		{
			if (back == ')' || back == '.' || back == ':' || back == '!' || back == ';' || back == '?')
			{
				rt.push_back( ' ');
				rt.push_back( ch ^ 32);
			}
			else if (back == '(')
			{
				rt.push_back( ch ^ 32);
			}
			else if (back == ' ')
			{
				rt.push_back( ch ^ 32);
			}
			else
			{
				rt.push_back( ch);
			}
		}
		else
		{
			rt.push_back( *ki);
		}
	}
	char back = rt.empty() ? '\0' : rt[ rt.size()-1];
	if (back == ' ') rt.resize( rt.size()-1);
	return rt;
}

std::map<std::string,std::string> LinkMapBuilder::build()
{
	std::map<std::string,std::string> rt;
	std::set<std::string> idset;
	std::multimap<std::string,std::string>::const_iterator mi = m_map.begin(), me = m_map.end();
	while (mi != me)
	{
		const std::string key = mi->first;
		std::vector<std::string> valuelist;
		for (; mi != me && key == mi->first; ++mi)
		{
			valuelist.push_back( mi->second);
		}
		std::string val = transitiveValue( valuelist, m_idset);
		if (val.empty())
		{
			m_unresolved.insert( key);
		}
		else
		{
			rt[ key] = val;
		}
	}
	return rt;
}

void LinkMapBuilder::define( const std::string& key)
{
	std::string normval = LinkMap::normalizeValue( key);
	m_idset.insert( normval);
	redirect( key, key);
}

void LinkMapBuilder::redirect( const std::string& key, const std::string& value)
{
	std::string normkey = LinkMap::normalizeKey( key);
	std::string normval = LinkMap::normalizeValue( value);
	typedef std::multimap<std::string,std::string>::const_iterator insert_iterator;
	std::pair<insert_iterator,insert_iterator> irange = m_map.equal_range( normkey);
	std::map<std::string,std::string>::const_iterator mi = irange.first, me = irange.second;
	for (; mi != me && mi->second != normval; ++mi){}
	if (mi == me)
	{
		m_map.insert( std::pair<std::string,std::string>( normkey, normval));
	}
}

std::string LinkMapBuilder::transitiveValue( const std::vector<std::string>& valuelist, const std::set<std::string> idset) const
{
	std::set<std::string> vset( valuelist.begin(), valuelist.end());
	std::vector<std::string> vl = valuelist;
	int vidx = 0;
	for (; vidx < (int)vl.size(); ++vidx)
	{
		if (idset.find( vl[vidx]) != idset.end()) return vl[ vidx];
		std::string normkey = LinkMap::normalizeKey( vl[vidx]);

		typedef std::multimap<std::string,std::string>::const_iterator insert_iterator;
		std::pair<insert_iterator,insert_iterator> irange = m_map.equal_range( normkey);
		std::map<std::string,std::string>::const_iterator mi = irange.first, me = irange.second;
		for (; mi != me; ++mi)
		{
			if (vset.find( mi->second) == vset.end())
			{
				if (idset.find( mi->second) != idset.end())
				{
					return mi->second;
				}
				else
				{
					vl.push_back( mi->second);
					vset.insert( mi->second);
				}
			}
		}
	}
	return std::string();
}

