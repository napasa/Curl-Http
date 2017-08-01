// curl.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "Network/Http.h"
#include "Network/URL.h"

#define STOP_DOWNLOAD_AFTER_THIS_MANY_BYTES         6000
#define MINIMAL_PROGRESS_FUNCTIONALITY_INTERVAL     3

int main(int argc, char *argv[])
{
	Network::URL url("http://cnv2.pdftodoc.cn/doc/word2pdf ");
	Network::Http::GetInstance()->Get("www.baidu.com");
	system("pause");
	return 0;
}
