#include "stdafx.h"
#include <thread>
#include <iostream>
#include <assert.h> 
#include <mutex>
#include <chrono>
#include "Http.h"

namespace Network
{
	Http* Http::instance = NULL;
	Http* Http::getInstance(){
		if (instance == NULL)
		{
			Http::instance = new Http();
		}
		return instance;
	}

	size_t Http::WriteMemoryCallback(void *contents, size_t size, size_t nmemb, void *userp)
	{
		size_t realsize = size * nmemb;
		MemoryStruct *mem = (MemoryStruct *)userp;

		mem->memory = (char *)realloc(mem->memory, mem->size + realsize + 1);
		assert(NULL != mem->memory);
		memcpy(&(mem->memory[mem->size]), contents, realsize);
		mem->size += realsize;
		mem->memory[mem->size] = 0;

		return realsize;
	}

	void Http::run(const URL &url, Http::FinishedCallback cbk, Identifier identifier)
	{
		CURL *handle = curl_easy_init();
		if (handle)
		{

			MemoryStruct *chunk = new MemoryStruct;
			chunk->memory = (char *)malloc(1);  /* will be grown as needed by the realloc above */
			chunk->size = 0;    /* no data at this point */


			CURLcode res;

			curl_easy_setopt(handle, CURLOPT_URL, url.getUrl().c_str());
			curl_easy_setopt(handle, CURLOPT_WRITEDATA, (void *)chunk);
			curl_easy_setopt(handle, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);
			curl_easy_setopt(handle, CURLOPT_USERAGENT, "libcurl-agent/1.0");
			curl_easy_setopt(handle, CURLOPT_TIMEOUT, 10L);
			curl_easy_setopt(handle, CURLOPT_CONNECTTIMEOUT, 10L);
			res = curl_easy_perform(handle);

			/* check for errors */
			if (CURLE_OK != res)
			{
				chunk->status = false;
				std::string errCode = std::to_string(res);
				chunk->memory = (char *)realloc(chunk->memory, chunk->size + errCode.size() + 1);
				memcpy(&(chunk->memory[chunk->size]), errCode.c_str(), errCode.size());
				chunk->size += errCode.size();
				chunk->memory[chunk->size] = 0;
			}
			else
			{
				chunk->status = true;
			}
			chunk->url = url;
			if (cbk != NULL)
			{
				cbk(chunk);
			}

			free(chunk->memory);
			delete chunk;
			curl_easy_cleanup(handle);
		}
	}
	std::mutex g_mutex;
	void Http::run1(const URL &url, HWND hwnd, Identifier identifier)
	{
		g_mutex.lock();

		CURL *handle = curl_easy_init();
		if (handle)
		{

			MemoryStruct *chunk = new MemoryStruct;
			chunk->memory = (char *)malloc(1);  /* will be grown as needed by the realloc above */
			chunk->size = 0;    /* no data at this point */


			CURLcode res;
			curl_easy_setopt(handle, CURLOPT_URL, url.getUrl().c_str());
			curl_easy_setopt(handle, CURLOPT_WRITEDATA, (void *)chunk);
			curl_easy_setopt(handle, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);
			curl_easy_setopt(handle, CURLOPT_USERAGENT, "libcurl-agent/1.0");
			curl_easy_setopt(handle, CURLOPT_TIMEOUT, 10L);
			curl_easy_setopt(handle, CURLOPT_CONNECTTIMEOUT, 10L);
			res = curl_easy_perform(handle);

			/* check for errors */
			if (CURLE_OK != res)
			{
				chunk->status = false;
				std::string errCode = std::to_string(res);
				chunk->memory = (char *)realloc(chunk->memory, chunk->size + errCode.size() + 1);
				memcpy(&(chunk->memory[chunk->size]), errCode.c_str(), errCode.size());
				chunk->size += errCode.size();
				chunk->memory[chunk->size] = 0;
			}
			else
			{
				chunk->status = true;
			}
			chunk->url = url;
			if (hwnd != 0)
			{
				SendMessage(hwnd, identifier, (WPARAM)chunk, 0);
			}
			free(chunk->memory);
			delete chunk;
			curl_easy_cleanup(handle);
		}
		g_mutex.unlock();
	}

