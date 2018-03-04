#ifdef _WIN32
#define  _CRT_SECURE_NO_WARNINGS
#endif
#include "stdafx.h"
#include <thread>
#include <memory>
#include <list>
#include <algorithm>
#include <mutex>
#include <condition_variable>
#include "Router.h"

namespace Http
{
	std::condition_variable cv;
	std::mutex cv_m;

	Memory::Memory()
	{
		Size(0);    /* no data at this point */
		MemoryAddr((char *)malloc(1));  /* will be grown as needed by the realloc above */
	}

	Memory::Memory(const Memory &memory)
		:size(memory.size), memoryAddr(nullptr)
	{
		Size(memory.Size());
		memoryAddr = (char *)malloc(memory.Size());
		memcpy(memoryAddr, memory.MemoryAddr(), memory.Size());
	}

	Memory::Memory(Memory &&memory)noexcept
		:memoryAddr(memory.memoryAddr), size(memory.size)
	{
		memory.MemoryAddr(nullptr);
	}

	bool Memory::operator==(const Memory&memory)const
	{
		return memoryAddr == memory.memoryAddr && size == memory.size;
	}

	Memory::~Memory()
	{
		free(MemoryAddr());
		MemoryAddr(0);
	}

	Request::Request(const std::string &url, void * userData) :url(url), userData(userData), unhandled(true)
	{}

	Request::Request(const Request &request) : url(request.Url()), userData(request.UserData()),
		unhandled(request.Unhandled())
	{}

	Request::Request(Request &&request)
		: url(std::move(request.url)), unhandled(request.unhandled), userData(request.userData)
	{
		request.UserData(nullptr);
	}

	bool Request::operator==(const Request&request) const
	{
		return url == request.url&&unhandled == request.unhandled&&userData == request.userData;
	}

	Request::~Request()
	{
		delete userData;
		UserData(0);
	}

	Response::Response() :Memory(), curlCode(CURLE_OK), curl(nullptr) {}

	Response::Response(const Response &response) : Memory(response), curlCode(response.CurlCode()), curl(response.Curl()) {}


	Response::Response(Response &&response)
		: curlCode(response.curlCode), curl(response.curl), Memory(std::move(response))
	{

	}

	bool Response::operator==(const Response &&response)const
	{
		if (Memory::operator==(static_cast<const Memory &&>(response)))
		{
			return curlCode == response.curlCode&&curl == response.curl;
		}
		return false;
	}

	Http::Router & Router::GetInstance()
	{
		static Router instance;
		return instance;
	}

	Task::Task(Task &&task) :
		Request(std::move(static_cast<Request&>(task))), Response(std::move(static_cast<Response&>(task))),
		action(task.Action()), mark(task.Mark())
	{
		task.Action(nullptr);
	}

	bool Task::operator==(const Task &task) const
	{
		if (Request::operator==(static_cast<const Task&&>(task)) &&
			Response::operator==(static_cast<const Task&&>(task)))
		{
			return mark == task.mark && action == task.action;
		}
		return false;
	}

	long long Task::markCouter = 0;
	volatile  bool g_createdExcutor = true;

	/*taskQueue maintains a task queue to perform task orderly*/
	class TaskQueue {
	public:
		std::list<Task>::iterator FrontUnhandledTask() {
			for (auto beg = std::begin(taskQueue); beg != std::end(taskQueue); ++beg)
			{
				if (beg->Unhandled())
				{
					return beg;
				}
			}
			return taskQueue.end();
		}
		bool HasUnhandledTask() {
			return !(FrontUnhandledTask() == taskQueue.end());
		}

		void Push(Task &&task) {
			std::lock_guard<std::mutex> lk(cv_m);
			cv.notify_one();
			taskQueue.push_back(std::move(task));
		}
		void Pop(long long mark) {
			taskQueue.remove_if([mark](const Task &task) {
				return mark == task.Mark();
			});
		}
	private:
		std::list<Task> taskQueue;
	};
	TaskQueue g_taskQueue;
	/*atomic variable determines TaskExcuter Thread Living Status*/

	static size_t WriteMemoryCallback(void *contents, size_t size, size_t nmemb, void *userp)
	{
		size_t realsize = size * nmemb;
		Task *l_memory = (Task *)userp;

		l_memory->MemoryAddr((char *)realloc(l_memory->MemoryAddr(), l_memory->Size() + realsize + 1));
		if (l_memory->MemoryAddr() == NULL)
			return 0;

		memcpy(&(l_memory->MemoryAddr()[l_memory->Size()]), contents, realsize);
		l_memory->Size(l_memory->Size() + realsize);
		l_memory->MemoryAddr()[l_memory->Size()] = 0;

		return realsize;
	}

