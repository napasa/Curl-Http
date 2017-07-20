#include "stdafx.h"
#include <thread>
#include <iostream>
#include <assert.h> 
#include <mutex>
#include <chrono>
#include "Http.h"


struct data {
	char trace_ascii; /* 1 or 0 */
	FILE *stream;
};

static
void dump(const char *text,
	FILE *stream, unsigned char *ptr, size_t size,
	char nohex)
{
	size_t i;
	size_t c;

	unsigned int width = 0x10;

	if (nohex)
		/* without the hex output, we can fit more on screen */
		width = 0x40;

	fprintf(stream, "%s, %10.10ld bytes (0x%8.8lx)\n",
		text, (long)size, (long)size);

	for (i = 0; i < size; i += width) {

		fprintf(stream, "%4.4lx: ", (long)i);

		if (!nohex) {
			/* hex not disabled, show it */
			for (c = 0; c < width; c++)
				if (i + c < size)
					fprintf(stream, "%02x ", ptr[i + c]);
				else
					fputs("   ", stream);
		}

		for (c = 0; (c < width) && (i + c < size); c++) {
			/* check for 0D0A; if found, skip past and start a new line of output */
			if (nohex && (i + c + 1 < size) && ptr[i + c] == 0x0D && ptr[i + c + 1] == 0x0A) {
				i += (c + 2 - width);
				break;
			}
			fprintf(stream, "%c",
				(ptr[i + c] >= 0x20) && (ptr[i + c] < 0x80) ? ptr[i + c] : '.');
			/* check again for 0D0A, to avoid an extra \n if it's at width */
			if (nohex && (i + c + 2 < size) && ptr[i + c + 1] == 0x0D && ptr[i + c + 2] == 0x0A) {
				i += (c + 3 - width);
				break;
			}
		}
		fputc('\n', stream); /* newline */
	}
	fflush(stream);
}

static
int my_trace(CURL *handle, curl_infotype type,
	char *data, size_t size,
	void *userp)
{
	struct data *config = (struct data *)userp;
	const char *text;
	(void)handle; /* prevent compiler warning */

	switch (type) {
	case CURLINFO_TEXT:
		fprintf(config->stream, "== Info: %s", data);
		/* FALLTHROUGH */
	default: /* in case a new one is introduced to shock us */
		return 0;

	case CURLINFO_HEADER_OUT:
		text = "=> Send header";
		break;
	case CURLINFO_DATA_OUT:
		text = "=> Send data";
		break;
	case CURLINFO_SSL_DATA_OUT:
		text = "=> Send SSL data";
		break;
	case CURLINFO_HEADER_IN:
		text = "<= Recv header";
		break;
	case CURLINFO_DATA_IN:
		text = "<= Recv data";
		break;
	case CURLINFO_SSL_DATA_IN:
		text = "<= Recv SSL data";
		break;
	}

	dump(text, config->stream, (unsigned char *)data, size, config->trace_ascii);
	return 0;
}

namespace Network
{
	Http* Http::instance = NULL;
	Http* Http::GetInstance(){
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

	void Http::GetRun(const URL &url, std::shared_ptr<AbstractAction> action, Id id)
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
	}

	struct myprogress {
		double lastruntime;
		std::shared_ptr<AbstractAction> action;
		CURL *curl;
	};
	void Http::PostRun(const URL &url, std::shared_ptr<AbstractAction> action, Id id, const std::string &postedFilename)
	{
		CURL *handle;
		CURLcode res;

		struct myprogress prog;

		struct curl_httppost *formpost = NULL;
		struct curl_httppost *lastptr = NULL;

		struct curl_slist *headerlist = NULL;
		static const char buf[] = "Expect:";

		/* Fill in the file upload field */
		curl_formadd(&formpost,
			&lastptr,
			CURLFORM_COPYNAME, "file",
			CURLFORM_FILE, postedFilename.c_str(),
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

#ifdef _DEBUG
			struct data config;
			config.trace_ascii = 1;
			config.stream = fopen("dump", "a+");
			curl_easy_setopt(handle, CURLOPT_DEBUGFUNCTION, my_trace);
			curl_easy_setopt(handle, CURLOPT_DEBUGDATA, &config);
#endif



			curl_easy_setopt(handle, CURLOPT_NOPROGRESS, 0);
			curl_easy_setopt(handle, CURLOPT_WRITEDATA, (void *)chunk);
			curl_easy_setopt(handle, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);
			curl_easy_setopt(handle, CURLOPT_VERBOSE, 1);
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
			curl_easy_setopt(handle, CURLOPT_HTTPHEADER, headerlist);
			curl_easy_setopt(handle, CURLOPT_HTTPPOST, formpost);
			curl_easy_setopt(handle, CURLOPT_VERBOSE, 1L);
			/* Perform the request, res will get the return code */
			res = curl_easy_perform(handle);
#ifdef _DEBUG
			fclose(config.stream);
#endif // DEBUG


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


	void Http::Get(const URL &url, std::shared_ptr<AbstractAction> action, Id id, bool async/*=true*/)
	{
		if (async == true)
		{
			std::thread http(Http::GetRun, URL(url), action, id);
			http.detach();
		}
		else
		{
			GetRun(url, action, id);
		}
	}


#define MINIMAL_PROGRESS_FUNCTIONALITY_INTERVAL     0.001
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


	void Http::Post(const URL &url, std::shared_ptr<AbstractAction> action, Id id, const std::string &postedFilename, bool async/*=true*/)
	{
		if (async == true)
		{
			std::thread http(Http::PostRun, url, action, id, postedFilename);
			http.detach();
		}
		else
		{
			PostRun(url, action, id, postedFilename);
		}
	}
	Http::~Http()
	{
		;
	}

}