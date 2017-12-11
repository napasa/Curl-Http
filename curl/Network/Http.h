#pragma once
#include <memory>
#include <list>
#include <algorithm>
#include <atomic>
#include "URL.h"
#include <curl/curl.h>

namespace Network
{

	/*Memory Used For Storing Response Content*/
	class Memory
	{
	public:
		Memory(); 
		/*deep copy*/
		Memory(const Memory &memory);
		~Memory();
		char *m_memory;
		size_t m_size;
	};

	/*HTTP Request*/
	class Request {
	public:
		Request() = delete;
		Request(const std::string &url);
		Request(const Request &request);
		const std::string &GetUrl()const;
		void SetUnhandled(bool handled);
		bool isUnhandled()const;
		~Request() {  }
	private:
		std::string m_url;
		bool m_unhandled;
	};

	/*HTTP Response*/
	class Response{
	public:
		Response();
		Response(const Response &response);
		~Response(){}
		void SetResult(CURLcode code);
		CURLcode GetResult()const;
		const Memory &GetMemory()const;
		Memory &GetMemory();
		void SetCURL(CURL *curl);
		CURL *GetCURL()const;
	private:
		Memory m_memory;
		CURLcode m_code;
		CURL *m_curl;
	};

	/*HTTP Action For Response From Server, overload Do to perform action to response*/
	class HttpAction{
	public:
		HttpAction(){}
		virtual void Do(const Network::Response  &response) = 0;
		virtual int Progress(double totaltime, double dltotal, double dlnow, double ultotal, double ulnow) = 0;
		~HttpAction(){}

	};

	class Http
	{
	public:
		static const Http & GetInstance();
		void Get(const std::string &url, HttpAction *httpAction)const;
		void StopExcutor()const;
		~Http() {}
		Http(const Http &http)=delete;
		Http& operator=(const Http&)=delete;
	private:
		Http(){};
	};

}
