#ifndef _LJFTCPSERVER_HPP_
#define _LJFTCPSERVER_HPP_

#ifdef _WIN32
	#define WIN32_LEAN_AND_MEAN
	#define _WINSOCK_DEPRECATED_NO_WARNINGS
	#include<windows.h>
	#include<WinSock2.h>
	#pragma comment(lib,"ws2_32.lib")
#else
	#include<unistd.h> 
	#include<arpa/inet.h>
	#include<string.h>

	#define SOCKET int
	#define INVALID_SOCKET  (SOCKET)(~0)
	#define SOCKET_ERROR            (-1)
#endif

#include<stdio.h>
#include <iostream>
#include<vector>
#include"MessageHeader.hpp"

using namespace std;

class LJFTcpServer
{
private:
	SOCKET _sock;
	std::vector<SOCKET> g_clients;
public:
	LJFTcpServer()
	{
		_sock = INVALID_SOCKET;
	}
	virtual ~LJFTcpServer()
	{
		Close();
	}
	
	SOCKET InitSocket()
	{
#ifdef _WIN32
		//win socket 2.x
		WORD ver = MAKEWORD(2, 2);
		WSADATA dat;
		WSAStartup(ver, &dat);
#endif
		if (INVALID_SOCKET != _sock)
		{
			cout<<"socket is can not to use is: "<<_sock<<endl;
			Close();
		}
		_sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
		if (INVALID_SOCKET == _sock)
		{
			cout<<"create socket is error"<<endl;
		}
		else {
			cout<<"create socket ok, is "<<_sock<<endl;
		}
		return _sock;
	}

	int Bind(const char* ip, unsigned short port)
	{
		sockaddr_in _sin = {};
		_sin.sin_family = AF_INET;
		_sin.sin_port = htons(port);

#ifdef _WIN32
		if (ip){
			_sin.sin_addr.S_un.S_addr = inet_addr(ip);
		}
		else {
			_sin.sin_addr.S_un.S_addr = INADDR_ANY;
		}
#else
		if (ip) {
			_sin.sin_addr.s_addr = inet_addr(ip);
		}
		else {
			_sin.sin_addr.s_addr = INADDR_ANY;
		}
#endif
		int ret = bind(_sock, (sockaddr*)&_sin, sizeof(_sin));
		if (SOCKET_ERROR == ret)
		{
			cout<<"bind socket error"<<endl;
		}
		else {
			cout<<"bing socket ok"<<endl;
		}
		return ret;
	}


	int Listen(int n)
	{
		int ret = listen(_sock, n);
		if (SOCKET_ERROR == ret)
		{
			cout<<"bing socket error "<<_sock<<endl;
		}
		else {
			cout<<"bing socket ok "<<_sock<<endl;
		}
		return ret;
	}

	SOCKET Accept()
	{
		sockaddr_in clientAddr = {};
		int nAddrLen = sizeof(sockaddr_in);
		SOCKET _cSock = INVALID_SOCKET;
#ifdef _WIN32
		_cSock = accept(_sock, (sockaddr*)&clientAddr, &nAddrLen);
#else
		_cSock = accept(_sock, (sockaddr*)&clientAddr, (socklen_t *)&nAddrLen);
#endif
		if (INVALID_SOCKET == _cSock)
		{
			cout<<"accept client socket error"<<endl;
		}
		else
		{
			NewUserJoin userJoin;
			SendDataToAll(&userJoin);
			g_clients.push_back(_cSock);
			cout<<"has aclient connected, socket:"<<_cSock<<" ip:"<<inet_ntoa(clientAddr.sin_addr)<<endl;
		}
		return _cSock;
	}

	void Close()
	{
		if (_sock != INVALID_SOCKET)
		{
#ifdef _WIN32
			for (int n = (int)g_clients.size() - 1; n >= 0; n--)
			{
				closesocket(g_clients[n]);
			}
			//win closesocket
			closesocket(_sock);
			WSACleanup();
#else
			for (int n = (int)g_clients.size() - 1; n >= 0; n--)
			{
				close(g_clients[n]);
			}
			close(_sock);
#endif
		}
	}
	
	bool OnRun()
	{
		if (isRun())
		{
			//select io module
			fd_set fdRead;
			FD_ZERO(&fdRead);
			FD_SET(_sock, &fdRead);
			SOCKET maxSock = _sock;
			for (int n = (int)g_clients.size() - 1; n >= 0; n--)
			{
				FD_SET(g_clients[n], &fdRead);
				if (maxSock < g_clients[n])
				{
					maxSock = g_clients[n];
				}
			}
			
			timeval t = { 1,0 };
			int ret = select(maxSock + 1, &fdRead, nullptr, nullptr, &t);
			if (ret < 0)
			{
				cout<<"select error"<<endl;
				Close();
				return false;
			}
			
			if (FD_ISSET(_sock, &fdRead))
			{
				FD_CLR(_sock, &fdRead);
				Accept();
			}
			
			for (int n = (int)g_clients.size() - 1; n >= 0; n--)
			{
				if (FD_ISSET(g_clients[n], &fdRead))
				{
					if (-1 == RecvData(g_clients[n]))
					{
						std::vector<SOCKET>::iterator iter = g_clients.begin() + n;
						{
							g_clients.erase(iter);
						}
					}
				}
			}
			return true;
		}
		return false;

	}
	
	bool isRun()
	{
		return _sock != INVALID_SOCKET;
	}

	
	int RecvData(SOCKET _cSock)
	{
		
		char szRecv[4096] = {};
		//first to recv dataheader
		int nLen = (int)recv(_cSock, szRecv, sizeof(DataHeader), 0);
		DataHeader* header = (DataHeader*)szRecv;
		if (nLen <= 0)
		{
			cout<<"recv error"<<endl;
			return -1;
		}
		//second to recv real data
		recv(_cSock, szRecv + sizeof(DataHeader), header->dataLength - sizeof(DataHeader), 0);

		OnNetMsg(_cSock, header);
		return 0;
	}
	
	virtual void OnNetMsg(SOCKET _cSock, DataHeader* header)
	{
		switch (header->cmd)
		{
		case CMD_LOGIN:
		{
			
			Login* login = (Login*)header;
			cout<<"recv CMD_LOGIN"<<_cSock<<" "<<login->dataLength<<" "<<login->userName<<""<<login->PassWord;
			LoginResult ret;
			send(_cSock, (char*)&ret, sizeof(LoginResult), 0);
		}
		break;
		case CMD_LOGOUT:
		{
			Logout* logout = (Logout*)header;
			cout<<"recv CMD_LOGOUT"<<_cSock<<" "<< logout->dataLength<<" "<< logout->userName;
			LogoutResult ret;
			send(_cSock, (char*)&ret, sizeof(ret), 0);
		}
		break;
		default:
		{
			DataHeader header = { 0,CMD_ERROR };
			send(_cSock, (char*)&header, sizeof(header), 0);
		}
		break;
		}
	}

	int SendData(SOCKET _cSock, DataHeader* header)
	{
		if (isRun() && header)
		{
			return send(_cSock, (const char*)header, header->dataLength, 0);
		}
		return SOCKET_ERROR;
	}

	void SendDataToAll(DataHeader* header)
	{
		for (int n = (int)g_clients.size() - 1; n >= 0; n--)
		{
			SendData(g_clients[n], header);
		}
	}

};

#endif
