#pragma once
#include <memory>
#include <list>
#include <algorithm>
#include <atomic>
#include "URL.h"
#include "..\..\include\curl\curl.h"

namespace Network
{
	/*Memory Used For Storing Response Content*/
	class Memory
	{
	public:
		Memory(){
			m_size = 0;    /* no data at this point */
			m_memory = (char *)malloc(1);  /* will be grown as needed by the realloc above */
		} 
		/*deep copy*/
		Memory(const Memory &memory) {
			m_size = memory.m_size;
			m_memory =(char *) malloc(memory.m_size);
			memcpy(m_memory, memory.m_memory, memory.m_size);
		}

		~Memory() {
			delete m_memory;
			m_memory = 0;
		}
		char *m_memory;
		size_t m_size;
	};

	/*HTTP Request*/
	class Request {
	public:
		Request() = delete;
		Request(const std::string &url) :m_url(url), m_handled(true) {
		}
		Request(const Request &request) :m_url(request.m_url), m_handled(request.m_handled) {

		}
		const std::string &GetUrl()const {
			return m_url;
		}
		void SetHandledStatus(bool handled) {
			m_handled = handled;
		}
		bool isUnhandled()const {
			return m_handled;
		}
		~Request() {  }
	private:
		std::string m_url;
		bool m_handled;
	};

	/*HTTP Response*/
	class Response{
	public:
		Response():m_code(CURLE_OK),m_curl(nullptr),m_memory(){}
		Response(const Response &response):m_code(response.m_code), m_curl(response.m_curl), m_memory(response.m_memory) {
		}
		~Response(){}
		void SetResult(CURLcode code){
			m_code = code;
		}
		CURLcode GetResult()const{
			return m_code;
		}

		const Memory &GetMemory()const{
			return m_memory;
		}
		Memory &GetMemory(){
			return m_memory;
		}
		void SetCURL(CURL *curl) {
			m_curl = curl;
		}
		CURL *GetCURL()const {
			return m_curl;
		}
	private:
		Memory m_memory;
		CURLcode m_code;
		CURL *m_curl;
	};

	/*HTTP Action For Response From Server, overload Do to perform action to response*/
	class HttpAction{
	public:
		HttpAction(){}
		virtual void Do(std::shared_ptr<Network::Response> response) = 0;
		virtual int Progress(double totaltime, double dltotal, double dlnow, double ultotal, double ulnow) = 0;
		~HttpAction(){}
	};

	class Http
	{
	public:
		static const Http & GetInstance();
		void Get(const std::string &url, std::shared_ptr<HttpAction> httpAction)const;
		void StopExcutor()const;

		~Http() {}
		Http(const Http &http)=delete;
		Http& operator=(const Http&)=delete;
	private:
		Http(){};
	};

}
