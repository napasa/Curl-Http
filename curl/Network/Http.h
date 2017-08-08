#pragma once
#include <memory>
#include <list>
#include <algorithm>
#include <atomic>
#include "URL.h"
#include <curl\curl.h>

namespace Network
{
	class Memory
	{
	public:
		Memory(){
			m_memory = (char *)malloc(1);  /* will be grown as needed by the realloc above */
			m_size = 0;    /* no data at this point */
		}
		char *m_memory;
		size_t m_size;
	};
	class Response{
	public:
		Response(){}

		~Response(){
			OutputDebugStringA("Response Released");
		}
		void SetResult(CURLcode code){
			m_code = code;
		}
		CURLcode GetResult(){
			return m_code;
		}

		Memory *GetMemory(){
			return m_memory;
		}
		void SetMemory(Memory *memory){
			m_memory = memory;
		}
	private:
		Memory *m_memory;
		CURLcode m_code;
	};
	class HttpAction;
	class Request{
	public:
		Request(const std::string &url, HttpAction *action) :m_pendingProcess(true){
			m_url = url;
			m_action = action;
		}
		const std::string &GetUrl(){
			return m_url;
		}
		HttpAction *GetAction(){
			return m_action;
		}
		void SetPendingProcess(bool pending){
			m_pendingProcess = pending;
		}
		bool isPendingProcess(){
			return m_pendingProcess;
		}
		Memory &GetMemory(){
			return m_memory;
		}
		~Request(){  }
	private:
		std::string m_url;
		HttpAction *m_action;
		bool m_pendingProcess;
		Memory m_memory;
	};
	class HttpAction{
	public:
		HttpAction() :m_response(nullptr){}
		virtual void Do(Network::Response *response){
			m_response = response;
		}
		virtual int Progress(double totaltime, double dltotal, double dlnow, double ultotal, double ulnow) { return 1; }

		~HttpAction(){
			;//ReleaseResponse();
		}
	private:
		void ReleaseResponse(){
			if (nullptr != m_response)
			{
				delete m_response;
			}
			m_response = nullptr;
		}
		Network::Response *m_response;
	};

	class RequestQueue{
	public:
		Request &FrontPendingProcessRequest(){
			for (Request &request : m_requestQueue)
			{
				if (request.isPendingProcess())
					return request;
			}
		}
		bool HasPendingProcessRequest(){
			for (Request &request : m_requestQueue)
			{
				if (request.isPendingProcess())
					return true;
			}
			return false;
		}
		void Push(Request request){
			m_requestQueue.push_back(request);
		}
		void PopProcessedRequest(){
			m_requestQueue.remove_if([](Request request){
				return !request.isPendingProcess();
			});
		}
		bool empty(){
			return m_requestQueue.empty();
		}
	private:
		std::list<Request> m_requestQueue;
	};
	extern RequestQueue g_requestQueue;








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
		virtual int Progress(double totaltime, double dltotal, double dlnow, double ultotal, double ulnow) { return 0; }
		virtual ~AbstractAction(){}
	};

	class Http
	{
	public:
		static Http* GetInstance();

		typedef void(*FinishedCallback)(MemoryStruct *memory);

	public:
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
