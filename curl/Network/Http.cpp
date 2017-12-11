#include "stdafx.h"
#include <thread>
#include <iostream>
#include <assert.h> 
#include <mutex>
#include <chrono>
#include "Http.h"

namespace Network
{
	const Http &  Http::GetInstance(){
			static Http instance;
		return instance;
	}

	static int i = 0;
	/*HTTP Get Task. You Used HTTP::Get Method a time Which created a GetTask, System will push your Task into GetTaskQueue*/
	class GetTask {
	public:
		GetTask():m_request("error"), m_httpAction(0){}
		GetTask(const std::string &url, HttpAction *action) :m_request(url), m_response(), m_httpAction(action), m_mark(GetTask::st_markCouter++) {}
		GetTask(const GetTask &gettask) :m_request(gettask.m_request), m_response(gettask.m_response), m_httpAction(gettask.m_httpAction), m_mark(gettask.m_mark) {
		}
		const Request &GetRequest()const {
			return m_request;
		}
		Request &GetRequest() {
			return m_request;
		}
		bool IsUnhandled() {
			return m_request.isUnhandled();
		}
		void SetUnhandled(bool unhandled = false) {
			m_request.SetUnhandled(unhandled);
		}

		const Response &GetResponse()const {
			return m_response;
		}
		Response &GetResponse() {
			return m_response;
		}
		long long GetMark()const { return m_mark; }
		HttpAction *GetHttpAction() {
			return m_httpAction;
		}
		~GetTask() { }
	private:
		Request m_request;
		Response m_response;
		HttpAction *m_httpAction;
		long long m_mark;
		static long long st_markCouter;
		
	};
	long long GetTask::st_markCouter = 0;

	/*GetTaskQueue maintains a task queue to perform task orderly*/
	class GetTaskQueue {
	public:
		GetTask &FrontUnhandledTask() {
			for (GetTask &httpGetTask : m_getTaskQueue)
			{
				if (httpGetTask.IsUnhandled())
					return httpGetTask;
			}
			static GetTask empty_task;
			return empty_task;
		}
		bool HasUnhandledTask() {
			for (GetTask &httpGetTask : m_getTaskQueue)
			{
				if (httpGetTask.IsUnhandled())
					return true;
			}
			return false;
		}

		void Push(const GetTask &gettask) {
			m_getTaskQueue.push_back(gettask);
		}
		
		void PopProcessedTask(long long mark) {
			m_getTaskQueue.remove_if([mark](const GetTask &getwork) {
				return mark == getwork.GetMark();
			});
		}
		bool empty() {
			return m_getTaskQueue.empty();
		}
	private:
		std::list<GetTask> m_getTaskQueue;
	};


	GetTaskQueue g_getTaskQueue;
	/*atomic variable determines TaskExcuter Thread Living Status*/

	static size_t WriteMemoryCallback(void *contents, size_t size, size_t nmemb, void *userp)
	{
		size_t realsize = size * nmemb;
		Network::Memory *l_memory = (Network::Memory *)userp;

		l_memory->m_memory = (char *)realloc(l_memory->m_memory, l_memory->m_size + realsize + 1);
		if (l_memory->m_memory == NULL) 
			return 0;

		memcpy(&(l_memory->m_memory[l_memory->m_size]), contents, realsize);
		l_memory->m_size += realsize;
		l_memory->m_memory[l_memory->m_size] = 0;

		return realsize;
	}

	static int xferinfo(void *p, curl_off_t dltotal, curl_off_t dlnow, curl_off_t ultotal, curl_off_t ulnow)
	{
		double curtime = 0;
		GetTask *task = (GetTask * )p;
		curl_easy_getinfo(task->GetResponse().GetCURL(), CURLINFO_TOTAL_TIME, &curtime);
		return task->GetHttpAction()->Progress(curtime, (double)dltotal, (double)dlnow, 0, 0);
	}

