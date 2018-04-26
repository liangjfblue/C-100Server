#include "LJFTcpServer.hpp"

int main()
{

	LJFTcpServer server;
	server.InitSocket();
	server.Bind(nullptr, 8888);
	server.Listen(5);

	while (server.isRun())
	{
		server.OnRun();
	}
	server.Close();
	cout << "the server is over!!!" << endl;
	return 0;
}