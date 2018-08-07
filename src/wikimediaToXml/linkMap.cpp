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
#include "strus/base/string_format.hpp"
#include "strus/errorBufferInterface.hpp"
#include "strus/base/fileio.hpp"

#define _TXT(XX) XX

using namespace strus;

void LinkMap::init( const SymbolTable& symtab_, const std::map<int,int>& map_)
{
	if (!m_symtab.empty()) throw std::runtime_error( _TXT("call of init on non empty link map not allowed"));
	int ki = 0, ke = symtab_.size();
	for (; ki < ke; ++ki)
	{
		int keyidx = ki+1;
		const char* keystr = symtab_.key( keyidx);
		if (keyidx != (int)m_symtab.getOrCreate( keystr, std::strlen(keystr))) throw std::runtime_error( _TXT("corrupt data: bad index"));
	}
	m_map = map_;
}

void LinkMap::addLine( const std::string& ln)
{
	char const* mid = std::strchr( ln.c_str(), '\t');
	if (!mid) throw std::runtime_error( strus::string_format( _TXT("missing tab separator in linkmap file in line '%s'"), ln.c_str()));
	define( std::string( ln.c_str(), mid-ln.c_str()), std::string(mid+1));
}

void LinkMap::load( const std::string& filename)
{
	std::string content;
	int ec = strus::readFile( filename, content);
	if (ec) throw std::runtime_error( strus::string_format( _TXT("error reading link map file %s: %s"), filename.c_str(), ::strerror(ec)));
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
	std::map<int,int>::const_iterator mi = m_map.begin(), me = m_map.end();
	for (; mi != me; ++mi)
	{
		const char* key = m_symtab.key( mi->first);
		const char* val = m_symtab.key( mi->second);
		out << key << '\t' << val << "\n";
	}
	int ec = strus::writeFile( filename, out.str());
	if (ec) throw std::runtime_error( strus::string_format( _TXT("error writing link map file %s: %s"), filename.c_str(), ::strerror(ec)));
}

void LinkMap::define( const std::string& key, const std::string& value)
{
	int keyidx = m_symtab.getOrCreate( key);
	if (!keyidx) throw std::runtime_error( m_errorhnd->fetchError());
	int validx = m_symtab.getOrCreate( value);
	if (!validx) throw std::runtime_error( m_errorhnd->fetchError());
	m_map[ keyidx] = validx;
}

const char* LinkMap::get( const std::string& key) const
{
	int keyidx = m_symtab.get( normalizeKey( key));
	if (!keyidx) return 0;
	std::map<int,int>::const_iterator mi = m_map.find( keyidx);
	if (mi == m_map.end()) return 0;
	return m_symtab.key( mi->second);
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
	while (back == ' ')
	{
		rt.resize( rt.size()-1);
		back = rt.empty() ? '\0' : rt[ rt.size()-1];
	}
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
	int validx = m_symtab.getOrCreate( normval);
	if (!validx) throw std::runtime_error(_TXT("failed to create symbol"));
	m_idset.insert( validx);
	redirect( key, key);
}

void LinkMapBuilder::redirect( const std::string& key, const std::string& value)
{
	std::string normkey = LinkMap::normalizeKey( key);
	std::string normval = LinkMap::normalizeValue( value);
	std::string normvalkey = LinkMap::normalizeKey( value);
	int keyidx = m_symtab.getOrCreate( normkey);
	int validx = m_symtab.getOrCreate( normval);
	int valkeyidx = m_symtab.getOrCreate( normvalkey);
	LnkDef lnkdef( keyidx, validx, valkeyidx);
	m_lnkdefmap.insert( lnkdef);
}

const char* LinkMapBuilder::transitiveFindValue( int keyidx, int validx, int depth) const
{
	std::set<int>::const_iterator id = m_idset.find( validx);
	if (id != m_idset.end()) return m_symtab.key( validx);

	if (depth <= 0) return NULL;

	LnkDef lnkdef( keyidx, 0, 0);
	LnkDefSet::const_iterator li = m_lnkdefmap.upper_bound( lnkdef);
	LnkDefSet::const_iterator le = m_lnkdefmap.end();

	for (; li != le && li->key == lnkdef.key; ++li)
	{
		const char* rt = transitiveFindValue( li->valkey, li->val, depth-1);
		if (rt) return rt;
	}
	return NULL;
}

void LinkMapBuilder::build( LinkMap& res)
{
	LnkDefSet::const_iterator li = m_lnkdefmap.begin(), le = m_lnkdefmap.end();
	while (li != le)
	{
		LnkDefSet::const_iterator next_li = li;
		for (++next_li; next_li->key == li->key; ++next_li){}

		const char* keystr = m_symtab.key( li->key);
		for (; li != next_li; ++li)
		{
			const char* selectedval = transitiveFindValue( li->valkey, li->val, TransitiveSearchDepth);
			if (selectedval)
			{
				res.define( keystr, selectedval);
				break;
			}
		}
		if (li == next_li)
		{
			m_unresolved.insert( li->key);
		}
		li = next_li;
	}
}

std::vector<const char*> LinkMapBuilder::unresolved() const
{
	std::vector<const char*> rt;
	std::set<int>::const_iterator ui = m_unresolved.begin(), ue = m_unresolved.end();
	for (; ui != ue; ++ui)
	{
		const char* key = m_symtab.key( *ui);
		if (!key) throw std::runtime_error(_TXT("internal: corrupt key in unresolved list"));
		rt.push_back( key);
	}
	return rt;
}

