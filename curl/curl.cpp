// curl.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <iostream>
#include <thread>
#include <ctime>
#include <SimpleLog/SimpleLog.h>
#include "Network/Router.h"

#define STOP_DOWNLOAD_AFTER_THIS_MANY_BYTES         6000
#define MINIMAL_PROGRESS_FUNCTIONALITY_INTERVAL     3

enum RequestType {
	SOFTWARE_TRATEGY,
	NEWEST_VERSION,
	PULL_PACKAGE,
	USER_INFO,
	PACKAGE_LIST,
	QRCODE,
	QRCODE_STATUS
};

struct UserData {
	UserData(RequestType reqType) :m_requestType(reqType) { }
	RequestType m_requestType;
	int test[1024] = {1,2,3,45,8888,743,4,344,5637,3,73,3,57};
};

int i = 0;
std::chrono::time_point<std::chrono::system_clock> start;
class HttpAction : public Http::Action {
public:
	static HttpAction *GetInstance() {
		static HttpAction s_userAction;
		return &s_userAction;
	}
private:
	virtual void Do(const Http::Task&task) override {
		static int j = 1;
		if (j++== 1000) {
			auto end = std::chrono::system_clock::now();
			std::chrono::duration<double> elapsed_seconds = end - start;
			std::time_t end_time = std::chrono::system_clock::to_time_t(end);
			std::cout << "finished computation at " << std::ctime(&end_time)
				<< "elapsed time: " << elapsed_seconds.count() << "s. request counts:" << j <<"\n";
		}
		else {
			std::cout << "processed: " << j << std::endl;
		}
	};
};

int main(int argc, char *argv[])
{
#define ACTION_INIT HttpAction::GetInstance()
#define ACTION_OBJ ACTION_INIT
	start = std::chrono::system_clock::now();
	while (i++<1000)
	{
		std::string url = "www.baidu.com";
		ROUTER.Get(url, ACTION_OBJ/*, new UserData(RequestType::PACKAGE_LIST)*/);
	}
	system("pause");
	return 0;
}
