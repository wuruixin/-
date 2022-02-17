#pragma once
#include <WinSock2.h>
#include <windows.h>
#pragma comment(lib, "ws2_32.lib")

class CInitSockLib
{
public:
	CInitSockLib();
	~CInitSockLib();

private:
	static CInitSockLib m_initsocklib;
};

