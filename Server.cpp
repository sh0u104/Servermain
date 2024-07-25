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
	SOCKET sock;
	sock = socket(AF_INET, SOCK_STREAM, 0);
	if (sock == INVALID_SOCKET) {
		std::cout << "Create Socket Failed." << std::endl;
		return;
	}

	// �\�P�b�g�Ǝ�t����R�Â���
	int Bind = bind(sock, reinterpret_cast<struct sockaddr*>(&addr), sizeof(addr));


	//// �m���u���b�L���O�̐ݒ�
	//u_long mode = 1; // �m���u���b�L���O���[�h��L���ɂ��邽�߂�1��ݒ�
	//ioctlsocket(sock, FIONBIO, &mode);

	// ��t���J�n����
	int backlog = 10;
	int Listen = listen(sock, backlog);


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
			//std::cout << "Connect Success" << std::endl;
			int addr[4];
			addr[0] = newClient->addr.sin_addr.S_un.S_un_b.s_b1;
			addr[1] = newClient->addr.sin_addr.S_un.S_un_b.s_b2;
			addr[2] = newClient->addr.sin_addr.S_un.S_un_b.s_b3;
			addr[3] = newClient->addr.sin_addr.S_un.S_un_b.s_b4;
			//std::cout << "IPAddress" << addr[0] << addr[1] << addr[2] << addr[3] << std::endl;

			//++this->id;
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

	Execute();
}

