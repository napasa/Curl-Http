#include "stdafx.h"
#include <thread>
#include <iostream>
#include <assert.h> 
#include <mutex>
#include <chrono>
#include "Http.h"
#include "..\SimpleLog.h"

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

	std::queue<std::string> g_queueRequest({
		"http://open.55.la/index.php?class=WeChat&type=GetCode&guid=10888&token=KY0SQ3FX996PU1ZO&soft_type=1",
		"http://open.55.la/index.php?class=WeChat&type=GetCode&guid=10888&token=KY0SQ3FX996PU1ZO&soft_type=1",
		"http://open.55.la/index.php?class=WeChat&type=GetCode&guid=10888&token=KY0SQ3FX996PU1ZO&soft_type=1",
		"http://open.55.la/index.php?class=WeChat&type=GetCode&guid=10888&token=KY0SQ3FX996PU1ZO&soft_type=1",
		"http://open.55.la/index.php?class=WeChat&type=GetCode&guid=10888&token=KY0SQ3FX996PU1ZO&soft_type=1"
	});
	std::atomic_bool m_living{false};

	struct PrivateData{
		MemoryStruct m_memory;
		std::string m_url;
	};

	static size_t
		WriteMemoryCallback(void *contents, size_t size, size_t nmemb, void *userp)
	{
		size_t realsize = size * nmemb;
		PrivateData *data = (struct PrivateData *)userp;

		data->m_memory.memory = (char *)realloc(data->m_memory.memory, data->m_memory.size + realsize + 1);
		if (data->m_memory.memory == NULL) {
			/* out of memory! */
			printf("not enough memory (realloc returned NULL)\n");
			return 0;
		}

		memcpy(&(data->m_memory.memory[data->m_memory.size]), contents, realsize);
		data->m_memory.size += realsize;
		data->m_memory.memory[data->m_memory.size] = 0;

		return realsize;
	}



	static size_t cb(char *d, size_t n, size_t l, void *p)
	{
		/* take care of the data here, ignored in this example */
		(void)d;
		(void)p;
		return n*l;
	}

#ifdef _DEBUG
	/*测试是否所有request请求都会被执行*/
	std::vector<std::string> _input_requests;
	std::vector<std::string> _excuted_requests;
#endif
	static void init(CURLM *cm)
	{
		CURL *eh = curl_easy_init();

		//设置数据
		PrivateData *l_privateData = new PrivateData;
		l_privateData->m_url = g_queueRequest.front();
		l_privateData->m_memory.memory = (char *)malloc(1);
		l_privateData->m_memory.size = 0;
		curl_easy_setopt(eh, CURLOPT_PRIVATE, l_privateData);
		curl_easy_setopt(eh, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);
		curl_easy_setopt(eh, CURLOPT_WRITEDATA, (void *)l_privateData);

		curl_easy_setopt(eh, CURLOPT_VERBOSE, 0L);
#ifdef _DEBUG
		_input_requests.push_back(g_queueRequest.front());
#endif // _DEBUG

		curl_easy_setopt(eh, CURLOPT_HEADER, 0L);
		curl_easy_setopt(eh, CURLOPT_URL, g_queueRequest.front().c_str());


		g_queueRequest.pop();
		curl_multi_add_handle(cm, eh);
	}

	void RequestRun()
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
			//当队列为空时，则等待元素
			while (g_queueRequest.empty() == true)
			{
#ifdef _DEBUG
				std::sort(_input_requests.begin(), _input_requests.end());
				std::sort(_excuted_requests.begin(), _excuted_requests.end());
				assert(_input_requests == _excuted_requests);
#endif // _DEBUG
			}
			//初始化debug
#ifdef _DEBUG
			_input_requests.clear();
			_excuted_requests.clear();
