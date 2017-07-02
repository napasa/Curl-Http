#include "stdafx.h"
#include <sstream>
#include <random>
#include <array>
#include "URL.h"

namespace Network
{
	URL::URL(const std::string &host, const std::string &path, const AttribMap &queryString) :_scheme("http"), _host(host), _path(path), _queryString(queryString)
	{
		formatUrl();
	}


	URL::URL(const char *url)
		:_url(url)
	{
	}


	URL::URL(const std::string &url)
		:_url(url)
	{

	}

	const std::string & URL::getUrl() const
	{
		return _url;
	}


	void URL::formatUrl()
	{
		_url.reserve((_host.length() + _path.length()) * 5);
		_url = _scheme + "://" + _host + _path + "?";


		for (auto const &attribPair : _queryString)
		{
			_url += (attribPair.first + "=" + attribPair.second);
			_url.append("&");
		}
		_url[_url.size() - 1] = '\0';
	}

	URL::~URL()
	{
	}

}