// ��M�֐�
void Server::Recieve(Client* client)
{
	bool Loop = true;
	do {
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
			switch (static_cast<NetworkTag>(type))
			{
			case NetworkTag::Message:
			{
				Message massage;
				std::cout << "Message " << std::endl;
				memcpy(&massage, buffer, sizeof(Message));
			    std::cout << massage.text << std::endl;

				for (int i = 0; i < TeamMax - 1; ++i)
				{
					if (client->player->teamnumber != team[i].TeamNumber)continue;
					for (int j = 0; j < 3; ++j)
					{
						if (team[i].sock[j] <= 0)continue;

						//std::cout << "���M��id " << client->player->id << std::endl;
						//std::cout << "���M��id " << Client->player->id << std::endl;
						int s = send(team[i].sock[j], buffer, sizeof(buffer), 0);
					}
				}
			}
			break;
			case NetworkTag::Move:
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
			case NetworkTag::Attack:
			{
				PlayerInput input;
				memcpy_s(&input, sizeof(input), buffer, sizeof(PlayerInput));
				client->player->velocity = input.velocity;


				for (Client* Client : clients)
				{
					std::cout << "���M��id " << client->player->id << std::endl;
					std::cout << "���M��id " << Client->player->id << std::endl;
					int s = send(Client->sock, buffer, sizeof(buffer), 0);
					//client->player->position = input.position;
					std::cout << "send cmd " << type << std::endl;
					std::cout << "" << std::endl;

				}
			}
			break;
			case NetworkTag::Logout:
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
				std::cout<<"ID " << logout.id << " �ޏo" << std::endl;
				EraseClient(client);
				Loop = false;
				if (clients.size() <= 0)
				{
					std::cout << "�T�[�o�[����Ȃ�" << "���͂��ā@ exit" << std::endl;
				}
			}
			break;
			case NetworkTag::TeamCreate:
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
			case NetworkTag::Teamjoin:
			{
				Teamjoin teamjoin;
				Teamsync teamsync;

				char syncbuffer[sizeof(Teamsync)];
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
								teamsync.cmd = NetworkTag::Teamsync;
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
			}
			break;
			case NetworkTag::StartCheck:
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
									std::cout << team[i].TeamNumber<<"��ID�@:�@" << client->player->id << "����OK" << std::endl;
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
			case NetworkTag::Gamestart:
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
					if (sendcount == team[i].logincount-1)
					{
						for (int count = sendcount; -1 < count; --count)
						{
							int s = send(team[i].sock[count], buffer, sizeof(buffer), 0);
						}
					}
				}
			}
			break;
			case NetworkTag::GameEnd:
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
			case NetworkTag::GeustLogin:
			{
				++this->id;
				client->player->id = this->id;
	            std::cout << "send login : " << this->id << "->" << client->player->id << std::endl;
				client->Geustflag = true;

				Login(client->sock, this->id);
			}
			break;
			case NetworkTag::SignIn:
			{
				SignIn signIn;
				SignData signInData;

				bool signInflag = false;

				memcpy_s(&signIn, sizeof(SignIn), buffer, sizeof(SignIn));

				strcpy_s(signInData.name, signIn.name);
				strcpy_s(signInData.pass, signIn.pass);
				
				for (size_t i = 0; i < signData.size(); ++i)
				{
					//�T�[�o�[���O�C���҃f�[�^�������
					if (std::strcmp(signData.at(i).name, signIn.name) == 0)
					{
						if (std::strcmp(signData.at(i).pass, signIn.pass) != 0)continue;
						signInflag = true;

						client->player->id = signData.at(i).ID;
						signData.at(i).sock = client->sock;
						signData.at(i).onlineflag = true;

						
						Login(client->sock, client->player->id);

						signIn.result = true;
						memcpy_s(buffer, sizeof(SignIn), &signIn, sizeof(SignIn));

						//�T�C���C�����𑗂�Ԃ�
						std::cout << "Name " << signIn.name << "���T�C���C��" << std::endl;
						std::cout << "Name " << signIn.name << "�̓I�����C����" << std::endl;
						int s = send(client->sock, buffer, sizeof(buffer), 0);

						//�ۑ�����ăt�����h���N�G�X�g��S�đ��M
						for (int j = 0; j < signData.at(i).friendSenderSaveData.size(); ++j)
						{
							FriendRequest friendRequest;
							friendRequest.cmd = NetworkTag::FriendRequest;
							strcpy_s(friendRequest.name, signData.at(i).friendSenderSaveData.at(j).name);
							friendRequest.senderid = signData.at(i).friendSenderSaveData.at(j).ID;
		
							memcpy_s(buffer, sizeof(FriendRequest), &friendRequest, sizeof(FriendRequest));

							int s = send(client->sock, buffer, sizeof(buffer), 0);
						}
					
						break;
					}
				}
				//�T�[�o�[�Ƀ��O�C���ҏ�񂪂Ȃ���
				if(!signInflag)
				{
					signIn.result = false;
					memcpy_s(buffer, sizeof(SignIn), &signIn, sizeof(SignIn));
					std::cout << "�T�C���C�����s" << std::endl;
					int s = send(client->sock, buffer, sizeof(buffer), 0);
				}
				
			}
			break;
			case NetworkTag::SignUp:
			{
				++this->id;
				client->player->id = this->id;
				Login(client->sock, this->id);

				SignUp signUp;
				SignData signInData;

				memcpy_s(&signUp, sizeof(SignUp), buffer, sizeof(SignUp));

				strcpy_s(signInData.name, signUp.name);
				strcpy_s(signInData.pass, signUp.pass);
				signInData.ID = client->player->id;
				signInData.sock = client->sock;
				signInData.onlineflag = true;

				signData.push_back(signInData);

				int s = send(client->sock, buffer, sizeof(buffer), 0);

				std::cout <<"Name "<< signUp.name << "���T�C���A�b�v �p�X��"<<signUp.pass << std::endl;
				std::cout << signUp.name << "�̓I�����C����" << std::endl;
			}
			break;
			case NetworkTag::IdSearch:
			{
				IdSearch idSearch;
				bool result = false;
				memcpy_s(&idSearch, sizeof(IdSearch), buffer, sizeof(IdSearch));

				std::cout << idSearch.id << "��ID���� " << std::endl;
				//�T�[�o�[�ɓo�^����Ă�A�J�E���g��
				for (size_t i = 0; i < signData.size(); ++i)
				{
					//�T���Ă�ID�������
					if (idSearch.id == signData.at(i).ID)
					{
						result = true;
						idSearch.result = true;
						strcpy_s(idSearch.name, signData.at(i).name);

						memcpy_s(buffer, sizeof(IdSearch), &idSearch, sizeof(IdSearch));

						int s = send(client->sock, buffer, sizeof(buffer), 0);
						std::cout <<"���������O�� " << signData.at(i).name << std::endl;
						break;
					}
				}

				//�T���Ă�ID���Ȃ����
				if (!result)
				{
					idSearch.result = false;
					memcpy_s(buffer, sizeof(IdSearch), &idSearch, sizeof(IdSearch));
					int s = send(client->sock, buffer, sizeof(buffer), 0);
					std::cout << "������Ȃ����� " << std::endl;
					break;
				}
			}
			break;
			case NetworkTag::FriendRequest:
			{
				FriendRequest friendRequest;
				memcpy_s(&friendRequest, sizeof(FriendRequest), buffer, sizeof(FriendRequest));

				for (size_t i = 0; i < signData.size(); ++i)
				{
					if (friendRequest.Senderid == signData.at(i).ID)
					{
						//��M�����I�����C�����Ȃ�
						if (signData.at(i).onlineflag)
						{
							int s = send(signData.at(i).sock, buffer, sizeof(buffer), 0);
						}
						//��M�����I�t���C�����Ȃ�
						else
						{
							//	��M�����I�����C���ɂȂ�܂ŗ��߂�
							SenderData senderData;
							senderData.ID = signData.at(i).ID;
							strcpy_s(senderData.name, signData.at(i).name);
							signData.at(i).friendSenderSaveData.push_back(senderData);
						}
						break;
					}
				}
			}
			break;
			case NetworkTag::FriendApproval:
			{
				FriendApproval friendApproval;
				memcpy_s(&friendApproval, sizeof(FriendApproval), buffer, sizeof(FriendApproval));

				//���v�b�V��������I��邽��
				int pushcount = 0;

				//���݂��̃t�����h���X�g�ɓo�^
				for (size_t i = 0; i < signData.size(); ++i)
				{
					if (signData.at(i).ID == friendApproval.myid)
					{
						++pushcount;

						User user;
						user.ID= friendApproval.youid;
						strcpy_s(user.name, friendApproval.youname);

						signData.at(i).friendList.push_back(user);

						continue;
					}

					if (signData.at(i).ID == friendApproval.youid)
					{
						++pushcount;

						User user;
						user.ID = friendApproval.myid;
						strcpy_s(user.name, friendApproval.myname);
			
						signData.at(i).friendList.push_back(user);

						continue;
					}

					//���݂��̃t�����h���X�g�ɓo�^������I���
					if (pushcount >= 2)break;
					
				}
			}
			break;
			}
		}
		std::cout << "" <<std::endl;
	} while (Loop);
}




void Server::Login(SOCKET clientsock, short ID)
{
	PlayerLogin login;
	login.cmd = NetworkTag::Login;
	login.id = ID;

	char bufferlogin[sizeof(PlayerLogin)];
	memcpy_s(bufferlogin, sizeof(bufferlogin), &login, sizeof(PlayerLogin));
	send(clientsock, bufferlogin, sizeof(PlayerLogin), 0);

}