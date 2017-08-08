#pragma once

#include <string>

using std::string;
using std::wstring;

class CSimpleLog
{
public:
	CSimpleLog(void);
	~CSimpleLog(void);

	void SetPath(const string& path);
	string GetPath();

	void Init();

	size_t GetLogFileSize();	// 得到日志文件大小
	void ClearLogFile();		// 清空日志文件内容

	void SetMaxLogFileSize(size_t size);
	size_t GetMaxLogFileSize() const;

	void Write(const char* pSourcePath, const char* pFunName, const long lLine, const char* pLogText);
	void Write(const char* pSourcePath, const char* pFunName, const long lLine, const wchar_t* pLogText);
	void ScanfWrite(const char* pSourcePath, const char* pFunName, const long lLine, const char* pLogText, ...);
	void ScanfWrite(const char* pSourcePath, const char* pFunName, const long lLine, const wchar_t* pLogText, ...);

protected:
	string GetTime();
	string U2A(const wstring& str);
	string m_path;				// 日志路径，设置默认路径
	size_t m_maxLogFileSize;	// 日志文件最大大小 超过该大小将被删除 默认为10MB
};

extern CSimpleLog g_log;

#define SL_LOG(x)				g_log.Write(__FILE__, __FUNCTION__, __LINE__, x)
#define SL_LOG1(x, p1)			<span style="white-space:pre">	</span>g_log.ScanfWrite(__FILE__, __FUNCTION__, __LINE__, x, p1)
#define SL_LOG2(x, p1, p2)		<span style="white-space:pre">	</span>g_log.ScanfWrite(__FILE__, __FUNCTION__, __LINE__, x, p1, p2)
#define SL_LOG3(x, p1, p2, p3)	<span style="white-space:pre">		</span>g_log.ScanfWrite(__FILE__, __FUNCTION__, __LINE__, x, p1, p2, p3)