	void Http::run2(const URL &url, std::shared_ptr<AbstractAction> action, Id id)
	{
		CURL *handle = curl_easy_init();
		if (handle)
		{

			MemoryStruct *chunk = new MemoryStruct;
			chunk->memory = (char *)malloc(1);  /* will be grown as needed by the realloc above */
			chunk->size = 0;    /* no data at this point */


			CURLcode res;
			curl_easy_setopt(handle, CURLOPT_URL, url.getUrl().c_str());
			curl_easy_setopt(handle, CURLOPT_WRITEDATA, (void *)chunk);
			curl_easy_setopt(handle, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);
			curl_easy_setopt(handle, CURLOPT_USERAGENT, "libcurl-agent/1.0");
			curl_easy_setopt(handle, CURLOPT_TIMEOUT, 10L);
			curl_easy_setopt(handle, CURLOPT_CONNECTTIMEOUT, 10L);
			res = curl_easy_perform(handle);

			/* check for errors */
			if (CURLE_OK != res)
			{
				chunk->status = false;
				std::string errCode = std::to_string(res);
				chunk->memory = (char *)realloc(chunk->memory, chunk->size + errCode.size() + 1);
				memcpy(&(chunk->memory[chunk->size]), errCode.c_str(), errCode.size());
				chunk->size += errCode.size();
				chunk->memory[chunk->size] = 0;
			}
			else
			{
				chunk->status = true;
			}
			chunk->url = url;
			chunk->id = id;
			action->Do(chunk);
			free(chunk->memory);
			delete chunk;
			curl_easy_cleanup(handle);
		}
		else
		{
			"xxx";
		}
	}

	void Http::get(URL url, FinishedCallback cbk, Identifier identifier)
	{
		std::thread http(Http::run, url, cbk, identifier);
		http.detach();
	}

	void Http::get(const char *url, FinishedCallback cbk, Identifier identifier)
	{
		std::thread http(Http::run, URL(url), cbk, identifier);
		http.detach();
	}

	void Http::get(const char *url, HWND hwnd /*= 0*/, Identifier identifier /*= 0*/)
	{
		std::thread http(Http::run1, URL(url), hwnd, identifier);
		http.detach();
	}

	void Http::get(URL url, HWND hwnd /*= 0*/, Identifier identifier /*= 0*/)
	{
		std::thread http(Http::run1, url, hwnd, identifier);
		http.detach();
	}

	void Http::get(const std::string &url, HWND hwnd /*= 0*/, Identifier identifier /*= 0*/)
	{
		std::thread http(Http::run1, URL(url), hwnd, identifier);
		http.detach();
	}

	void Http::Get(const URL &url, std::shared_ptr<AbstractAction> action, Id id)
	{

		std::thread http(Http::run2, URL(url), action, id);
		http.detach();
	}

	struct myprogress {
		double lastruntime;
		std::shared_ptr<AbstractAction> action;
		CURL *curl;
	};

#define MINIMAL_PROGRESS_FUNCTIONALITY_INTERVAL     0.001
	/* this is how the CURLOPT_XFERINFOFUNCTION callback works */
	int Http::xferinfo(void *p, curl_off_t dltotal, curl_off_t dlnow, curl_off_t ultotal, curl_off_t ulnow)
	{
		struct myprogress *myp = (struct myprogress *)p;
		CURL *curl = myp->curl;
		double curtime = 0;

		curl_easy_getinfo(curl, CURLINFO_TOTAL_TIME, &curtime);

		if ((curtime - myp->lastruntime) >= MINIMAL_PROGRESS_FUNCTIONALITY_INTERVAL) {
			myp->lastruntime = curtime;
			return myp->action->Progress(curtime, 0, 0, (double)ultotal, (double)ulnow);
		}
		return 0;
	}

