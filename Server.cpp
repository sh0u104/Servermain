#include "Server.h"

#include <stdio.h>
#include <WinSock2.h>
#include <iostream>

#pragma comment(lib, "ws2_32.lib")
// �T�[�o���R�}���h���̓X���b�h
void Server::Exit()
{
	while (loop) {
		std::string input;
		std::cin >> input;
		if (input == "exit")
		{
			loop = false;
			std::cout << "�T�[�o�[����" << std::endl;

		}
	}
}
// �N���C�A���g�폜�֐�
//void Server::EraseClient(Client* client)
//{
//	int i = 0;
//	for (i = 0; i < clients.size(); ++i)
//	{
//		if (clients.at(i)->sock == client->sock)
//		{
//			int r = closesocket(client->sock);
//			if (r != 0)
//			{
//				std::cout << "Close Socket Failed." << std::endl;
//			}
//			client->sock = INVALID_SOCKET;
//			delete client->player;
//			break;
//		}
//	}
//	clients.erase(clients.begin() + i);
//}


void Server::UDPExecute()
{
	uAddr.sin_family = AF_INET;
	uAddr.sin_port = htons(8000);
	uAddr.sin_addr.S_un.S_addr = INADDR_ANY;

	uSock = socket(AF_INET, SOCK_DGRAM, 0);
	if (uSock == INVALID_SOCKET) {
		std::cout << "�\�P�b�g�̐����Ɏ��s���܂���" << std::endl;
		// 9.WSA�̉��
		WSACleanup();
	}

	// bind
	bind(uSock, (struct sockaddr*)&uAddr, sizeof(uAddr));

	//// �m���u���b�L���O�̐ݒ�
	u_long mode = 1; // �m���u���b�L���O���[�h��L���ɂ��邽�߂�1��ݒ�
	ioctlsocket(uSock, FIONBIO, &mode);
}


void Server::Execute()
{
	std::cout << "Server�N��" << std::endl;
	std::cout << "�ڑ������Ȃ�ID 0" << std::endl;
	// Server�ۑ� �T�[�o���ݒ�
	// WinsockAPI��������
	WSADATA wsaData;

	// �o�[�W�������w�肷��ꍇMAKEWORD�}�N���֐����g�p����
	int wsaStartUp = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (wsaStartUp != 0)
	{
		// ���������s 
		std::cout << "WSA Initialize Failed." << std::endl;
		return;
	}

	// �T�[�o�̎�t�ݒ�
	struct sockaddr_in addr;
	addr.sin_family = AF_INET;
	addr.sin_port = htons(7000);
	addr.sin_addr.S_un.S_addr = INADDR_ANY;//"0.0.0.0"

	// �\�P�b�g�̍쐬
	SOCKET sock = socket(AF_INET, SOCK_STREAM, 0);
	if (sock == INVALID_SOCKET) {
		std::cout << "Create Socket Failed." << std::endl;
		return;
	}

	// �\�P�b�g�Ǝ�t����R�Â���
	if (bind(sock, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) != 0)
	{
		std::cout << "Bind Failed." << std::endl;
		return;
	}
	
	// ��t���J�n����
	int backlog = 10;
	int Listen = listen(sock, backlog);
	

	//// �m���u���b�L���O�̐ݒ�
	u_long mode = 1; // �m���u���b�L���O���[�h��L���ɂ��邽�߂�1��ݒ�
	ioctlsocket(sock, FIONBIO, &mode);

	UDPExecute();


	// �T�[�o������R�}���h���͂ŏI�������܂Ń��[�v����B
	// �L�[�{�[�h��exit����͂���ƃ��[�v�𔲂��邽�߂̕ʃX���b�h��p��
	std::thread th(&Server::Exit, this);

	// �N���C�A���g����̎�t����
	int size = sizeof(struct sockaddr_in);
	do {

		std::shared_ptr<Client> newClient = std::make_shared<Client>();
		//Client* newClient = new Client();
		// �ڑ��v������������
		newClient->sock = accept(sock, (struct sockaddr*)&newClient->addr, &size);
		if (newClient->sock != INVALID_SOCKET)
		{
			//�ڑ��㏈��
			newClient->player = std::make_shared<Player>();
			newClient->player->id = 0;
			newClient->player->position = DirectX::XMFLOAT3(0, 0, 0);
			newClient->player->angle = DirectX::XMFLOAT3(0, 0, 0);

			clients.emplace_back(newClient);

			//std::thread* recvThread = new std::thread(&Server::Recieve, this, newClient);
			//recvThreads.push_back(recvThread);
			recvThreads.push_back(std::make_shared<std::thread>(&Server::Recieve, this, newClient));

		}
	} while (loop);


	th.join();
	//for (std::thread* thread : recvThreads)
	//{
	//	thread->join();
	//}
	 // �X���b�h�̏I����ҋ@���Ă�����
	for (auto& thread : recvThreads) {
		if (thread->joinable()) {
			thread->join();
		}
	}

	// �N���C�A���g�̃\�P�b�g�폜
	for (int i = 0; i < clients.size(); ++i)
	{
		closesocket(clients.at(i)->sock);
	}

	// ��t�\�P�b�g�̐ؒf
	closesocket(sock);

	// WSA�I��
	int wsaCleanup = WSACleanup();

}

