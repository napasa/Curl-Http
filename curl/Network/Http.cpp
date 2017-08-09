#include "stdafx.h"
#include <thread>
#include <iostream>
#include <assert.h> 
#include <mutex>
#include <chrono>
#include "Http.h"
#include "..\SimpleLog.h"

namespace Network
{
	const Http &  Http::GetInstance(){
			static Http instance;
		return instance;
	}

	/*HTTP Get Task. You Used HTTP::Get Method a time Which created a GetTask, System will push your Task into GetTaskQueue*/
	class GetTask {
	public:
		GetTask(const std::string &url, std::shared_ptr<HttpAction> action) :m_request(url), m_response(), m_httpAction(action) {

		}
		GetTask(const GetTask &gettask) :m_request(gettask.m_request), m_response(gettask.m_response), m_httpAction(gettask.m_httpAction) {
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

	/*GetTaskQueue maintains a task queue to perform task orderly*/
	class GetTaskQueue {
	public:
		GetTask &FrontUnhandledTask() {
			for (GetTask &httpGetTask : m_getTaskQueue)
			{
				if (httpGetTask.GetRequest().isUnhandled())
					return httpGetTask;
			}
		}
		bool HasUnhandledTask() {
			for (GetTask &httpGetTask : m_getTaskQueue)
			{
				if (httpGetTask.GetRequest().isUnhandled())
					return true;
			}
			return false;
		}
		void Push(const GetTask &gettask) {
			m_getTaskQueue.push_back(gettask);
		}
		void PopProcessedRequest() {
			m_getTaskQueue.remove_if([](const GetTask &getwork) {
				return !getwork.GetRequest().isUnhandled();
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
	std::atomic_bool m_living{ false };

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


#ifdef _DEBUG
	/*Test Whether All Request Will be Excuted*/
	std::vector<std::string> _input_requests;
	std::vector<std::string> _excuted_requests;
#endif
	static void init(CURLM *cm)
	{
		CURL *eh = curl_easy_init();

		GetTask& unhandledTask = g_getTaskQueue.FrontUnhandledTask();
		Network::Memory &l_memory = unhandledTask.GetResponse().GetMemory();
		//设置easy handle option
		curl_easy_setopt(eh, CURLOPT_PRIVATE, &unhandledTask);
		curl_easy_setopt(eh, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);
		curl_easy_setopt(eh, CURLOPT_WRITEDATA, (void *)&l_memory);
		curl_easy_setopt(eh, CURLOPT_VERBOSE, 0L);
#ifdef _DEBUG
		_input_requests.push_back(unhandledTask.GetRequest().GetUrl());
#endif // _DEBUG

		curl_easy_setopt(eh, CURLOPT_HEADER, 0L);
		curl_easy_setopt(eh, CURLOPT_URL, unhandledTask.GetRequest().GetUrl().c_str());
		unhandledTask.GetRequest().SetHandledStatus(false);
		curl_multi_add_handle(cm, eh);
	}

	void Excutor()
	{
		m_living.store(true);

		CURLM *cm;
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
			//Once Queue is empty, Pending it
			while (!g_getTaskQueue.HasUnhandledTask())
			{
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
			l_initNum = 0;
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
						m_living.store(false);
						return;
					}
					//An application using the libcurl multi interface should call curl_multi_timeout to figure out how long it should wait for socket actions - at most - before proceeding
					if (curl_multi_timeout(cm, &L)) {
						m_living.store(false);
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
							m_living.store(false);
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

					/*Execute action indicate by user*/
					task->GetHttpAction()->Do(std::make_shared<Response>(task->GetResponse()));
					g_getTaskQueue.PopProcessedRequest();


					curl_multi_remove_handle(cm, e);
					curl_easy_cleanup(e);

					if (g_getTaskQueue.HasUnhandledTask()) {
						init(cm);
						U++; /* just to prevent it from remaining at 0 if there are more
							 URLs to get */
					}
				}

			}
		}
		m_living.store(false);
	}


	void Http::Get(const std::string &url, std::shared_ptr<HttpAction> action)const
	{
		GetTask task(url, action);
		g_getTaskQueue.Push(task);
		if (!m_living.load())
		{
			std::thread networker(Excutor);
			networker.detach();
		}
	}

}