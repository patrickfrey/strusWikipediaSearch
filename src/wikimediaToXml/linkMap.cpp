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

void LinkMapBuilder::define( const std::string& key)
{
	std::string normval = LinkMap::normalizeValue( key);
	m_idset.insert( normval);
	redirect( key, key);
}

void LinkMapBuilder::redirect( const std::string& key, const std::string& value)
{
	LnkDef lnkdef( LinkMap::normalizeKey( key), LinkMap::normalizeValue( value));
	m_lnkdefmap.insert( lnkdef);
}

const std::string* LinkMapBuilder::transitiveFindValue( const std::string& value, int depth) const
{
	std::set<std::string>::const_iterator id = m_idset.find( value);
	if (id != m_idset.end()) return &*id;

	if (depth == 0) return NULL;

	LnkDef lnkdef( LinkMap::normalizeKey( value), "");
	LnkDefSet::const_iterator li = m_lnkdefmap.upper_bound( lnkdef);
	LnkDefSet::const_iterator le = m_lnkdefmap.end();

	for (; li != le && li->key == lnkdef.key; ++li)
	{
		const std::string* rt = transitiveFindValue( li->val, depth-1);
		if (rt) return rt;
	}
	return NULL;
}

std::map<std::string,std::string> LinkMapBuilder::build()
{
	std::map<std::string,std::string> rt;
	LnkDefSet::const_iterator li = m_lnkdefmap.begin(), le = m_lnkdefmap.end();
	while (li != le)
	{
		const std::string* selectedval = transitiveFindValue( li->val, TransitiveSearchDepth);
		if (!selectedval)
		{
			const LnkDef& lnkdef = *li;
			++li;
			if (li->key != lnkdef.key)
			{
				m_unresolved.insert( li->key);
			}
		}
		else
		{
			rt[ li->key] = *selectedval;
			const LnkDef& lnkdef = *li;
			// Skip to the next key
			for (++li; li != le && lnkdef.key == li->key; ++li){}
		}
	}
	return rt;
}