	static int older_progress(void *p, double dltotal, double dlnow, double ultotal, double ulnow)
	{return xferinfo(p, (curl_off_t)dltotal, (curl_off_t)dlnow, (curl_off_t)ultotal, (curl_off_t)ulnow);}

#ifdef _DEBUG
	/*Test Whether All Request Will be Excuted*/
	std::vector<std::string> _input_requests;
	std::vector<std::string> _excuted_requests;
#endif
	static void init(CURLM *cm)
	{
		CURL *eh = curl_easy_init();
		GetTask& unhandledTask = g_getTaskQueue.FrontUnhandledTask();
		unhandledTask.GetResponse().SetCURL(eh);
		Network::Memory &l_memory = unhandledTask.GetResponse().GetMemory();
		//set easy handle option
		curl_easy_setopt(eh, CURLOPT_PRIVATE, (void *)&unhandledTask);
		curl_easy_setopt(eh, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);
		curl_easy_setopt(eh, CURLOPT_WRITEDATA, (void *)&l_memory);
		curl_easy_setopt(eh, CURLOPT_VERBOSE, 0L);
		curl_easy_setopt(eh, CURLOPT_NOPROGRESS, 0);
#if LIBCURL_VERSION_NUM >= 0x072000
		curl_easy_setopt(eh, CURLOPT_XFERINFOFUNCTION, xferinfo);
		curl_easy_setopt(eh, CURLOPT_XFERINFODATA, &unhandledTask);
#else
		curl_easy_setopt(eh, CURLOPT_PROGRESSFUNCTION, older_progress);
		curl_easy_setopt(eh, CURLOPT_PROGRESSDATA, &unhandledTask);
#endif
#ifdef _DEBUG
		_input_requests.push_back(unhandledTask.GetRequest().GetUrl());
#endif // _DEBUG

		curl_easy_setopt(eh, CURLOPT_HEADER, 0L);
		curl_easy_setopt(eh, CURLOPT_URL, unhandledTask.GetRequest().GetUrl().c_str());
		unhandledTask.SetUnhandled(false);
		curl_multi_add_handle(cm, eh);
	}

