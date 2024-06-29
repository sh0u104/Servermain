#include <iostream>
#include <thread>
#include <string>
#include <iostream>

#include "Server.h"

enum
{
	Login  = 0,
};

int main(void)
{
	bool loop = true;
	do
	{
		
		int input = 0;
		//std::cin >> input;
		switch (input)
		{
		case Login:
		{
			Server server = Server();
			server.Execute();
		}
		break;
		}
	} while (loop);
	std::cout << "ServerÅ‹N“®" << std::endl;
}