// ��M�֐�
void Server::Recieve(std::shared_ptr<Client> client)
{
	bool Loop = true;
	std::cout  << "Loop�J�n" << std::endl;
	do {
		//UDP
		{
			char Buffer[256]{};
			int addrSize = sizeof(sockaddr);
			sockaddr_in temp;
			int size = recvfrom(uSock, Buffer, sizeof(Buffer), 0, reinterpret_cast<sockaddr*>(&temp), &addrSize);
			

			if (size == -1)
			{
				memset(Buffer, 0, sizeof(Buffer));
				int error = WSAGetLastError();
				if (error == WSAEWOULDBLOCK) {
					// �f�[�^���܂����Ă��Ȃ��ꍇ�A�������X�L�b�v���Ď��̃��[�v��
					//if (client->uAddr.sin_port == temp.sin_port)
					//continue;
				}
				else {
					// ���̃G���[�̏ꍇ�A���[�v���I��
					if (error == WSAECONNRESET&& client->uAddr.sin_port == temp.sin_port)
					{
						std::cout << client->player->id << "�͐ڑ����r�؂ꂽ" << std::endl;
						Loop = false;
					}
					else
					{
						std::cout << client->player->id << " recv error" << error << std::endl;
					}
				}
			}
		    if (size == 0)
			{
				// �N���C�A���g���ڑ�������ꍇ�̏���
				std::cout << "�ڑ������" << std::endl;
				Loop = false;
				break;
			}
			

			if (size > 0)
			{
				short type = 0;
				memcpy_s(&type, sizeof(type), Buffer, sizeof(short));
			    //clients�ɓo�^����Ă邩�ǂ���
				if (HasSameData(clients, temp))
				{
					//�`�[���ɒN�����邩
					if (client->team)
					//if (client->team->clients.size() > 0 && client->player->id != 0)
					//�`�[����clients���ǂ���
					if (HasSameData(client->team->clients, temp))
					{
						switch (static_cast<UdpTag>(type))
						{
						case UdpTag::Move:
						{
							//�����Ȃ��΂�
							if (temp.sin_port == client->uAddr.sin_port)
								break;
							size = sendto(uSock, Buffer, sizeof(Buffer), 0, (struct sockaddr*)(&client->uAddr), addrSize);
							
						}
						break;
						//�z�X�g�̃G�l�~�[�����`�[�������o�[�ɑ���p
						case UdpTag::EnemyMove:
						{
							//�����Ȃ��΂�
							if (temp.sin_port == client->uAddr.sin_port)
								break;
							size = sendto(uSock, Buffer, sizeof(Buffer), 0, (struct sockaddr*)(&client->uAddr), addrSize);
							
						}
						break;
						}
					}
				}
				//clients�ɓo�^����ĂȂ�������
				else
				{
					switch (static_cast<UdpTag>(type))
					//�S����UDP�A�h���X��ۑ�����
					case UdpTag::UdpAddr:
					{
						if (!client->isRecvUdpAddr)
						{
							//UDP�̃A�h���X��ۑ���
							client->uAddr = temp;
							client->isRecvUdpAddr = true;
							//�ڑ��҂�0�������i��ڑ��҂�-1�j
							client->player->id = 0;

							//�ۑ��������TCP�ŕԂ�
							std::cout << "UDP�A�h���X��ۑ�����" << std::endl;
							
							SendUdpAddr sendUdpAddr;
							sendUdpAddr.cmd = TcpTag::UdpAddr;

							char buffer[sizeof(SendUdpAddr)];
							memcpy_s(buffer, sizeof(SendUdpAddr), &sendUdpAddr, sizeof(SendUdpAddr));
							int s = send(client->sock, buffer, sizeof(buffer), 0);
						}
					break;
					}
				}
			}
		}

		//TCP
		{
			char buffer[2048];
			int r = recv(client->sock, buffer, sizeof(buffer), 0);
			
			if (r == -1) {
				int error = WSAGetLastError();
				if (error == WSAEWOULDBLOCK) {
					// �f�[�^���܂����Ă��Ȃ��ꍇ�A�������X�L�b�v���Ď��̃��[�v��
				}
				else {
					// ���̃G���[�̏ꍇ�A���[�v���I��
					if (error == WSAECONNRESET)
					{
						std::cout << client->player->id << "�͐ڑ����r�؂ꂽ" << std::endl;
						Loop = false;
					}
					else
					{
						std::cout << client->player->id << " recv error" << error << std::endl;
					}
					
				}
			}
			if (r == 0)
			{
				Loop = false;
				if (clients.size() <= 0)
				{
					std::cout << "�T�[�o�[����" << std::endl;
				}
			}

			if (r > 0)
			{
				short type = 0;
				memcpy_s(&type, sizeof(type), buffer, sizeof(short));
				switch (static_cast<TcpTag>(type))
				{
				case TcpTag::Logout:
				{
					PlayerLogout logout;
					memcpy_s(&logout, sizeof(logout), buffer, sizeof(PlayerLogout));

					std::cout << "ID " << logout.id << " �ޏo" << std::endl;
					Loop = false;
					if (clients.size() <= 0)
					{
						std::cout << "�T�[�o�[����Ȃ�" << "���͂��ā@ exit" << std::endl;
					}
				}
				break;
				case TcpTag::TeamCreate:
				{
					TeamCreate teamcreate;
					char sendbuffer[2048];
					memcpy_s(&teamcreate, sizeof(teamcreate), buffer, sizeof(TeamCreate));

					auto team = std::make_shared<Team>();
					++teamnumbergrant;
					
					team->isJoin = true;
					team->TeamNumber = teamnumbergrant;
					client->player->teamnumber = teamnumbergrant;
					team->clients.emplace_back(client);
					client->team = team.get();
					teams.emplace_back(team);

					std::cout << "ID : " << client->player->id << "���`�[���쐬 ID : " << team->TeamNumber << std::endl;
					std::cout << "�`�[���ԍ� " << team->TeamNumber << " �����\" << std::endl;
					teamcreate.number = teamnumbergrant;
					teamcreate.Permission = true;
					memcpy_s(sendbuffer, sizeof(sendbuffer), &teamcreate, sizeof(TeamCreate));
					int s = send(client->sock, sendbuffer, sizeof(TeamCreate), 0);
				}
				break;
				case TcpTag::Teamjoin:
				{
					Teamjoin teamjoin;
					Teamsync teamsync;

					char syncbuffer[sizeof(Teamsync)];
					char failurebuffer[sizeof(Teamjoin)];
					//�`�[���ɉ����ł�����
					bool join = false;
					memcpy_s(&teamjoin, sizeof(teamjoin), buffer, sizeof(Teamjoin));

					//�Q�X�g����Ȃ����ׂ̈�
					client->player->id = teamjoin.id;

					for (int i = 0; i < teams.size(); ++i)
					{
						//�`�[�������ԍ��ƈ�v���Ȃ����
						if (teams.at(i)->TeamNumber != teamjoin.number)continue;
						//�`�[���̑S�����܂��Ă�����
						if (teams.at(i)->clients.size() >= 4)continue;
						//�`�[���ɓ����^�C�~���O���ǂ����H
						if (teams.at(i)->isJoin)
						{
							join = true;
							client->player->teamnumber = teamjoin.number;
	
							teams.at(i)->clients.emplace_back(client);
							client->team = teams.at(i).get();
							//���ꂽ�瑗��
							int s = send(client->sock, buffer, sizeof(Teamjoin), 0);
							std::cout << client->player->id << "���`�[��ID : " << teamjoin.number << " �ɉ���" << std::endl;

							//�`�[���ɓ��낤�Ƃ����l�͒N���`�[���ɂ��邩�킩��񂩂�
							teamsync.cmd = TcpTag::Teamsync;
							//�`�[���ɂ���l��
							for (int j = 0; j < teams.at(i)->clients.size(); ++j)
							{
								teamsync.id[j] = teams.at(i)->clients.at(j)->player->id;
								//�{�l�ɈȊO�ɑ��邽��
								if (teamjoin.id != teamsync.id[j])
								{
									//�`�[���ɂ���l�ɉ����҂̏��𑗂�
									int s = send(teams.at(i)->clients.at(j)->sock, buffer, sizeof(buffer), 0);
									std::cout << "ID " << teams.at(i)->clients.at(j)->player->id << " : ��ID�@" << client->player->id << "�̉������𑗐M" << std::endl;
								}
							}
							//�Q���Җ{�l�ɂ��łɂ���l�̏����擾����
							memcpy_s(&syncbuffer, sizeof(syncbuffer), &teamsync, sizeof(Teamsync));
							int se = send(client->sock, syncbuffer, sizeof(Teamsync), 0);

							std::cout << "�`�[�����";
							for (int i = 0; i < client->team->clients.size(); ++i)
							{
								std::cout << client->team->clients.at(i)->player->id << ",";
							}
							std::cout << std::endl;
						}
					}

					//�`�[���ɉ����o���Ȃ�������
					if (!join)
					{
						teamjoin.number = -1;
						memcpy_s(&failurebuffer, sizeof(failurebuffer), &teamjoin, sizeof(Teamjoin));
						std::cout << "ID : " << teamjoin.id << "���`�[���ɉ������s ��M�`�[���ԍ�" << teamjoin.number << std::endl;
						int s = send(client->sock, failurebuffer, sizeof(failurebuffer), 0);
					}
				}
				break;
				//�`�[�����甲������
				case TcpTag::Teamleave:
				{
					TeamLeave teamLeave;
					memcpy_s(&teamLeave, sizeof(teamLeave), buffer, sizeof(TeamLeave));

					//�`�[�������o�[�ɑ��M
					for (int i = 0; i < client->team->clients.size(); ++i)
					{
						std::cout << "���M��id " << client->team->clients.at(i)->player->id << std::endl;
						int s = send(client->team->clients.at(i).get()->sock, buffer, sizeof(buffer), 0);

						std::cout << "ID " << teamLeave.id << "�̓`�[�����甲���� " << std::endl;
					}

					//�z�X�g����������
					if (teamLeave.isHost)
					{
						//�`�[���ɓ���Ȃ�����
						client->team->isJoin = false;
						std::cout <<"ID " << teamLeave.id << "�̃z�X�g���`�[�������U�����`�[���ԍ�" << client->team->TeamNumber <<std::endl;
						for (int i = 0; i < client->team->clients.size(); ++i)
						{
							if (client != client->team->clients.at(i))
							RemoveClientFromTeam(client->team->clients.at(i), teams);
						}
					}
					else
					{
						std::cout << "ID" << teamLeave.id  << "�@���`�[�����甲�����`�[���ԍ� " << client->team->TeamNumber << std::endl;
					}
					RemoveClientFromTeam(client, teams);

					

				}
				break;
				case TcpTag::StartCheck:
				{
					StartCheck startcheck;
					memcpy_s(&startcheck, sizeof(startcheck), buffer, sizeof(StartCheck));

					client->startCheck= startcheck.check;
					int s = send(client->sock, buffer, sizeof(StartCheck), 0);

					if (startcheck.check)
					{
						std::cout << client->team->TeamNumber << "��ID�@:�@" << client->player->id << "����OK" << std::endl;
					}
					else
					{
						std::cout << client->team->TeamNumber << "��ID�@:�@" << client->player->id << "������" << std::endl;
					}
				}
				break;
				case TcpTag::Gamestart:
				{
					GameStart gamestart;
					
					memcpy_s(&gamestart, sizeof(gamestart), buffer, sizeof(GameStart));

					//�`�[����g��ł邩�ǂ���
					if (gamestart.teamnunber == 0)
					{
						//�g��łȂ��������̂܂ܕԂ�
						send(client->sock, buffer, sizeof(buffer), 0);
						std::cout << "�\���ŃQ�[���X�^�[�gID "<< client->player->id <<  std::endl;
						break;
					}
					else
					{
						//�Q�[�����̓`�[���ɎQ���ł��Ȃ�����
						client->team->isJoin = false;

						std::cout << "�`�[���ŃQ�[���X�^�[�g�`�[�� " << std::endl;
						std::cout << "�`�[���ԍ� " << client->team->TeamNumber << " �����s��" << std::endl;
						//�z�X�g���炩�ǂ���
						//if (team[teamGrantID].clients.at(0)->player->id != gamestart.id)continue;

						bool isStartCheck = true;
						//�`�[���S���������ł��Ă��邩
						for (int i = 1; i < client->team->clients.size(); ++i)
						{
							//�`�[�������o�[�ɏ����o���ĂȂ��l��������
							if (client->team->clients.at(i)->startCheck != true)
							{
								isStartCheck = false;
								break;
							}
						}

						//�S���̏������ł��Ă�����
						if (isStartCheck)
						{
							for (int i = 0; i< client->team->clients.size(); ++i)
							{
								int s = send(client->team->clients.at(i)->sock, buffer, sizeof(buffer), 0);

								std::cout << "�Q�[���X�^�[�g���M "<< client->team->clients.at(i)->player->id<< std::endl;
								//std::cout<<"ID : " << client->team->clients.at(i)->player->id << "uaddr "<< client->team->clients.at(i)->uAddr.sin_addr.S_un.S_addr <<
								//"uport"<< client->team->clients.at(i)->uAddr.sin_port << std::endl;
							}
							std::cout << "TeamNUmber : " << client->team->TeamNumber << std::endl;
						}
					}
				}
				break;
				case TcpTag::GameEnd:
				{
					
					GameEnd gameend;
					memcpy_s(&gameend, sizeof(GameEnd), buffer, sizeof(GameEnd));

					int teamGrantID = client->player->teamGrantID;
					//�z�X�g���炩�ǂ���
					if (client->team->clients.at(0)->sock == client->sock)
					{
						
						for (int i = 0; i < client->team->clients.size(); ++i)
						{ 
							//�`�[���S���̏����`�F�b�N���O��
							client->team->clients.at(i)->startCheck = false;
						}
					}
					//�`�[���ɂ͂����悤�ɂ���
					client->team->isJoin = true;
					std::cout	<<"�`�[���ԍ� " << client->team->TeamNumber << " �����\" << std::endl;

				}
				break;
				case TcpTag::GeustLogin:
				{
					++this->id;
					client->player->id = this->id;
					std::cout << "send login : " << this->id << "->" << client->player->id << std::endl;
					client->Geustflag = true;

					Login(client->sock, this->id);
				}
				break;
				case TcpTag::EnemyDamage:
				{
					//�`�[���̃z�X�g�ɓG�̃_���[�W����𑗂�
					int s = send(client->team->clients.at(0)->sock, buffer, sizeof(buffer), 0);

				}
				break;
				//case TcpTag::Message:
				//{
				//	Message massage;
				//	std::cout << "Message " << std::endl;
				//	memcpy(&massage, buffer, sizeof(Message));
				//	std::cout << massage.text << std::endl;
				//	for (int i = 0; i < TeamMax - 1; ++i)
				//	{
				//		if (client->player->teamnumber != team[i].TeamNumber)continue;
				//		for (int j = 0; j < 3; ++j)
				//		{
				//			if (team[i].sock[j] <= 0)continue;
				//			//std::cout << "���M��id " << client->player->id << std::endl;
				//			//std::cout << "���M��id " << Client->player->id << std::endl;
				//			int s = send(team[i].sock[j], buffer, sizeof(buffer), 0);
				//		}
				//	}
				//}
				//break;
				}
			}

		}
	} while (Loop);
	std::cout << "ID " << client->player->id <<"��";
	std::cout << "Loop��~" << std::endl;
	EraseClient(client);
	std::cout << "�`�[���� "<<teams.size() << std::endl;
	std::cout << "�N���C�A���g�� "<<clients.size() << std::endl;
}




