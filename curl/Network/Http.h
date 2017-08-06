#pragma once
#include <memory>
#include <list>
#include <algorithm>
#include <atomic>
#include "URL.h"
#include "..\..\include\curl\curl.h"

namespace Network
{
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

	/*HTTP请求体*/
	class Request {
	public:
		Request() = delete;
		Request(const std::string &url) :m_url(url), m_pendingProcess(true) {
		}
		Request(const Request &request) :m_url(request.m_url), m_pendingProcess(request.m_pendingProcess) {

		}
		const std::string &GetUrl()const {
			return m_url;
		}
		void SetPendingProcess(bool pending) {
			m_pendingProcess = pending;
		}
		bool isPendingProcess()const {
			return m_pendingProcess;
		}
		~Request() {  }
	private:
		std::string m_url;
		bool m_pendingProcess;
	};

	/*HTTP请求结果*/
	class Response{
	public:
		Response():m_code(CURLE_OK),m_memory(){}
		Response(const Response &response):m_code(response.m_code), m_memory(response.m_memory) {
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
	private:
		Memory m_memory;
		CURLcode m_code;
	};

	class HttpAction{
	public:
		HttpAction(){}
		virtual void Do(std::shared_ptr<Network::Response> response /*Network::Response *response*/){
		}
		virtual int Progress(double totaltime, double dltotal, double dlnow, double ultotal, double ulnow) { return 1; }

		~HttpAction(){}

	};


	class GetWork {
	public:
		GetWork(const std::string &url, std::shared_ptr<HttpAction> action) :m_request(url), m_response(),m_httpAction(action){

		}
		GetWork(const GetWork &getwork):m_request(getwork.m_request), m_response(getwork.m_response), m_httpAction(getwork.m_httpAction) {
		}
		const Request &GetRequest()const {
			return m_request;
		}
		Request &GetRequest() {
			return m_request;
		}
		const Response &GetResponse()const {
			return m_response;
		}
		Response &GetResponse() {
			return m_response;
		}

		std::shared_ptr<HttpAction> GetHttpAction() {
			return m_httpAction;
		}
	private:
		Request m_request;
		Response m_response;
		std::shared_ptr<HttpAction> m_httpAction;
	};
	class GetWorkQueue{
	public:
		GetWork &FrontPendingProcessGetWork(){
			for (GetWork &httpGetWork : m_getWorkQueue)
			{
				if (httpGetWork.GetRequest().isPendingProcess())
					return httpGetWork;
			}
		}
		bool HasPendingProcessGetWork(){
			for (GetWork &httpGetWork : m_getWorkQueue)
			{
				if (httpGetWork.GetRequest().isPendingProcess())
					return true;
			}
			return false;
		}
		void Push(const GetWork &getwork){
			m_getWorkQueue.push_back(getwork);
		}
		void PopProcessedRequest(){
			m_getWorkQueue.remove_if([](const GetWork &getwork){
				return !getwork.GetRequest().isPendingProcess();
			});
		}
		bool empty(){
			return m_getWorkQueue.empty();
		}
	private:
		std::list<GetWork> m_getWorkQueue;
	};








	typedef int Id;
	typedef struct tagMemoryStruct
	{
		char *memory;
		size_t size;
		bool status;
		URL url;
		Id id;
	} MemoryStruct;

	typedef int Identifier;

	//获取到HTTP的response后的action
	class AbstractAction{
	public:
		AbstractAction(){};
		virtual void Do(const MemoryStruct *memory) = 0;
		virtual int Progress(double totaltime, double dltotal, double dlnow, double ultotal, double ulnow) { return 1; }
		virtual ~AbstractAction(){}
	};

	class Http
	{
	public:
		static Http* GetInstance();

	public:
		void Get(const std::string &url, std::shared_ptr<HttpAction> httpAction);
		void Get(const URL &url, std::shared_ptr<AbstractAction> action, Id id, bool async=true);
		void Post(const URL &url, std::shared_ptr<AbstractAction> action, Id id, const std::string &postedFilename, bool async=true);
		~Http();
		Http(const Http &http)=delete;
		Http& operator=(const Http&)=delete;
	private:
		Http(){};
		static size_t WriteMemoryCallback(void *contents, size_t size, size_t nmemb, void *userp);

		/* this is how the CURLOPT_XFERINFOFUNCTION callback works */
		static int xferinfo(void *p, curl_off_t dltotal, curl_off_t dlnow, curl_off_t ultotal, curl_off_t ulnow);

		static 	int older_progress(void *p, double dltotal, double dlnow, double ultotal, double ulnow);
		
		static void GetRun(const URL &url, std::shared_ptr<AbstractAction> action, Id id);

		static void PostRun(const URL &url, std::shared_ptr<AbstractAction> action, Id id, const std::string &postedFilename);

		static Http* instance;
	};

}
