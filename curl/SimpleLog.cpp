#include "StdAfx.h"
#include "SimpleLog.h"

#include <fcntl.h>
#include <io.h> 
#include <Windows.h>
#include <mutex>

CSimpleLog g_log;
std::mutex g_mtx;

#define MAX_LOG_SIZE 10*1024*1024 //10MB

CSimpleLog::CSimpleLog(void)
{
	char filename[MAX_PATH] = { 0 };
	GetModuleFileNameA(NULL, filename, MAX_PATH);
	std::string l_module(filename);
	int sum = l_module.find_last_of('\\')+1;
	l_module = l_module.substr(0, sum);
	m_path = l_module + "simple.log";
}

CSimpleLog::~CSimpleLog(void)
{
}

void CSimpleLog::SetPath(const string& path)
{
	m_path = path;
	Init();
}

string CSimpleLog::GetPath()
{
	return m_path;
}

void CSimpleLog::Init()
{
	//设置
	SetMaxLogFileSize(MAX_LOG_SIZE);

	//判断
	size_t l_size = GetLogFileSize();
	if (l_size > GetMaxLogFileSize())
	{
		ClearLogFile();
	}
}

size_t CSimpleLog::GetLogFileSize()
{
	FILE* l_file = NULL;
	size_t l_size = 0;
	fopen_s(&l_file, m_path.c_str(), "rb");
	if (l_file)
	{
		l_size = static_cast<size_t>(_filelength(_fileno(l_file)));
		fclose(l_file);
	}
	return l_size;
}

void CSimpleLog::ClearLogFile()
{
	FILE* l_file = NULL;
	fopen_s(&l_file, m_path.c_str(), "w");
	if (l_file)
	{
		fclose(l_file);
	}
}

void CSimpleLog::SetMaxLogFileSize(size_t size)
{
	m_maxLogFileSize = size;
}

size_t CSimpleLog::GetMaxLogFileSize() const
{
	return m_maxLogFileSize;
}

void CSimpleLog::Write(const char* pSourcePath, const char* pFunName, \
	const long lLine, const char* pLogText)
{
	//自动锁：调用即加锁，离开大括号作用域自动解锁  
	std::lock_guard<std::mutex> lock(g_mtx);

	if (pLogText == NULL)
		return;
	int nLogLen = strlen(pLogText);
	if (nLogLen == 0)
		return;
	int nSourceLen = strlen(pSourcePath);
	int nFunLen = strlen(pFunName);
	char szLine[10] = { 0 };
	sprintf_s(szLine, "%ld", lLine);
	int nLineLen = strlen(szLine);
	int nSpaceLen = 80 - nSourceLen - nFunLen - nLineLen;
	string strTime = GetTime();

	FILE* fp = NULL;
	fopen_s(&fp, m_path.c_str(), "a+");
	fwrite(strTime.c_str(), strTime.size(), 1, fp);
	fwrite(" ", 1, 1, fp);

	if (nSourceLen > 0)
	{
		fwrite(pSourcePath, nSourceLen, 1, fp);
		for (int i = 0; i < nSpaceLen; ++i)
			fwrite(" ", 1, 1, fp);
	}
	
	if (nFunLen > 0)
	{
		fwrite(pFunName, nFunLen, 1, fp);
		fwrite(" ", 1, 1, fp);
	}
	
	if (lLine > 0)
	{
		fwrite(szLine, nLineLen, 1, fp);
		fwrite(": ", 2, 1, fp);
	}
	fwrite(pLogText, nLogLen, 1, fp);
	fwrite("\n", 1, 1, fp);
	fclose(fp);

}

void CSimpleLog::Write(const char* pSourcePath, const char* pFunName, const long lLine, const wchar_t* pLogText)
{
	string strLogText = U2A(pLogText);
	Write(pSourcePath, pFunName, lLine, strLogText.c_str());
}

string CSimpleLog::GetTime()
{
	SYSTEMTIME st;
	::GetLocalTime(&st);
	char szTime[26] = { 0 };
	sprintf_s(szTime, "%04d-%02d-%02d %02d:%02d:%02d %03d  ", st.wYear, st.wMonth, st.wDay, st.wHour, \
		st.wMinute, st.wSecond, st.wMilliseconds);
	return szTime;
}

string CSimpleLog::U2A(const wstring& str)
{
	string strDes;
	if (str.empty())
		goto __end;
	int nLen = ::WideCharToMultiByte(CP_ACP, 0, str.c_str(), str.size(), NULL, 0, NULL, NULL);
	if (0 == nLen)
		goto __end;
	char* pBuffer = new char[nLen + 1];
	memset(pBuffer, 0, nLen + 1);
	::WideCharToMultiByte(CP_ACP, 0, str.c_str(), str.size(), pBuffer, nLen, NULL, NULL);
	pBuffer[nLen] = '\0';
	strDes.append(pBuffer);
	delete[] pBuffer;
__end:
	return strDes;
}

void CSimpleLog::ScanfWrite(const char* pSourcePath, const char* pFunName, const long lLine, \
	const char* pLogText, ...)
{
	va_list pArgs;
	va_start(pArgs, pLogText);
	char szBuffer[1024] = { 0 };
	_vsnprintf_s(szBuffer, 1024, pLogText, pArgs);
	va_end(pArgs);
	Write(pSourcePath, pFunName, lLine, szBuffer);
}

void CSimpleLog::ScanfWrite(const char* pSourcePath, const char* pFunName, const long lLine, \
	const wchar_t* pLogText, ...)
{
	va_list pArgs;
	va_start(pArgs, pLogText);
	wchar_t szBuffer[1024] = { 0 };
	_vsnwprintf_s(szBuffer, 1024, pLogText, pArgs);
	va_end(pArgs);
	Write(pSourcePath, pFunName, lLine, szBuffer);
}
