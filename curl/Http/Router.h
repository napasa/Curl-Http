#pragma once
#include <vector>
#include <curl/curl.h>

#define ROUTER Http::Router::GetInstance()
namespace Http
{
	/*Memory used for storing response Content*/
	class Memory
	{
	public:
		Memory();
		Memory(const Memory &memory);
		Memory(Memory &&memory)noexcept;
		Memory & operator=(const Memory &memory) = delete;
		bool operator==(const Memory&)const;
		//Setter and getter
	public:
		char * MemoryAddr() const { return memoryAddr; }
		void MemoryAddr(char * val) { memoryAddr = val; }
		size_t Size() const { return size; }
		void Size(size_t val) { size = val; }
		virtual ~Memory();
	private:
		char *memoryAddr;
		size_t size;
	};

	/*HTTP request*/
	class TaskQueue;
	class Request
	{
	public:
		Request() = delete;
		Request(const std::string &url, void * userData = nullptr);
		Request(const Request &request);
		Request(Request &&request);
		Request & operator=(const Request &) = delete;
		bool operator==(const Request&request)const;
		~Request();
		//Setter and getter
	public:
		const std::string &Url() const { return url; }
		void Url(const std::string &val) { url = val; }
		void Unhandled(bool ) { unhandled = false; }
		bool Unhandled() const { return unhandled; }
		void * UserData() const { return userData; }
		void UserData(void * val) { userData = val; }
	protected:
		std::string url;
		bool unhandled;
		void *userData;

	};

	/*HTTP response*/
	class Response : public Memory {
	public:
		Response();
		Response(const Response &response);
		Response(Response &&response);
		Response & operator=(const Response &) = delete;
		bool operator==(const Response &&response)const;
		~Response() {}
		//Setter and getter
	public:
		CURLcode CurlCode() const { return curlCode; }
		void CurlCode(CURLcode val) { curlCode = val; }
		CURL * Curl() const { return curl; }
		void Curl(CURL * val) { curl = val; }
	private:
		CURLcode curlCode;
		CURL *curl;
	};

	class Action;
	/*HTTP task. Queue model*/
	class Task : public Request, public Response {
	public:
		Task() = delete;
		Task(const Task &task) : Request(task), Response(task),
			action(task.action), mark(task.mark) {}
		Task(Task &&task);
		Task(const std::string &url, Action *action, void * userdata = nullptr)
			:Request(url, userdata), Response(), action(action), mark(Task::markCouter++) {}
		bool operator==(const Task &task)const;
		~Task() {  }
		//Setter and getter
	public:
		Http::Action * Action() const { return action; }
		void Action(Http::Action * val) { action = val; }
		long long Mark() const { return mark; }
		void Mark(long long val) { mark = val; }
	private:
		Http::Action *action;
		long long mark;
		static long long markCouter;
	};

	/*HTTP action for response from server, overload do func to perform action to response*/
	class Action {
	public:
		Action() {}
		virtual void Do(const Http::Task&task) = 0;
		virtual int Progress(double totaltime, double dltotal, double dlnow, double ultotal, double ulnow, const Http::Task&task)=0; //{ return 0; }
		~Action() {}
	};

	class Router
	{
	public:
		static  Router & GetInstance();
		void Get(const std::string &url, Action *httpAction, void * userData = nullptr);
		~Router();
		Router(const Router &http) = delete;
		Router& operator=(const Router&) = delete;
		std::vector<Action *> ActionList() const { return actionList; }
		void ActionList(std::vector<Action *> val) { actionList = val; }
	private:
		std::vector<Action *> actionList;
		Router() {}
	};

}
