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
	virtual void Do(Network::Response *response){
		HttpAction::Do(response);
		char l_result[MAX_PATH];
		if (response->GetResult() != CURLE_OK)
		{
#ifdef _DEBUG
			sprintf(l_result, "R: %d - %s \n",
				response->GetResult(), curl_easy_strerror(response->GetResult()));
			SL_LOG(l_result);
#endif // _DEBUG
		}
		else
		{
#ifdef _DEBUG
			static int i;
			std::string htmlname = std::to_string(i++) + ".html";
			FILE * pFile = fopen(htmlname.c_str(), "wb");
			fwrite(response->GetMemory()->m_memory, sizeof(char), response->GetMemory()->m_size, pFile);
			fclose(pFile);
#endif // _DEBUG
		}
	}
	~Action(){}
};

int main(int argc, char *argv[])
{
	curl_global_init(CURL_GLOBAL_ALL);
	
	std::string url;
	std::cout << "Input Request:  ";
	while (std::cin>> url)
	{
		std::cout << "Receive Request: ";
		//Network::Http::GetInstance()->Get(url, new Action());
		std::cout << url << std::endl;
		std::cout << "Input Request:  ";
	}
	return 0;
}
