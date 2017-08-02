// curl.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "Network/Http.h"
#include "Network/URL.h"
#include <iostream>

#define STOP_DOWNLOAD_AFTER_THIS_MANY_BYTES         6000
#define MINIMAL_PROGRESS_FUNCTIONALITY_INTERVAL     3

int main(int argc, char *argv[])
{
	curl_global_init(CURL_GLOBAL_ALL);
	Network::g_queueRequest.push("www.alibaba.com");
	Network::Http::GetInstance()->Get("www.baidu.com");
	
	std::string request;
	std::cout << "Input Request:  ";
	while (std::cin>> request)
	{
		std::cout << "Receive Request: ";
		//Network::g_queueRequest.push(request);
		Network::Http::GetInstance()->Get(request);
		std::cout << request << std::endl;
		std::cout << "Input Request:  ";
	}
	return 0;
}
