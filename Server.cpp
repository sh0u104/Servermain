#include "Server.h"

#include <stdio.h>
#include <WinSock2.h>
#include <iostream>

#pragma comment(lib, "ws2_32.lib")
// サーバ側コマンド入力スレッド
void Server::Exit()
{
	while (loop) {
		std::string input;
		std::cin >> input;
		if (input == "exit")
		{
			loop = false;
			std::cout << "サーバー閉じた" << std::endl;

		}
	}
}
// クライアント削除関数
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
	std::cout << "Server起動" << std::endl;
	// Server課題 サーバ情報設定
	// WinsockAPIを初期化
	WSADATA wsaData;

	// バージョンを指定する場合MAKEWORDマクロ関数を使用する
	int wsaStartUp = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (wsaStartUp != 0)
	{
		// 初期化失敗 
		std::cout << "WSA Initialize Failed." << std::endl;
		return;
	}

	// サーバの受付設定
	struct sockaddr_in addr;
	addr.sin_family = AF_INET;
	addr.sin_port = htons(7000);
	addr.sin_addr.S_un.S_addr = INADDR_ANY;//"0.0.0.0"

	// ソケットの作成
	SOCKET sock;
	sock = socket(AF_INET, SOCK_STREAM, 0);
	if (sock == INVALID_SOCKET) {
		std::cout << "Create Socket Failed." << std::endl;
		return;
	}

	// ソケットと受付情報を紐づける
	int Bind = bind(sock, reinterpret_cast<struct sockaddr*>(&addr), sizeof(addr));


	//// ノンブロッキングの設定
	//u_long mode = 1; // ノンブロッキングモードを有効にするために1を設定
	//ioctlsocket(sock, FIONBIO, &mode);

	// 受付を開始する
	int backlog = 10;
	int Listen = listen(sock, backlog);


	// サーバ側からコマンド入力で終了されるまでループする。
	// キーボードでexitを入力するとループを抜けるための別スレッドを用意
	std::thread th(&Server::Exit, this);

	// クライアントからの受付処理
	int size = sizeof(struct sockaddr_in);
	do {
		Client* newClient = new Client();
		// 接続要求を処理する
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
			//接続後処理
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

	// クライアントのソケット削除
	for (int i = 0; i < clients.size(); ++i)
	{
		closesocket(clients.at(i)->sock);
	}

	// 受付ソケットの切断
	closesocket(sock);

	// WSA終了
	int wsaCleanup = WSACleanup();

	Execute();
}

