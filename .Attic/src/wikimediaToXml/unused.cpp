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


