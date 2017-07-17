// curl.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "Network/Http.h"
#include "Network/URL.h"

#define STOP_DOWNLOAD_AFTER_THIS_MANY_BYTES         6000
#define MINIMAL_PROGRESS_FUNCTIONALITY_INTERVAL     3

class TestAction : public  Network::AbstractAction
{
public:
	virtual void Do(const Network::MemoryStruct *memory) override
	{
		printf(memory->memory);
	}
	virtual int Progress(double totaltime, double dltotal, double dlnow, double ultotal, double ulnow) override
	{
		char *fmt = "Upload Percents = %.3f After %.2f Seconds\r\n";
		if (ultotal != 0 && ulnow != 0)
		{
			double p = (double)ulnow / (double)ultotal;
			fprintf(stderr, fmt,
				p, totaltime);
		}
#define UP_LIMIT 100
		if (totaltime > UP_LIMIT)
			return 1;
		else
			return 0;
	}
};
int main(int argc, char *argv[])
{
	Network::URL url("http://cnv2.pdftodoc.cn/doc/word2pdf ");
	Network::Http::GetInstance()->Post(url, std::make_shared<TestAction>(), 10, "C:\\Users\\no6fo\\Desktop\\32571_20150608.doc");
	system("pause");
	return 0;
}
