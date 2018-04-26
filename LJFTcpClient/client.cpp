#include "LJFTcpClient.hpp"
#include <thread>

void cmdThread(LJFTcpClient* client)
{
	while (true)
	{
		char cmdBuf[256] = {};
		cin >> cmdBuf;
		if (0 == strcmp(cmdBuf, "exit"))
		{
			client->Close();
			cout<<"now exit..."<<endl;
			break;
		}
		else if (0 == strcmp(cmdBuf, "login"))
		{
			Login login;
			strncpy_s(login.userName, "ljf", 4);
			strncpy_s(login.PassWord, "123456", 6);
			client->SendData(&login);

		}
		else if (0 == strcmp(cmdBuf, "logout"))
		{
			Logout logout;
			strncpy_s(logout.userName, "ljf", 3);
			client->SendData(&logout);
		}
		else {
			cout << "input error,please input login/logout/exit !!!" << endl;
		}
	}
}

int main()
{

	LJFTcpClient client1;
	client1.Connect("127.0.0.1", 8888);

	cout <<"input error,please input login/logout/exit !!!" << endl;
	//create a pthread to control the program stop
	std::thread t1(cmdThread, &client1);
	t1.detach();
	
	while (client1.isRun())
	{
		client1.OnRun();
		
		//sleep(1000);
	}
	client1.Close();

	cout << "the client is over!!!" << endl;
	return 0;
}