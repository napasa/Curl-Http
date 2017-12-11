// curl.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "Network/Http.h"
#include "Network/URL.h"
#include <iostream>
#include <SimpleLog/SimpleLog.h>

#define STOP_DOWNLOAD_AFTER_THIS_MANY_BYTES         6000
#define MINIMAL_PROGRESS_FUNCTIONALITY_INTERVAL     3


class Action : public Network::HttpAction{
public:
	Action(){}
	virtual void Do(const Network::Response  &response)override{
		char l_result[MAX_PATH];
		if (response.GetResult() != CURLE_OK)
		{
#ifdef _DEBUG
			//sprintf(l_result, "R: %d - %s \n",
			//	response.GetResult(), curl_easy_strerror(response.GetResult()));
			//printf(l_result);
#endif // _DEBUG
		}
		else
		{
#ifdef _DEBUG
			//printf(response.GetMemory().m_memory);
#endif // _DEBUG
		}
	}
	virtual int Progress(double totaltime, double dltotal, double dlnow, double ultotal, double ulnow)override {
		return 0;
	}
	~Action(){}
};


int main(int argc, char *argv[])
{
	curl_global_init(CURL_GLOBAL_ALL);

	{
		Network::Http::GetInstance().Get("127.0.0.1/alipay/createqrpay",  new Action);
		Network::Http::GetInstance().Get("127.0.0.1/alipay/createqrpay", new Action);
	}
	system("pause");
	return 0;
}