// 受信関数
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
				std::cout << "サーバー閉じる" << std::endl;
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

						//std::cout << "送信元id " << client->player->id << std::endl;
						//std::cout << "送信先id " << Client->player->id << std::endl;
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

					//std::cout << "送信元id " << client->player->id << std::endl;
					//std::cout << "送信先id " << Client->player->id << std::endl;
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
					std::cout << "送信元id " << client->player->id << std::endl;
					std::cout << "送信先id " << Client->player->id << std::endl;
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

				//チームメンバーに退出処理送信
				for (int i = 0; i < 3; ++i)
				{
					if (team[client->player->teamID].sock[i] == 0)break;

					std::cout << "退出 id : " << logout.id << std::endl;
					std::cout << "送信元id " << logout.id << std::endl;
					std::cout << "送信先id " << team[client->player->teamID].ID[i] << std::endl;
					int s = send(team[client->player->teamID].sock[i], buffer, sizeof(buffer), 0);

					std::cout << "send cmd " << type << std::endl;
					std::cout << "" << std::endl;
					if (team[client->player->teamID].ID[i] == logout.id)
					{
						team[client->player->teamID].sock[i] = 0;
					}
				}

				//アカウント情報をオフラインにする
				//ゲストログインじゃなかったら
				if (!client->Geustflag)
				{
					for (size_t i = 0; i < signData.size(); ++i)
					{
						if (signData.at(i).ID == client->player->id)
						{
							signData.at(i).onlineflag = false;
							std::cout << "Name " << signData.at(i).name << "はオフライン中" << std::endl;
							break;
						}
					}
				}
				std::cout<<"ID " << logout.id << " 退出" << std::endl;
				EraseClient(client);
				Loop = false;
				if (clients.size() <= 0)
				{
					std::cout << "サーバー閉じるなら" << "入力して　 exit" << std::endl;
				}
			}
			break;
			case NetworkTag::TeamCreate:
			{
				TeamCreate teamcreate;
				//チームを作成できたか
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

						std::cout << "ID : " << client->player->id << "が作成チームID : " << teamcreate.number << std::endl;
						break;
					}
				}
				//チームを作れなかったら
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
				//チームに加入できたか
				bool join = false;
				memcpy_s(&teamjoin, sizeof(teamjoin), buffer, sizeof(Teamjoin));

				for (int i = 0; i < TeamMax - 1; ++i)
				{
					if (team[i].sock[3] > 0)continue;

					if (team[i].TeamNumber == teamjoin.number)
					{
						for (int j = 0; j < 3; ++j)
						{
							//同期用にチームのIDをいれる
							teamsync.id[j] = team[i].ID[j];

							//チームに空きがあるか
							if (team[i].ID[j] == 0)
							{
								//入れたら
								join = true;
								team[i].sock[j] = client->sock;
								team[i].ID[j] = client->player->id;
								++team[i].logincount;

								client->player->teamnumber = team[i].TeamNumber;
								//入れたら送る
								int s = send(client->sock, buffer, sizeof(Teamjoin), 0);
								std::cout << client->player->id << "がチームID : " << teamjoin.number << " に加入" << std::endl;

								//チームに入ろうとした人は誰がチームにいるかわからんから
								teamsync.cmd = NetworkTag::Teamsync;
								teamsync.id[j] = teamjoin.id;
								//ここでsendする
								memcpy_s(&syncbuffer, sizeof(syncbuffer), &teamsync, sizeof(Teamsync));
								int se = send(client->sock, syncbuffer, sizeof(Teamsync), 0);
								break;
							}
							//チームにいる人に加入者の情報を送る
							int s = send(team[i].sock[j], buffer, sizeof(buffer), 0);
							std::cout << "ID " << team[i].ID[j] << " : にID　" << client->player->id << "の加入情報を送信" << std::endl;
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
									std::cout << team[i].TeamNumber<<"のID　:　" << client->player->id << "準備OK" << std::endl;
								}
								else
								{
									std::cout << team[i].TeamNumber << "のID　:　" << client->player->id << "準備中" << std::endl;
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
					//同じチーム番号が見つかったて
					//全員が準備できているか
					for (int j = 1; j < team[i].logincount; ++j)
					{
						if (team[i].ID[j] == 0)  break;
						if (team[i].check[j] != true)break;
						++sendcount;
					}
					//ログイン数と準備OKに数が合えば全員に送信
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
						//チーム全員の準備チェックを外す
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
					//サーバーログイン者データがあれば
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

						//サインイン情報を送り返す
						std::cout << "Name " << signIn.name << "がサインイン" << std::endl;
						std::cout << "Name " << signIn.name << "はオンライン中" << std::endl;
						int s = send(client->sock, buffer, sizeof(buffer), 0);

						//保存されてフレンドリクエストを全て送信
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
				//サーバーにログイン者情報がない時
				if(!signInflag)
				{
					signIn.result = false;
					memcpy_s(buffer, sizeof(SignIn), &signIn, sizeof(SignIn));
					std::cout << "サインイン失敗" << std::endl;
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

				std::cout <<"Name "<< signUp.name << "がサインアップ パスは"<<signUp.pass << std::endl;
				std::cout << signUp.name << "はオンライン中" << std::endl;
			}
			break;
			case NetworkTag::IdSearch:
			{
				IdSearch idSearch;
				bool result = false;
				memcpy_s(&idSearch, sizeof(IdSearch), buffer, sizeof(IdSearch));

				std::cout << idSearch.id << "でID検索 " << std::endl;
				//サーバーに登録されてるアカウント分
				for (size_t i = 0; i < signData.size(); ++i)
				{
					//探してるIDがあれば
					if (idSearch.id == signData.at(i).ID)
					{
						result = true;
						idSearch.result = true;
						strcpy_s(idSearch.name, signData.at(i).name);

						memcpy_s(buffer, sizeof(IdSearch), &idSearch, sizeof(IdSearch));

						int s = send(client->sock, buffer, sizeof(buffer), 0);
						std::cout <<"見つけた名前は " << signData.at(i).name << std::endl;
						break;
					}
				}

				//探してるIDがなければ
				if (!result)
				{
					idSearch.result = false;
					memcpy_s(buffer, sizeof(IdSearch), &idSearch, sizeof(IdSearch));
					int s = send(client->sock, buffer, sizeof(buffer), 0);
					std::cout << "見つからなかった " << std::endl;
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
						//受信側がオンライン中なら
						if (signData.at(i).onlineflag)
						{
							int s = send(signData.at(i).sock, buffer, sizeof(buffer), 0);
						}
						//受信側がオフライン中なら
						else
						{
							//	受信側がオンラインになるまで溜める
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

				//二回プッシュしたら終わるため
				int pushcount = 0;

				//お互いのフレンドリストに登録
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

					//お互いのフレンドリストに登録したら終わり
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