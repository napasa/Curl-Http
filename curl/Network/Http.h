#pragma once
#include <memory>
#include "URL.h"
#include "..\..\include\curl\curl.h"

namespace Network
{
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
		virtual int Progress(double totaltime, double dltotal, double dlnow, double ultotal, double ulnow) = 0;
		virtual ~AbstractAction(){}
	};

	class Http
	{
	public:
		static Http* getInstance();

		typedef void(*FinishedCallback)(MemoryStruct *memory);

	public:
		void get(URL url, FinishedCallback cbk, Identifier identifier=0);
		void get(const char *url, FinishedCallback cbk, Identifier identifier=0);
		void get(const std::string &url, HWND hwnd = 0, Identifier identifier = 0);
		void get(const char *url, HWND hwnd = 0, Identifier identifier = 0);
		void get(URL url, HWND hwnd = 0, Identifier identifier = 0);
		void Get(const URL &url, std::shared_ptr<AbstractAction> action, Id id);
		void Post(const URL &url, std::shared_ptr<AbstractAction> action, Id id, const std::string &postedFilename);
		~Http();
		
		Http(const Http &http)=delete;
		Http& operator=(const Http&)=delete;
	private:
		Http(){};
		static size_t WriteMemoryCallback(void *contents, size_t size, size_t nmemb, void *userp);

		/* this is how the CURLOPT_XFERINFOFUNCTION callback works */
		static int xferinfo(void *p, curl_off_t dltotal, curl_off_t dlnow, curl_off_t ultotal, curl_off_t ulnow);

		static 	int older_progress(void *p, double dltotal, double dlnow, double ultotal, double ulnow);

		static void run(const URL &url, Http::FinishedCallback cbk, Identifier identifier);

		static void run1(const URL &url, HWND hwnd, Identifier identifier);
		
		static void run2(const URL &url, std::shared_ptr<AbstractAction> action, Id id);

		static Http* instance;
	};

}
