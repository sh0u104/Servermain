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
void Server::EraseClient(Client* client)
{
	int i = 0;
	for (i = 0; i < clients.size(); ++i)
	{
		if (clients.at(i)->sock == client->sock)
		{
			int r = closesocket(client->sock);
			if (r != 0)
			{
				std::cout << "Close Socket Failed." << std::endl;
			}
			client->sock = INVALID_SOCKET;
			delete client->player;
			break;
		}
	}
	clients.erase(clients.begin() + i);
}


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
	//u_long mode = 1; // �m���u���b�L���O���[�h��L���ɂ��邽�߂�1��ݒ�
	//ioctlsocket(sock, FIONBIO, &mode);

	UDPExecute();


	// �T�[�o������R�}���h���͂ŏI�������܂Ń��[�v����B
	// �L�[�{�[�h��exit����͂���ƃ��[�v�𔲂��邽�߂̕ʃX���b�h��p��
	std::thread th(&Server::Exit, this);

	// �N���C�A���g����̎�t����
	int size = sizeof(struct sockaddr_in);
	do {

		Client* newClient = new Client();
		// �ڑ��v������������
		newClient->sock = accept(sock, (struct sockaddr*)&newClient->addr, &size);
		if (newClient->sock != INVALID_SOCKET)
		{
			//�ڑ��㏈��
			newClient->player = new Player();
			newClient->player->id = 0;
			newClient->player->position = DirectX::XMFLOAT3(0, 0, 0);
			newClient->player->angle = DirectX::XMFLOAT3(0, 0, 0);

			clients.push_back(newClient);


			//PlayerLogin login;
			//login.cmd = NetworkTag::Login;
			//login.id = newClient->player->id;
			//
			//char bufferlogin[sizeof(PlayerLogin)];
			//memcpy_s(bufferlogin, sizeof(bufferlogin), &login, sizeof(PlayerLogin));
			//
			//send(newClient->sock, bufferlogin, sizeof(PlayerLogin), 0);
			//std::cout << "send login : " << login.id << "->" << newClient->player->id << std::endl;

			std::thread* recvThread = new std::thread(&Server::Recieve, this, newClient);
			recvThreads.push_back(recvThread);

		}
	} while (loop);


	th.join();
	for (std::thread* thread : recvThreads)
	{
		thread->join();
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
void Server::Recieve(Client* client)
{
	bool Loop = true;
	do {
		//UDP
		{
			char Buffer[256]{};
			int addrSize = sizeof(struct sockaddr_in);
			sockaddr_in temp;
			int size = recvfrom(uSock, Buffer, sizeof(Buffer), 0, reinterpret_cast<sockaddr*>(&temp), &addrSize);

			if (size > 0)
			{
				size = sendto(uSock, Buffer, sizeof(Buffer), 0, (struct sockaddr*)&temp, addrSize);
			}
		}

		//TCP
		{
			char buffer[2048];
			int r = recv(client->sock, buffer, sizeof(buffer), 0);
			if (r == 0)
			{
				EraseClient(client);
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
				//std::cout << "recv cmd " << type << std::endl;
				switch (static_cast<TcpTag>(type))
				{
				case TcpTag::Logout:
				{
					PlayerLogout logout;
					memcpy_s(&logout, sizeof(logout), buffer, sizeof(PlayerLogout));

					//�`�[�������o�[�ɑޏo�������M
					for (int i = 0; i < 3; ++i)
					{
						if (team[client->player->teamID].sock[i] == 0)break;

						std::cout << "�ޏo id : " << logout.id << std::endl;
						std::cout << "���M��id " << logout.id << std::endl;
						std::cout << "���M��id " << team[client->player->teamID].ID[i] << std::endl;
						int s = send(team[client->player->teamID].sock[i], buffer, sizeof(buffer), 0);

						std::cout << "send cmd " << type << std::endl;
						std::cout << "" << std::endl;
						if (team[client->player->teamID].ID[i] == logout.id)
						{
							team[client->player->teamID].sock[i] = 0;
						}
					}

					//�A�J�E���g�����I�t���C���ɂ���
					//�Q�X�g���O�C������Ȃ�������
					if (!client->Geustflag)
					{
						for (size_t i = 0; i < signData.size(); ++i)
						{
							if (signData.at(i).ID == client->player->id)
							{
								signData.at(i).onlineflag = false;
								std::cout << "Name " << signData.at(i).name << "�̓I�t���C����" << std::endl;
								break;
							}
						}
					}
					std::cout << "ID " << logout.id << " �ޏo" << std::endl;
					EraseClient(client);
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
					//�`�[�����쐬�ł�����
					bool Permission = false;
					char sendbuffer[2048];
					memcpy_s(&teamcreate, sizeof(teamcreate), buffer, sizeof(TeamCreate));

					for (int i = 0; i < TeamMax - 1; ++i)
					{
						if (team[i].TeamNumber == 0)
						{
							Permission = true;
							++teamnumbergrant;
							team[i].TeamNumber = teamnumbergrant;
							team[i].sock[0] = client->sock;
							team[i].ID[0] = client->player->id;
							client->player->teamnumber = teamnumbergrant;
							client->player->teamID = i;
							++team[i].logincount;

							teamcreate.number = teamnumbergrant;
							teamcreate.Permission = true;
							memcpy_s(sendbuffer, sizeof(sendbuffer), &teamcreate, sizeof(TeamCreate));
							int s = send(client->sock, sendbuffer, sizeof(TeamCreate), 0);

							std::cout << "ID : " << client->player->id << "���쐬�`�[��ID : " << teamcreate.number << std::endl;
							break;
						}
					}
					//�`�[�������Ȃ�������
					if (!Permission)
					{
						int s = send(client->sock, buffer, sizeof(buffer), 0);
					}
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

					for (int i = 0; i < TeamMax - 1; ++i)
					{
						if (team[i].sock[3] > 0)continue;

						if (team[i].TeamNumber == teamjoin.number)
						{
							for (int j = 0; j < 3; ++j)
							{
								//�����p�Ƀ`�[����ID�������
								teamsync.id[j] = team[i].ID[j];

								//�`�[���ɋ󂫂����邩
								if (team[i].ID[j] == 0)
								{
									//���ꂽ��
									join = true;
									team[i].sock[j] = client->sock;
									team[i].ID[j] = client->player->id;
									++team[i].logincount;

									client->player->teamnumber = team[i].TeamNumber;
									//���ꂽ�瑗��
									int s = send(client->sock, buffer, sizeof(Teamjoin), 0);
									std::cout << client->player->id << "���`�[��ID : " << teamjoin.number << " �ɉ���" << std::endl;

									//�`�[���ɓ��낤�Ƃ����l�͒N���`�[���ɂ��邩�킩��񂩂�
									teamsync.cmd = TcpTag::Teamsync;
									teamsync.id[j] = teamjoin.id;
									//������send����
									memcpy_s(&syncbuffer, sizeof(syncbuffer), &teamsync, sizeof(Teamsync));
									int se = send(client->sock, syncbuffer, sizeof(Teamsync), 0);
									break;
								}
								//�`�[���ɂ���l�ɉ����҂̏��𑗂�
								int s = send(team[i].sock[j], buffer, sizeof(buffer), 0);
								std::cout << "ID " << team[i].ID[j] << " : ��ID�@" << client->player->id << "�̉������𑗐M" << std::endl;
							}
							break;

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
				case TcpTag::Teamleave:
				{
					TeamLeave teamLeave;
					memcpy_s(&teamLeave, sizeof(teamLeave), buffer, sizeof(TeamLeave));
					//�z�X�g����������
					if (teamLeave.isLeader)
					{
						std::cout << "�z�X�g���`�[�������U���� id : " << teamLeave.id << std::endl;
					}
					else
					{
						std::cout << "�`�[�����甲���� id : " << teamLeave.id << std::endl;
					}
					{
						//�`�[�������o�[�ɑ��M
						for (int i = 0; i < 3; ++i)
						{
							if (team[client->player->teamID].ID[i] <= 0)continue;

							std::cout << "���M��id " << team[client->player->teamID].ID[i] << std::endl;
							int s = send(team[client->player->teamID].sock[i], buffer, sizeof(buffer), 0);

							std::cout << "send cmd " << type << std::endl;
							std::cout << "" << std::endl;
							if (team[client->player->teamID].ID[i] == teamLeave.id)
							{
								team[client->player->teamID].sock[i] = 0;
								team[client->player->teamID].ID[i] = 0;
								team[client->player->teamID].logincount -= 1;
								team[client->player->teamID].check[i] = false;
							}
						}
					}
				}
				break;
				case TcpTag::StartCheck:
				{
					StartCheck startcheck;
					memcpy_s(&startcheck, sizeof(startcheck), buffer, sizeof(StartCheck));
					for (int i = 0; i < TeamMax - 1; ++i)
					{
						if (team[i].TeamNumber == startcheck.teamnunber)
						{
							for (int j = 0; j < 3; ++j)
							{
								if (team[i].sock[j] == client->sock)
								{
									team[i].check[j] = startcheck.check;

									int s = send(client->sock, buffer, sizeof(StartCheck), 0);
									if (startcheck.check)
									{
										std::cout << team[i].TeamNumber << "��ID�@:�@" << client->player->id << "����OK" << std::endl;
									}
									else
									{
										std::cout << team[i].TeamNumber << "��ID�@:�@" << client->player->id << "������" << std::endl;
									}
									break;
								}
							}
							break;
						}
					}

				}
				break;
				case TcpTag::Gamestart:
				{
					GameStart gamestart;
					int sendcount = 0;
					memcpy_s(&gamestart, sizeof(gamestart), buffer, sizeof(GameStart));
					for (int i = 0; i < TeamMax - 1; ++i)
					{
						if (team[i].TeamNumber != gamestart.teamnunber)continue;
						if (team[i].ID[0] != gamestart.id)continue;
						//�����`�[���ԍ�������������
						//�S���������ł��Ă��邩
						for (int j = 1; j < team[i].logincount; ++j)
						{
							if (team[i].ID[j] == 0)  break;
							if (team[i].check[j] != true)break;
							++sendcount;
						}
						//���O�C�����Ə���OK�ɐ��������ΑS���ɑ��M
						if (sendcount == team[i].logincount - 1)
						{
							for (int count = sendcount; -1 < count; --count)
							{
								int s = send(team[i].sock[count], buffer, sizeof(buffer), 0);
							}
						}
					}
				}
				break;
				case TcpTag::GameEnd:
				{
					GameEnd gameend;
					memcpy_s(&gameend, sizeof(GameEnd), buffer, sizeof(GameEnd));

					for (int i = 0; i < TeamMax - 1; ++i)
					{
						if (team[i].sock[0] == client->sock)
						{
							//�`�[���S���̏����`�F�b�N���O��
							std::fill(team[i].check, team[i].check + 4, false);
							break;
						}
					}
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
				//case NetworkTag::SignIn:
				//{
				//	SignIn signIn;
				//	SignData signInData;
				//	bool signInflag = false;
				//	memcpy_s(&signIn, sizeof(SignIn), buffer, sizeof(SignIn));
				//	strcpy_s(signInData.name, signIn.name);
				//	strcpy_s(signInData.pass, signIn.pass);
				//	for (size_t i = 0; i < signData.size(); ++i)
				//	{
				//		//�T�[�o�[���O�C���҃f�[�^�������
				//		if (std::strcmp(signData.at(i).name, signIn.name) == 0)
				//		{
				//			if (std::strcmp(signData.at(i).pass, signIn.pass) != 0)continue;
				//			signInflag = true;
				//			client->player->id = signData.at(i).ID;
				//			signData.at(i).sock = client->sock;
				//			signData.at(i).onlineflag = true;
				//			Login(client->sock, client->player->id);
				//			signIn.result = true;
				//			memcpy_s(buffer, sizeof(SignIn), &signIn, sizeof(SignIn));
				//			//�T�C���C�����𑗂�Ԃ�
				//			std::cout << "Name " << signIn.name << "���T�C���C��" << std::endl;
				//			std::cout << "Name " << signIn.name << "�̓I�����C����" << std::endl;
				//			int s = send(client->sock, buffer, sizeof(buffer), 0);
				//			//�ۑ�����ăt�����h���N�G�X�g��S�đ��M
				//			for (int j = 0; j < signData.at(i).friendSenderSaveData.size(); ++j)
				//			{
				//				FriendRequest friendRequest;
				//				friendRequest.cmd = NetworkTag::FriendRequest;
				//				strcpy_s(friendRequest.name, signData.at(i).friendSenderSaveData.at(j).name);
				//				friendRequest.senderid = signData.at(i).friendSenderSaveData.at(j).ID;
				//memcpy_s(buffer, sizeof(FriendRequest), &friendRequest, sizeof(FriendRequest));
				//				int s = send(client->sock, buffer, sizeof(buffer), 0);
				//			}
				//			break;
				//		}
				//	}
				//	//�T�[�o�[�Ƀ��O�C���ҏ�񂪂Ȃ���
				//	if (!signInflag)
				//	{
				//		signIn.result = false;
				//		memcpy_s(buffer, sizeof(SignIn), &signIn, sizeof(SignIn));
				//		std::cout << "�T�C���C�����s" << std::endl;
				//		int s = send(client->sock, buffer, sizeof(buffer), 0);
				//	}
				//}
				//break;
				//case NetworkTag::SignUp:
				//{
				//	++this->id;
				//	client->player->id = this->id;
				//	Login(client->sock, this->id);
				//	SignUp signUp;
				//	SignData signInData;
				//	memcpy_s(&signUp, sizeof(SignUp), buffer, sizeof(SignUp));
				//	strcpy_s(signInData.name, signUp.name);
				//	strcpy_s(signInData.pass, signUp.pass);
				//	signInData.ID = client->player->id;
				//	signInData.sock = client->sock;
				//	signInData.onlineflag = true;
				//	signData.push_back(signInData);
				//	int s = send(client->sock, buffer, sizeof(buffer), 0);
				//	std::cout << "Name " << signUp.name << "���T�C���A�b�v �p�X��" << signUp.pass << std::endl;
				//	std::cout << signUp.name << "�̓I�����C����" << std::endl;
				//}
				//break;
				case TcpTag::Move:
				{
					PlayerInput input;
					memcpy_s(&input, sizeof(input), buffer, sizeof(PlayerInput));
					client->player->velocity = input.velocity;
					client->player->position = input.position;
					client->player->state = input.state;
					client->player->angle = input.angle;

					for (int i = 0; i < 3; ++i)
					{
						if (team[client->player->teamID].sock[i] <= 0)continue;

						//std::cout << "���M��id " << client->player->id << std::endl;
						//std::cout << "���M��id " << Client->player->id << std::endl;
						int s = send(team[client->player->teamID].sock[i], buffer, sizeof(buffer), 0);

						//client->player->position = input.position;
						//std::cout << "velocity y: " << input.velocity.y << std::endl;
						//std::cout << "state " << int(input.state) << std::endl;
						//std::cout << "" << std::endl;

					}
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
				//case TcpTag::Attack:
				//{
				//	PlayerInput input;
				//	memcpy_s(&input, sizeof(input), buffer, sizeof(PlayerInput));
				//	client->player->velocity = input.velocity;
				//	for (Client* Client : clients)
				//	{
				//		std::cout << "���M��id " << client->player->id << std::endl;
				//		std::cout << "���M��id " << Client->player->id << std::endl;
				//		int s = send(Client->sock, buffer, sizeof(buffer), 0);
				//		//client->player->position = input.position;
				//		std::cout << "send cmd " << type << std::endl;
				//		std::cout << "" << std::endl;
				//	}
				//}
				//break;
				}
			}

		}

		



	} while (Loop);
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