	static int xferinfo(void *p, curl_off_t dltotal, curl_off_t dlnow, curl_off_t ultotal, curl_off_t ulnow)
	{
		double curtime = 0;
		Task *task = (Task *)p;
		curl_easy_getinfo(task->Curl(), CURLINFO_TOTAL_TIME, &curtime);
		return task->Action()->Progress(curtime, (double)dltotal, (double)dlnow, 0, 0);
	}

	static int older_progress(void *p, double dltotal, double dlnow, double ultotal, double ulnow)
	{
		return xferinfo(p, (curl_off_t)dltotal, (curl_off_t)dlnow, (curl_off_t)ultotal, (curl_off_t)ulnow);
	}


	static void init(CURLM *cm)
	{
		CURL *eh = curl_easy_init();
		Task& unhandledTask = *g_taskQueue.FrontUnhandledTask();
		unhandledTask.Curl(eh);
		//set easy handle option
		curl_easy_setopt(eh, CURLOPT_PRIVATE, (void *)&unhandledTask);
		curl_easy_setopt(eh, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);
		curl_easy_setopt(eh, CURLOPT_WRITEDATA, (void *)&unhandledTask);
		curl_easy_setopt(eh, CURLOPT_VERBOSE, 0L);
		curl_easy_setopt(eh, CURLOPT_NOPROGRESS, 0);
#if LIBCURL_VERSION_NUM >= 0x072000
		curl_easy_setopt(eh, CURLOPT_XFERINFOFUNCTION, xferinfo);
		curl_easy_setopt(eh, CURLOPT_XFERINFODATA, &unhandledTask);
#else
		curl_easy_setopt(eh, CURLOPT_PROGRESSFUNCTION, older_progress);
		curl_easy_setopt(eh, CURLOPT_PROGRESSDATA, &unhandledTask);
#endif

		curl_easy_setopt(eh, CURLOPT_HEADER, 0L);
		curl_easy_setopt(eh, CURLOPT_URL, unhandledTask.Url().c_str());
		unhandledTask.Unhandled(false);
		curl_multi_add_handle(cm, eh);
	}

	void Excutor()
	{

		CURLM *cm = nullptr;
		CURLMsg *msg;
		long L;
		int M, Q, U = -1;
		fd_set R, W, E;
		struct timeval T;
		CURLMcode code;
		cm = curl_multi_init();
		int MAX_SIZE = 9;
		/* we can optionally limit the total amount of connections this multi handle uses */
		curl_multi_setopt(cm, CURLMOPT_MAXCONNECTS, MAX_SIZE);

		while (true)
		{
			std::unique_lock<std::mutex> lk(cv_m);
			cv.wait(lk, [] {return g_taskQueue.HasUnhandledTask(); });

			M = Q = U = -1;

			int initNum = 0;
			while (g_taskQueue.HasUnhandledTask() && initNum++ < MAX_SIZE)
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
					if (code = curl_multi_fdset(cm, &R, &W, &E, &M)) {
						fprintf(stderr, "%s/n", curl_multi_strerror(code));
						return;
					}
					//An application using the libcurl multi interface should call curl_multi_timeout to figure out how long it should wait for socket actions - at most - before proceeding
					if (code = curl_multi_timeout(cm, &L)) {
						fprintf(stderr, "%s/n", curl_multi_strerror(code));
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
							fprintf(stderr, "E: select(%i,,,,%li): %i: %s\n",
								M + 1, L, errno, strerror(errno));
							return;
						}
					}
				}

				while ((msg = curl_multi_info_read(cm, &Q))) {
					Task *task;
					curl_easy_getinfo(msg->easy_handle, CURLINFO_PRIVATE, &task);
					task->CurlCode(msg->data.result);

					CURL *e = msg->easy_handle;
					curl_multi_remove_handle(cm, e);
					curl_easy_cleanup(e);

					/*Execute action indicate by user*/
					task->Action()->Do(*task);
					g_taskQueue.Pop(task->Mark());
				}
				if (g_taskQueue.HasUnhandledTask()) {
					init(cm);
					U++; /* just to prevent it from remaining at 0 if there are more URLs to get */
				}
			}
			lk.unlock();
		}
	}

	void Router::Get(const std::string &url, Action *httpAction, void * userData /*=-1*/)
	{
		if (std::find(std::begin(actionList), std::end(actionList), httpAction) != std::end(actionList))
		{
			actionList.push_back(httpAction);
		}
		g_taskQueue.Push(Task(url, httpAction, userData));
		if (g_createdExcutor == true)
		{
			g_createdExcutor = false;
			std::thread excutor(Excutor);
			excutor.detach();
		}
	}

	Router::~Router()
	{
		std::for_each(std::begin(actionList), std::end(actionList), [](Action *action) {
			delete action;
		});
	}

}