	volatile  bool g_stop=true ;
	void Excutor()
	{
		CURLM *cm=nullptr;
		CURLMsg *msg;
		long L;
		unsigned int C = 0;
		int M, Q, U = -1;
		fd_set R, W, E;
		struct timeval T;

		cm = curl_multi_init();
		int MAX_SIZE = 6;
		/* we can optionally limit the total amount of connections this multi handle
		uses */
		curl_multi_setopt(cm, CURLMOPT_MAXCONNECTS, MAX_SIZE);

		while (true)
		{
			if (g_stop)
			{
				break;
			}

			//Once Queue is empty, Pending it
			while (!g_getTaskQueue.HasUnhandledTask())
			{
				std::this_thread::sleep_for(std::chrono::milliseconds(100));
#ifdef _DEBUG
				std::sort(_input_requests.begin(), _input_requests.end());
				std::sort(_excuted_requests.begin(), _excuted_requests.end());
				assert(_input_requests == _excuted_requests);
#endif // _DEBUG
			}
			//Init Debug
#ifdef _DEBUG
			_input_requests.clear();
			_excuted_requests.clear();
#endif // _DEBUG

			M = Q = U = -1;
			C = 0;

			int l_initNum = 0;
			while (g_getTaskQueue.HasUnhandledTask() && l_initNum++ < MAX_SIZE)
			{
				init(cm);
			}
			while (U) {
				//when running_handles is set to zero (0) on the return of this function, there is no longer any transfers in progress
				curl_multi_perform(cm, &U);
				//when U==0, all in finished
				if (U) {
					FD_ZERO(&R);
					FD_ZERO(&W);
					FD_ZERO(&E);
					if (curl_multi_fdset(cm, &R, &W, &E, &M)) {
						fprintf(stderr, "E: curl_multi_fdset\n");
						g_stop=true;
						return;
					}
					//An application using the libcurl multi interface should call curl_multi_timeout to figure out how long it should wait for socket actions - at most - before proceeding
					if (curl_multi_timeout(cm, &L)) {
						g_stop=true;
						return;
					}
					//optimum solution for next time timeout
					if (L == -1)
						L = 100;
					if (M == -1) {
						Sleep(L);
					}
					else {
						T.tv_sec = L / 1000;
						T.tv_usec = (L % 1000) * 1000;
						//The select() system call examines the I/O descriptor sets whose addresses are passed	in readfds, writefds, and exceptfds to see if some of their
						//	descriptors are ready for reading, are ready for writing, or have an exceptional condition pending, respectively
						if (0 > select(M + 1, &R, &W, &E, &T)) {
							/*fprintf(stderr, "E: select(%i,,,,%li): %i: %s\n",
								M + 1, L, errno, strerror(errno));*/
							g_stop=true;
							return;
						}
					}
				}

				while ((msg = curl_multi_info_read(cm, &Q))) {

					GetTask *task;
					CURL *e = msg->easy_handle;
					curl_easy_getinfo(msg->easy_handle, CURLINFO_PRIVATE, &task);
#ifdef _DEBUG
					_excuted_requests.push_back(task->GetRequest().GetUrl());
#endif // _DEBUG
					task->GetResponse().SetResult(msg->data.result);

					curl_multi_remove_handle(cm, e);
					curl_easy_cleanup(e);

					/*Execute action indicate by user*/
					task->GetHttpAction()->Do(task->GetResponse());
					delete task->GetHttpAction();
					g_getTaskQueue.PopProcessedTask(task->GetMark());
				}
				if (g_getTaskQueue.HasUnhandledTask()) {
					init(cm);
					U++; /* just to prevent it from remaining at 0 if there are more URLs to get */
				}
			}
		}
		g_stop=true;
		return;
	}


	void Http::Get(const std::string &url, HttpAction *httpAction) const
	{
		g_getTaskQueue.Push(GetTask(url, httpAction));
		if (g_stop==true)
		{
			g_stop = false;
			std::thread networker(Excutor);
			networker.detach();
		}
	}

	void Http::StopExcutor()const
	{
		g_stop=true;
	}


	Request::Request(const std::string &url) :m_url(url), m_unhandled(true)
	{

	}

	Request::Request(const Request &request) : m_url(request.m_url), m_unhandled(request.m_unhandled)
	{

	}

	const std::string & Request::GetUrl() const
	{
		return m_url;
	}

	void Request::SetUnhandled(bool handled)
	{
		m_unhandled = handled;
	}

	bool Request::isUnhandled() const
	{
		return m_unhandled;
	}

	Response::Response() :m_code(CURLE_OK), m_curl(nullptr), m_memory()
	{

	}

	Response::Response(const Response &response) : m_code(response.m_code), m_curl(response.m_curl), m_memory(response.m_memory)
	{

	}

	void Response::SetResult(CURLcode code)
	{
		m_code = code;
	}

	CURLcode Response::GetResult() const
	{
		return m_code;
	}

	const Network::Memory & Response::GetMemory() const
	{
		return m_memory;
	}

	Network::Memory & Response::GetMemory()
	{
		return m_memory;
	}

	void Response::SetCURL(CURL *curl)
	{
		m_curl = curl;
	}

	CURL * Response::GetCURL() const
	{
		return m_curl;
	}

	Memory::Memory()
	{
		m_size = 0;    /* no data at this point */
		m_memory = (char *)malloc(1);  /* will be grown as needed by the realloc above */
	}

	Memory::Memory(const Memory &memory)
	{
		m_size = memory.m_size;
		m_memory = (char *)malloc(memory.m_size);
		memcpy(m_memory, memory.m_memory, memory.m_size);
	}

	Memory::~Memory()
	{
		delete m_memory;
		m_memory = 0;
	}

}