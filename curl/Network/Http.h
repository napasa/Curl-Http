#pragma once
#include <memory>
#include <list>
#include <algorithm>
#include <atomic>
#include "URL.h"
#include <curl\curl.h>

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
