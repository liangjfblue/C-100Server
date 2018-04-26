#ifndef _LJFTCPCLIENT_HPP__
#define _LJFTCPCLIENT_HPP__

#ifdef _WIN32
	#define WIN32_LEAN_AND_MEAN
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
#include <stdio.h>
#include <iostream>

#include "MessageHeader.hpp"

#define _WINSOCK_DEPRECATED_NO_WARNINGS 0

using namespace std;

class LJFTcpClient
{
	SOCKET _sock;
public:
	LJFTcpClient()
	{
		_sock = INVALID_SOCKET;
	}
	
	/*virtual LJFTcpClient()
	{
		Close();
	}*/
	~LJFTcpClient()
	{
		Close();
	}

	void InitSocket()
	{
#ifdef _WIN32
		WORD ver = MAKEWORD(2, 2);
		WSADATA dat;
		WSAStartup(ver, &dat);
#endif
		if (INVALID_SOCKET != _sock)
		{
			cout << "the socket is :" << _sock << endl;
			Close();
		}
		_sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
		if (INVALID_SOCKET == _sock)
		{
			cout << "create socket error" << endl;
		}
		else {
			cout << "create socket ok, is:" << _sock << endl;
		}
	}

	int Connect(const char* ip,unsigned short port)
	{
		if (INVALID_SOCKET == _sock)
		{
			InitSocket();
		}
	
		sockaddr_in _sin = {};
		_sin.sin_family = AF_INET;
		_sin.sin_port = htons(port);
#ifdef _WIN32
		_sin.sin_addr.S_un.S_addr = inet_addr(ip);
#else
		_sin.sin_addr.s_addr = inet_addr(ip);
#endif
		cout << "scoket: "<< _sock << "ip: " <<ip << "port: " << port << endl;

		int ret = connect(_sock, (sockaddr*)&_sin, sizeof(sockaddr_in));
		if (SOCKET_ERROR == ret)
		{
			cout << "error scoket: "<< _sock << "ip: " <<ip << "port: " << port << endl;
		}
		else {
			cout << "ok scoket: "<< _sock << "ip: " <<ip << "port: " << port << endl;
		}
		return ret;
	}


	void Close()
	{
		if (_sock != INVALID_SOCKET)
		{
#ifdef _WIN32
			closesocket(_sock);
			//win socket
			WSACleanup();
#else
			close(_sock);
#endif
			_sock = INVALID_SOCKET;
		}
	}

	//start to run client
	bool OnRun()
	{
		if (isRun())
		{
			fd_set fdReads;
			FD_ZERO(&fdReads);
			FD_SET(_sock, &fdReads);
			timeval t = { 0,0 };
			int ret = select(_sock + 1, &fdReads, 0, 0, &t); 
			if (ret < 0)
			{
				cout << "socket:"<<_sock<<"connected error"<<endl;
				Close();
				return false;
			}
			if (FD_ISSET(_sock, &fdReads))
			{
				FD_CLR(_sock, &fdReads);

				if (-1 == RecvData(_sock))
				{
					cout<<"recv data error"<<"socket:"<<_sock<<endl;
					Close();
					return false;
				}
			}
			return true;
		}
		return false;
	}

	//是否工作中
	bool isRun()
	{
		return _sock != INVALID_SOCKET;
	}

	//接收数据 处理粘包 拆分包
	int RecvData(SOCKET _cSock)
	{
		char szRecv[4096] = {};
		//first to recv datahead
		int nLen = (int)recv(_cSock, szRecv, sizeof(DataHeader), 0);
		DataHeader* header = (DataHeader*)szRecv;
		if (nLen <= 0)
		{
			cout<<"socket is close between server and client"<<endl;
			return -1;
		}
		//second to recv the real data
		recv(_cSock, szRecv + sizeof(DataHeader), header->dataLength - sizeof(DataHeader), 0);

		OnNetMsg(header);
		return 0;
	}

	//deal with the msg
	void OnNetMsg(DataHeader* header)
	{
		switch (header->cmd)
		{
		case CMD_LOGIN_RESULT:
		{
			
			LoginResult* login = (LoginResult*)header;
			cout<<"<socket="<<_sock<<"recv server msg CMD_LOGIN_RESULT,data length is:"<<login->dataLength<<endl;
		}
		break;
		case CMD_LOGOUT_RESULT:
		{
			LogoutResult* logout = (LogoutResult*)header;
			cout<<"<socket="<<_sock<<"recv server msg CMD_LOGOUT_RESULT,data length is:"<<logout->dataLength<<endl;
		}
		break;
		case CMD_NEW_USER_JOIN:
		{
			NewUserJoin* userJoin = (NewUserJoin*)header;
			cout<<"<socket="<<_sock<<"recv server msg CMD_NEW_USER_JOIN,data length is:"<<userJoin->dataLength<<endl;
		}
		break;
		}
	}

	//send data
	int SendData(DataHeader* header)
	{
		if (isRun() && header)
		{
			return send(_sock, (const char*)header, header->dataLength, 0);
		}
		return SOCKET_ERROR;
	}

};

#endif