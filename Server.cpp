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
			int addrSize = sizeof(sockaddr);
			sockaddr_in temp;
			int size = recvfrom(uSock, Buffer, sizeof(Buffer), 0, reinterpret_cast<sockaddr*>(&temp), &addrSize);

			if (size > 0)
			{
				short type = 0;
				memcpy_s(&type, sizeof(type), Buffer, sizeof(short));
			    //��x�ڂ��ǂ���
				if (HasSameData(clients, temp))
				{
					//std::cout << "recv cmd " << type << std::endl;
					switch (static_cast<UdpTag>(type))
					{
					case UdpTag::Move:
					{
						//�`�[���̊F��
						for (int i = 0; i < team[client->player->teamGrantID].logincount; ++i)
						{
							//�����Ȃ��΂�
							if (team[client->player->teamGrantID].sock[i] == client->sock)
								continue;
							size = sendto(uSock, Buffer, sizeof(Buffer), 0, (struct sockaddr*)(&team[client->player->teamGrantID].uAddr[i]), addrSize);
						}

					}
					break;
					
					case UdpTag::EnemyMove:
					{
						//�`�[���̊F��
						for (int i = 0; i < team[client->player->teamGrantID].logincount; ++i)
						{
							//�����Ȃ��΂�
							if (team[client->player->teamGrantID].sock[i] == client->sock)
								continue;

							size = sendto(uSock, Buffer, sizeof(Buffer), 0, (struct sockaddr*)(&team[client->player->teamGrantID].uAddr[i]), addrSize);
						}
					}
					break;
					}
				}
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
							std::cout << "UDP�A�h���X�󂯎������" << std::endl;
							std::cout << "ID : " << client->player->id << std::endl;
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
						//�`�[���Ǘ��z��ɋ󂫂����邩
						if (team[i].TeamNumber == 0)
						{
							Permission = true;
							++teamnumbergrant;
							
							team[i].TeamNumber = teamnumbergrant;
							std::cout << "�`�[���ԍ� " << team[i].TeamNumber << " �����\" << std::endl;
							team[i].isJoin = true;
							//�`�[���̃z�X�g�ɑ��
							team[i].sock[0] = client->sock;
							team[i].ID[0] = teamcreate.id;
							team[i].uAddr[0] = client->uAddr;

							client->player->teamnumber = teamnumbergrant;
							client->player->teamGrantID = i;
						    //�`�[���̃��O�C����
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

					//�Q�X�g����Ȃ����ׂ̈�
					client->player->id = teamjoin.id;

					for (int i = 0; i < TeamMax - 1; ++i)
					{
						//�`�[�������ԍ��ƈ�v���Ȃ����
						if (team[i].TeamNumber != teamjoin.number)continue;
						//�`�[���̑S�����܂��Ă�����
						if (team[i].sock[3] > 0)continue;

						//�`�[���ɓ����^�C�~���O���ǂ����H
						if(team[i].isJoin)
						//���Ԗڂɓ��邩�m�F
						{
							for (int j = 0; j < TeamJoinMax; ++j)
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
									team[i].uAddr[j] = client->uAddr;

									client->player->teamnumber = team[i].TeamNumber;
					
									client->player->teamGrantID = i;
									//���ꂽ�瑗��
									int s = send(client->sock, buffer, sizeof(Teamjoin), 0);
									std::cout << client->player->id << "���`�[��ID : " << teamjoin.number << " �ɉ���" << std::endl;

									std::cout << "�`�[�����"<< team[i].ID[0]<<" " << team[i].ID[1] << " " << team[i].ID[2] << " " << team[i].ID[3] << " " << std::endl;
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
				//�`�[�����甲������
				case TcpTag::Teamleave:
				{
					TeamLeave teamLeave;
					memcpy_s(&teamLeave, sizeof(teamLeave), buffer, sizeof(TeamLeave));
					std::cout <<"recvTeamLeave" << std::endl;
					int teamGrantID = client->player->teamGrantID;
					//�z�X�g����������
					if (teamLeave.isHost)
					{
						//�`�[���ɓ���Ȃ�����
						team[teamGrantID].isJoin = false;
						std::cout <<"ID " << teamLeave.id << "�̃z�X�g���`�[�������U�����ԍ�" << team[teamGrantID].TeamNumber <<std::endl;
					}
					else
					{
						std::cout << "ID" << teamLeave.id  << "�@���`�[�����甲�����`�[���ԍ� " << team[teamGrantID].TeamNumber << std::endl;
					}

					//�`�[�������o�[�ɑ��M
					for (int i = 0; i < TeamJoinMax; ++i)
					{
						if (team[teamGrantID].ID[i] <= 0)continue;

						std::cout << "���M��id " << team[client->player->teamGrantID].ID[i] << std::endl;
						int s = send(team[teamGrantID].sock[i], buffer, sizeof(buffer), 0);

						std::cout << "�`�[�����甲���� " << std::endl;
						std::cout << "" << std::endl;
						//�������ꏊ��������
						if (team[teamGrantID].ID[i] == teamLeave.id)
						{
							team[teamGrantID].sock[i] = 0;
							team[teamGrantID].ID[i] = 0;
							team[teamGrantID].logincount -= 1;
							team[teamGrantID].check[i] = false;
						}
					}

				}
				break;
				case TcpTag::StartCheck:
				{
					StartCheck startcheck;
					memcpy_s(&startcheck, sizeof(startcheck), buffer, sizeof(StartCheck));
					int teamGrantID = client->player->teamGrantID;

						for (int i = 0; i < TeamJoinMax; ++i)
						{
							//�`�[���̖{�l������������
							if (team[teamGrantID].sock[i] == client->sock)
							{
								team[teamGrantID].check[i] = startcheck.check;

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
				}
				break;
				case TcpTag::Gamestart:
				{
					GameStart gamestart;
					int sendcount = 0;
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

						int teamGrantID = client->player->teamGrantID;
						//�Q�[�����̓`�[���ɎQ���ł��Ȃ�����
						team[teamGrantID].isJoin = false;
						std::cout << "�`�[���ŃQ�[���X�^�[�g�`�[�� " << std::endl;
						std::cout << "�`�[���ԍ� " << team[teamGrantID].TeamNumber << " �����s��" << std::endl;
						//�z�X�g���炩�ǂ���
						if (team[teamGrantID].ID[0] != gamestart.id)continue;

						//�����`�[���ԍ�������������
						//�S���������ł��Ă��邩
						for (int i = 1; i < team[teamGrantID].logincount; ++i)
						{
							//�`�[�������o�[�����I�������
							if (team[teamGrantID].ID[i] == -1)  break;
							//�`�[�������o�[�ɏ����o���ĂȂ��l��������
							if (team[teamGrantID].check[i] != true)break;

							//�`�[�������o�[�̃`�F�b�N��true�Ȃ�
							++sendcount;
						}

						//����OK�ƃ��O�C�����̐��������ΑS���ɑ��M
						if (sendcount == team[teamGrantID].logincount - 1)
						{
							for (int count = sendcount; -1 < count; --count)
							{
								int s = send(team[teamGrantID].sock[count], buffer, sizeof(buffer), 0);
								std::cout << "�Q�[���X�^�[�g���M "<< team[teamGrantID].ID[count]<< std::endl;
								std::cout<<count <<"�l��" << "addr " << team[teamGrantID].uAddr[count].sin_addr.S_un.S_addr <<
									"port"<< team[teamGrantID].uAddr[count].sin_port << std::endl;
								
							}
						}

					}
				}
				break;
				case TcpTag::GameEnd:
				{
					GameEnd gameend;
					memcpy_s(&gameend, sizeof(GameEnd), buffer, sizeof(GameEnd));

					int teamGrantID = client->player->teamGrantID;

					if (team[teamGrantID].sock[0] == client->sock)
					{
						//�`�[���S���̏����`�F�b�N���O��
						std::fill(team[teamGrantID].check, team[teamGrantID].check + 4, false);
					}
					team[teamGrantID].isJoin = true;
					std::cout	<<"�`�[���ԍ� " << team[teamGrantID].TeamNumber << " �����\" << std::endl;

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
					int teamGrantID = client->player->teamGrantID;
					//�`�[���̃z�X�g�ɓG�̃_���[�W����𑗂�
					int s = send(team[teamGrantID].sock[0], buffer, sizeof(buffer), 0);

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

bool Server::HasSameData(const std::vector<Client*>& vec, const sockaddr_in& target)
{
	for (const Client* client : vec)
	{
		if (client->uAddr.sin_addr.S_un.S_addr == target.sin_addr.S_un.S_addr &&
			client->uAddr.sin_port == target.sin_port)
		{
			return true;
		}
	}
	return false;
}
