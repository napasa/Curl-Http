#pragma once
#include <string>
#include <map>
#include <vector>

namespace Network
{

	class URL
	{
	public:
		typedef std::string AttribKey;
		typedef std::string AttribValue;
		typedef std::map<AttribKey, AttribValue> _AttribMap;
		class AttribMap :public _AttribMap
		{
		public:
			AttribMap(std::initializer_list<value_type> init)
				:_AttribMap(init){}
			AttribValue& operator[](const AttribKey& key){
				return _AttribMap::operator[](key);
			}
			AttribMap(){}
			const std::string ToString()const{
				std::string _attribs;
				size_type increment = 0;
				for (auto const &attribPair : *this)
				{
					_attribs += (attribPair.first + "=" + attribPair.second);
#define LAST_ELEMENT_INDEX (size()-1)
					if (increment++ < LAST_ELEMENT_INDEX) _attribs.append("&");
				}
				return _attribs;
			}
		};

	public:
		URL(const std::string &host, const std::string &path, const AttribMap &queryString);
		URL(){};
		URL(const char *url);
		URL(const std::string &url);
		const std::string & getUrl()const;
		const AttribMap &GetAttribMap()const{
			return _queryString;
		}
		virtual ~URL();
	private:
		void formatUrl();
	private:
		std::string _scheme;
		std::string _host;
		std::string _path;
		AttribMap _queryString;
		std::string _url;
	};

}