#endif // _DEBUG

			M = Q = U = -1;
			C = 0;

			//初始化请求体
			int l_initNum = 0;
			while (g_queueRequest.size() != 0 && l_initNum++ < MAX_SIZE)
			{
				init(cm);
			}
			l_initNum = 0;
			while (U) {
				//读写CURLM中可用CURL数据。U值代表当前是否还有传输正在进行中。When running_handles is set to zero (0) on the return of this function, there is no longer any transfers in progress
				//因此此处用U来循环执行
				curl_multi_perform(cm, &U);
				//当U==0时意味着数据全都读写完毕
				if (U) {
					FD_ZERO(&R);
					FD_ZERO(&W);
					FD_ZERO(&E);
					//将cm中的fd分别读到三个fd_set当中，M值代表当前读取了多少个可用fd,疑问是M是什么意思
					if (curl_multi_fdset(cm, &R, &W, &E, &M)) {
						fprintf(stderr, "E: curl_multi_fdset\n");
						//return EXIT_FAILURE;
						return;
					}
					//An application using the libcurl multi interface should call curl_multi_timeout to figure out how long it should wait for socket actions - at most - before proceeding
					//获取自身该适当等待多久再次进行perform操作
					if (curl_multi_timeout(cm, &L)) {
						fprintf(stderr, "E: curl_multi_timeout\n");
						//return EXIT_FAILURE;
						return;
					}
					//应该再过多长时间再次perform或select
					if (L == -1)
						L = 100;

					if (M == -1) {
#ifdef WIN32
						Sleep(L);
#else
						sleep((unsigned int)L / 1000);
#endif
					}
					else {
						T.tv_sec = L / 1000;
						T.tv_usec = (L % 1000) * 1000;
						//The select() system call examines the I/O descriptor sets whose addresses are passed	in readfds, writefds, and exceptfds to see if some of their
						//	descriptors are ready for reading, are ready for writing, or have an exceptional condition pending, respectively
						if (0 > select(M + 1, &R, &W, &E, &T)) {
							fprintf(stderr, "E: select(%i,,,,%li): %i: %s\n",
								M + 1, L, errno, strerror(errno));
							//return EXIT_FAILURE;
							return;
						}
					}
				}

				while ((msg = curl_multi_info_read(cm, &Q))) {
					char l_result[MAX_PATH];
					if (msg->msg == CURLMSG_DONE) {
						PrivateData *l_privateData;
						CURL *e = msg->easy_handle;
						curl_easy_getinfo(msg->easy_handle, CURLINFO_PRIVATE, &l_privateData);
						_excuted_requests.push_back(l_privateData->m_url);
						if (msg->data.result == CURLE_OK)
						{
#ifdef _DEBUG
							static int i;
							std::string htmlname = std::to_string(i++) + ".html";
							FILE * pFile = fopen(htmlname.c_str(), "wb");
							fwrite(l_privateData->m_memory.memory, sizeof(char), l_privateData->m_memory.size, pFile);
							fclose(pFile);
#endif // _DEBUG
							//通知外界结果
						}
						else
						{
#ifdef _DEBUG
							sprintf(l_result, "R: %d - %s <%s>\n",
								msg->data.result, curl_easy_strerror(msg->data.result), l_privateData->m_url.c_str());
							SL_LOG(l_result);
#endif // _DEBUG
							//通知外界结果
						}
						
						delete l_privateData->m_memory.memory;
						delete l_privateData;
						curl_multi_remove_handle(cm, e);
						curl_easy_cleanup(e);
					}
					else {
						sprintf(l_result, "E: CURLMsg (%d)\n", msg->msg);
					}
					if (g_queueRequest.size()!=0) {
						init(cm);
						U++; /* just to prevent it from remaining at 0 if there are more
							 URLs to get */
					}
				}

			}
		}
		m_living.store(false);
	}


	void Http::Get(const std::string &url)
	{
		g_queueRequest.push(url);
		if (!m_living.load())
		{
			std::thread networker(RequestRun);
			networker.detach();
		}
	}

}