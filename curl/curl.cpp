// curl.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "Network/Http.h"
#include "Network/URL.h"
#include <iostream>
#include <SimpleLog/SimpleLog.h>

#define STOP_DOWNLOAD_AFTER_THIS_MANY_BYTES         6000
#define MINIMAL_PROGRESS_FUNCTIONALITY_INTERVAL     3


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
