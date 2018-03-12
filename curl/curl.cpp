// curl.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <iostream>
#include <thread>
#include <ctime>
#include "Http/Router.h"


using namespace Http;

class MyAction :public Action{
public:
	virtual void Do(const Http::Task&task)override{
		std::cout << curl_easy_strerror(task.CurlCode());
	}
	virtual int Progress(double totaltime, double dltotal, double dlnow, double ultotal, double ulnow, const Http::Task&task)override{
		return 0;
	}
};

std::string Utf16ToUtf8(const std::wstring &utf16)
{
	int utf8_size = WideCharToMultiByte(CP_UTF8, 0, utf16.c_str(),
		utf16.length(), nullptr, 0,
		nullptr, nullptr);
	std::string utf8_str;
	utf8_str.resize(utf8_size, '\0');
	WideCharToMultiByte(CP_UTF8, 0, utf16.c_str(),
		utf16.length(), &utf8_str[0], utf8_size,
		nullptr, nullptr);
	return utf8_str;
}

int main(int argc, char *argv[])
{
	std::string url = "127.0.0.1:8000";
	MyAction action;
	std::vector<UploadedData> data{ { UploadedData::FIELD::STRING, "key1",  Utf16ToUtf8(L"值").c_str() }, 
	{ UploadedData::FIELD::STRING, "key2", "value2" },
	{ UploadedData::FIELD::FILE, "file1", "C:\\Users\\admin\\Desktop\\空 格\\C++ Design Generic Programming and Design Patterns Applied.pdf" } };
	ROUTER.Post(url, data, &action);
	ROUTER.Post(url, data, &action);
	ROUTER.Get("www.baidu.com", &action);
	system("pause");
	return 0;
}