	int Http::older_progress(void *p, double dltotal, double dlnow, double ultotal, double ulnow)
	{
		return xferinfo(p,
			(curl_off_t)dltotal,
			(curl_off_t)dlnow,
			(curl_off_t)ultotal,
			(curl_off_t)ulnow);
	}


	void Http::Post(const URL &url, std::shared_ptr<AbstractAction> action, Id id, const std::string &postedFilename)
	{
		CURL *handle;
		CURLcode res;

		struct myprogress prog;

		struct curl_httppost *formpost = NULL;
		struct curl_httppost *lastptr = NULL;
		struct curl_slist *headerlist = NULL;
		static const char buf[] = "Expect:";

		curl_global_init(CURL_GLOBAL_ALL);

		/* Fill in the file upload field */
		const char * basename = postedFilename.data() + postedFilename.find_last_of('\\') + 1;
		curl_formadd(&formpost,
			&lastptr,
			CURLFORM_COPYNAME, "file",
			CURLFORM_FILE, basename,
			CURLFORM_END);


		/* Fill in the submit field too, even if this is rarely needed */
		curl_formadd(&formpost,
			&lastptr,
			CURLFORM_COPYNAME, "submit",
			CURLFORM_COPYCONTENTS, "send",
			CURLFORM_END);

		handle = curl_easy_init();
		/* initialize custom header list (stating that Expect: 100-continue is not
		wanted */
		headerlist = curl_slist_append(headerlist, buf);
		if (handle) {
			prog.curl = handle;
			prog.action = action;
			MemoryStruct *chunk = new MemoryStruct;
			chunk->memory = (char *)malloc(1);  /* will be grown as needed by the realloc above */
			chunk->size = 0;    /* no data at this point */
			
			curl_easy_setopt(handle, CURLOPT_NOPROGRESS, 0);
			curl_easy_setopt(handle, CURLOPT_WRITEDATA, (void *)chunk);
			curl_easy_setopt(handle, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);

			curl_easy_setopt(handle, CURLOPT_PROGRESSFUNCTION, older_progress);
			/* pass the struct pointer into the progress function */
			curl_easy_setopt(handle, CURLOPT_PROGRESSDATA, &prog);

#if LIBCURL_VERSION_NUM >= 0x072000
			curl_easy_setopt(handle, CURLOPT_XFERINFOFUNCTION, xferinfo);
			/* pass the struct pointer into the xferinfo function, note that this is
			an alias to CURLOPT_PROGRESSDATA */
			curl_easy_setopt(handle, CURLOPT_XFERINFODATA, &prog);
#endif

			/* what URL that receives this POST */
			curl_easy_setopt(handle, CURLOPT_URL, url.getUrl().c_str());
			//if ((argc == 2) && (!strcmp(argv[1], "noexpectheader")))
				/* only disable 100-continue header if explicitly requested */
			//	curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headerlist);
			curl_easy_setopt(handle, CURLOPT_HTTPPOST, formpost);
			//curl_easy_setopt(handle, CURLOPT_VERBOSE, 1L);
			/* Perform the request, res will get the return code */
			res = curl_easy_perform(handle);
			/* Check for errors */
			if (res != CURLE_OK)
			{
				chunk->status = false;
				std::string errCode = std::to_string(res);
				chunk->memory = (char *)realloc(chunk->memory, chunk->size + errCode.size() + 1);
				memcpy(&(chunk->memory[chunk->size]), errCode.c_str(), errCode.size());
				chunk->size += errCode.size();
				chunk->memory[chunk->size] = 0;
			}
			else
			{
				chunk->status = true;
			}
			chunk->id = id;
			action->Do(chunk);
			free(chunk->memory);
			delete chunk;

			/* always cleanup */
			curl_easy_cleanup(handle);

			/* then cleanup the formpost chain */
			curl_formfree(formpost);
			/* free slist */
			curl_slist_free_all(headerlist);
		}
	}
	Http::~Http()
	{
		curl_global_cleanup();
	}

}