void Server::EraseClient(std::shared_ptr<Client> client)
{
	// �N���C�A���g������
	auto it = std::find(clients.begin(), clients.end(), client);
	if (it != clients.end()) {
		// �\�P�b�g�����
		int r = closesocket(client->sock);
		if (r != 0) {
			std::cout << "�\�P�b�g�������s" << std::endl;
		}
		client->sock = INVALID_SOCKET;

		// �N���C�A���g�����X�g����폜
		clients.erase(it);
		std::cout << "��������" << std::endl;
	}
	else {
		std::cout << "�������s" << std::endl;
	}
}

void Server::Login(SOCKET clientsock, short ID)
{
	PlayerLogin login;
	login.cmd = TcpTag::Login;
	login.id = ID;
	
	char bufferlogin[sizeof(PlayerLogin)];
	memcpy_s(bufferlogin, sizeof(bufferlogin), &login, sizeof(PlayerLogin));
	send(clientsock, bufferlogin, sizeof(PlayerLogin), 0);

}

bool Server::HasSameData(const std::vector<std::shared_ptr<Client>>& vec, const sockaddr_in& target)
{
	for (const auto& client : vec)
	{
		if (client->uAddr.sin_addr.S_un.S_addr == target.sin_addr.S_un.S_addr &&
			client->uAddr.sin_port == target.sin_port)
		{
			return true;
		}
	}
	return false;
}

void Server::RemoveClientFromTeam(std::shared_ptr<Client> client, std::vector<std::shared_ptr<Team>>& teams)
{
	if (client->team) {
		// `client->team`���w���`�[��������
		auto it = std::find_if(teams.begin(), teams.end(),
			[&client](const std::shared_ptr<Team>& team) {
				return team.get() == client->team;
			});

		if (it != teams.end()) {
			// �`�[����`clients`����Y������`client`���폜
			auto& clients = (*it)->clients;
			clients.erase(std::remove(clients.begin(), clients.end(), client), clients.end());

			// �`�[������ɂȂ�����폜����ꍇ
			if (clients.empty()) {
				std::cout << "Team ID " << (*it)->TeamNumber << "����"<<std::endl;
				teams.erase(it);
			}

			// �N���C�A���g�̏����`�[�������Z�b�g
			client->team = nullptr;
		}
	}
}

