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


void Server::UDPExecute()
{
	uAddr.sin_family = AF_INET;
	uAddr.sin_port = htons(8000);
	uAddr.sin_addr.S_un.S_addr = INADDR_ANY;

	uSock = socket(AF_INET, SOCK_DGRAM, 0);
	if (uSock == INVALID_SOCKET) {
		std::cout << "ソケットの生成に失敗しました" << std::endl;
		// 9.WSAの解放
		WSACleanup();
	}

	// bind
	bind(uSock, (struct sockaddr*)&uAddr, sizeof(uAddr));

	//// ノンブロッキングの設定
	u_long mode = 1; // ノンブロッキングモードを有効にするために1を設定
	ioctlsocket(uSock, FIONBIO, &mode);
}


void Server::Execute()
{
	std::cout << "Server起動" << std::endl;
	std::cout << "接続だけならID 0" << std::endl;
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
	SOCKET sock = socket(AF_INET, SOCK_STREAM, 0);
	if (sock == INVALID_SOCKET) {
		std::cout << "Create Socket Failed." << std::endl;
		return;
	}

	// ソケットと受付情報を紐づける
	if (bind(sock, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) != 0)
	{
		std::cout << "Bind Failed." << std::endl;
		return;
	}
	
	// 受付を開始する
	int backlog = 10;
	int Listen = listen(sock, backlog);
	

	//// ノンブロッキングの設定
	u_long mode = 1; // ノンブロッキングモードを有効にするために1を設定
	ioctlsocket(sock, FIONBIO, &mode);

	UDPExecute();


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
			//接続後処理
			newClient->player = new Player();
			newClient->player->id = 0;
			newClient->player->position = DirectX::XMFLOAT3(0, 0, 0);
			newClient->player->angle = DirectX::XMFLOAT3(0, 0, 0);

			clients.emplace_back(newClient);

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

}

// 受信関数
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
			    //clientsに登録されてるかどうか
				if (HasSameData(clients, temp))
				{
					int teamGrantID = client->player->teamGrantID;
					switch (static_cast<UdpTag>(type))
					{
					case UdpTag::Move:
					{
						//チームからかどうか
						if (HasSameData(team[teamGrantID].clients, temp))
						{
							//自分なら飛ばす
							if (temp.sin_port == client->uAddr.sin_port)
								continue;
							size = sendto(uSock, Buffer, sizeof(Buffer), 0, (struct sockaddr*)(&client->uAddr), addrSize);
							//チームの皆に
							//for (int i = 0; i < team[teamGrantID].clients.size(); ++i)
							//{
								
								//if (team[teamGrantID].clients.at(i)->player->id == client->player->id)
								//break;
								//size = sendto(uSock, Buffer, sizeof(Buffer), 0, (struct sockaddr*)(&team[teamGrantID].clients.at(i)->uAddr), addrSize);
								//size = sendto(uSock, Buffer, sizeof(Buffer), 0, (struct sockaddr*)(&client->uAddr), addrSize);

							//}
						}

					}
					break;
					//ホストのエネミー情報をチームメンバーに送る用
					case UdpTag::EnemyMove:
					{
						//チームの皆に
						for (int i = 0; i < team[teamGrantID].clients.size(); ++i)
						{
							//自分なら飛ばす
							if (team[teamGrantID].clients.at(i)->uAddr.sin_port == client->uAddr.sin_port)
							continue;
							size = sendto(uSock, Buffer, sizeof(Buffer), 0, (struct sockaddr*)(&team[teamGrantID].clients.at(i)->uAddr), addrSize);
						}
					}
					break;
					}

				}
				//clientsに登録されてなかったら
				else
				{
					switch (static_cast<UdpTag>(type))
					//全員のUDPアドレスを保存する
					case UdpTag::UdpAddr:
					{
						if (!client->isRecvUdpAddr)
						{
							//UDPのアドレスを保存し
							client->uAddr = temp;
							client->isRecvUdpAddr = true;
							//接続者は0をいれる（非接続者は-1）
							client->player->id = 0;

							//保存したよをTCPで返す
							std::cout << "UDPアドレスを保存した" << std::endl;
							
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
					std::cout << "サーバー閉じる" << std::endl;
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

					std::cout << "ID " << logout.id << " 退出" << std::endl;
					EraseClient(client);
					Loop = false;
					if (clients.size() <= 0)
					{
						std::cout << "サーバー閉じるなら" << "入力して　 exit" << std::endl;
					}
				}
				break;
				case TcpTag::TeamCreate:
				{
					TeamCreate teamcreate;
					//チームを作成できたか
					bool Permission = false;
					char sendbuffer[2048];
					memcpy_s(&teamcreate, sizeof(teamcreate), buffer, sizeof(TeamCreate));

					for (int i = 0; i < TeamMax - 1; ++i)
					{
						//チームに誰かいたら入れない
						if (team[i].clients.size() > 0)
							continue;
						//チームに空きがあったら
						Permission = true;
						++teamnumbergrant;
						team[i].TeamNumber = teamnumbergrant;
						team[i].isJoin = true;
						client->player->teamnumber = teamnumbergrant;
						client->player->teamGrantID = i;

						team[i].clients.emplace_back(client);

						std::cout << "ID : " << client->player->id << "がチーム作成 ID : " << team[i].TeamNumber << std::endl;
					    std::cout << "チーム番号 " << team[i].TeamNumber << " 加入可能" << std::endl;
					    
					    teamcreate.number = teamnumbergrant;
					    teamcreate.Permission = true;
					    memcpy_s(sendbuffer, sizeof(sendbuffer), &teamcreate, sizeof(TeamCreate));
					    int s = send(client->sock, sendbuffer, sizeof(TeamCreate), 0);
						break;
					}
					//for (int i = 0; i < TeamMax - 1; ++i)
					//{
					//	//チーム管理配列に空きがあるか
					//	if (team[i].TeamNumber == 0)
					//	{
					//		Permission = true;
					//		++teamnumbergrant;
					//		
					//		team[i].TeamNumber = teamnumbergrant;
					//		team[i].isJoin = true;
					//		//チームのホストに代入
					//		team[i].sock[0] = client->sock;
					//		team[i].ID[0] = teamcreate.id;
					//		team[i].uAddr[0] = client->uAddr;
					//
					//		client->player->teamnumber = teamnumbergrant;
					//		client->player->teamGrantID = i;
					//	    //チームのログイン数
					//		++team[i].logincount;
					//
					//		std::cout << "ID : " << client->player->id << "がチーム作成 ID : " << team[i].TeamNumber << std::endl;
					//		std::cout << "チーム番号 " << team[i].TeamNumber << " 加入可能" << std::endl;
					//
					//		teamcreate.number = teamnumbergrant;
					//		teamcreate.Permission = true;
					//		memcpy_s(sendbuffer, sizeof(sendbuffer), &teamcreate, sizeof(TeamCreate));
					//		int s = send(client->sock, sendbuffer, sizeof(TeamCreate), 0);
					//
					//		
					//		break;
					//	}
					//}
					//チームを作れなかったら
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
					//チームに加入できたか
					bool join = false;
					memcpy_s(&teamjoin, sizeof(teamjoin), buffer, sizeof(Teamjoin));

					//ゲストじゃない時の為に
					client->player->id = teamjoin.id;

					for (int i = 0; i < TeamMax - 1; ++i)
					{
						//チーム加入番号と一致しなければ
						if (team[i].TeamNumber != teamjoin.number)continue;
						//チームの全員埋まっていたら
						if (team[i].clients.size() >= 4)continue;
						//チームに入れるタイミングかどうか？
						if (team[i].isJoin)
						{
							//入れたら
								join = true;
								client->player->teamnumber = team[i].TeamNumber;
								client->player->teamGrantID = i;
								
								team[i].clients.emplace_back(client);
								//入れたら送る
								int s = send(client->sock, buffer, sizeof(Teamjoin), 0);
								std::cout << client->player->id << "がチームID : " << teamjoin.number << " に加入" << std::endl;
					
								
								//チームに入ろうとした人は誰がチームにいるかわからんから
								teamsync.cmd = TcpTag::Teamsync;

								for (int j = 0; j < team[i].clients.size(); ++j)
								{
									teamsync.id[j] = team[i].clients.at(j)->player->id;
									//本人に以外に送るため
									if (teamjoin.id!= teamsync.id[j])
									{
										//チームにいる人に加入者の情報を送る
										int s = send(team[i].clients.at(j)->sock, buffer, sizeof(buffer), 0);
										std::cout << "ID " << team[i].clients.at(j)->player->id << " : にID　" << client->player->id << "の加入情報を送信" << std::endl;
									}
								}
								//参加者本人にすでにいる人の情報を取得送る
								memcpy_s(&syncbuffer, sizeof(syncbuffer), &teamsync, sizeof(Teamsync));
								int se = send(client->sock, syncbuffer, sizeof(Teamsync), 0);

								std::cout << "チーム情報";
								for (int i = 0; i < team[client->player->teamGrantID].clients.size(); ++i)
								{
									std::cout << team[client->player->teamGrantID].clients.at(i)->player->id<<",";
								}
								std::cout << std::endl;
							//チームにいる人に加入者の情報を送る
						//		int s = send(team[i].sock[j], buffer, sizeof(buffer), 0);
						//		std::cout << "ID " << team[i].ID[j] << " : にID　" << client->player->id << "の加入情報を送信" << std::endl;
						}

						//チームに入れるタイミングかどうか？
						//if(team[i].isJoin)
						////何番目に入るか確認
						//{
						//	for (int j = 0; j < TeamJoinMax; ++j)
						//	{
						//		//同期用にチームのIDをいれる
						//		teamsync.id[j] = team[i].ID[j];
						//
						//		//チームに空きがあるか
						//		if (team[i].ID[j] == 0)
						//		{
						//			//入れたら
						//			join = true;
						//			team[i].sock[j] = client->sock;
						//			team[i].ID[j] = client->player->id;
						//			++team[i].logincount;
						//			team[i].uAddr[j] = client->uAddr;
						//
						//			client->player->teamnumber = team[i].TeamNumber;
						//
						//			client->player->teamGrantID = i;
						//			//入れたら送る
						//			int s = send(client->sock, buffer, sizeof(Teamjoin), 0);
						//			std::cout << client->player->id << "がチームID : " << teamjoin.number << " に加入" << std::endl;
						//
						//			std::cout << "チーム情報"<< team[i].ID[0]<<" " << team[i].ID[1] << " " << team[i].ID[2] << " " << team[i].ID[3] << " " << std::endl;
						//			//チームに入ろうとした人は誰がチームにいるかわからんから
						//			teamsync.cmd = TcpTag::Teamsync;
						//			teamsync.id[j] = teamjoin.id;
						//			//ここでsendする
						//			memcpy_s(&syncbuffer, sizeof(syncbuffer), &teamsync, sizeof(Teamsync));
						//			int se = send(client->sock, syncbuffer, sizeof(Teamsync), 0);
						//			break;
						//		}
						//
						//		//チームにいる人に加入者の情報を送る
						//		int s = send(team[i].sock[j], buffer, sizeof(buffer), 0);
						//		std::cout << "ID " << team[i].ID[j] << " : にID　" << client->player->id << "の加入情報を送信" << std::endl;
						//	}
						//	break;
						//
						//}
					}
					//チームに加入出来なかったら
					if (!join)
					{
						teamjoin.number = -1;
						memcpy_s(&failurebuffer, sizeof(failurebuffer), &teamjoin, sizeof(Teamjoin));
						std::cout << "ID : " << teamjoin.id << "がチームに加入失敗 受信チーム番号" << teamjoin.number << std::endl;
						int s = send(client->sock, failurebuffer, sizeof(failurebuffer), 0);
					}
				}
				break;
				//チームから抜けたら
				case TcpTag::Teamleave:
				{
					TeamLeave teamLeave;
					memcpy_s(&teamLeave, sizeof(teamLeave), buffer, sizeof(TeamLeave));
					int teamGrantID = client->player->teamGrantID;
					//ホストが抜けた時
					if (teamLeave.isHost)
					{
						//チームに入れなくする
						team[teamGrantID].isJoin = false;
						std::cout <<"ID " << teamLeave.id << "のホストがチームを解散したチーム番号" << team[teamGrantID].TeamNumber <<std::endl;
					}
					else
					{
						std::cout << "ID" << teamLeave.id  << "　がチームから抜けたチーム番号 " << team[teamGrantID].TeamNumber << std::endl;
					}

					//チームメンバーに送信
					for (int i = 0; i < team[teamGrantID].clients.size(); ++i)
					{
						//if (team[teamGrantID].ID[i] <= 0)continue;

						std::cout << "送信先id " << team[teamGrantID].clients.at(i)->player->id << std::endl;
						int s = send(team[teamGrantID].clients.at(i)->sock, buffer, sizeof(buffer), 0);

						std::cout <<"ID " << teamLeave.id << "はチームから抜けた " << std::endl;
						////抜けた場所を初期化
						//if (team[teamGrantID].ID[i] == teamLeave.id)
						//{
						//	team[teamGrantID].sock[i] = 0;
						//	team[teamGrantID].ID[i] = 0;
						//	team[teamGrantID].logincount -= 1;
						//	team[teamGrantID].check[i] = false;
						//}
					}
					
					// teamGrantIDを基に、削除する処理
					auto& Clients = team[teamGrantID].clients; // 省略用の参照
					auto it = std::find(Clients.begin(), Clients.end(), client); // client を検索

					if (it != Clients.end()) {
						// 見つかった場合は削除
						Clients.erase(it);
					}
					else {
						// 見つからなかった場合の処理（必要に応じて追加）
						std::cout << "Client not found in team[teamGrantID].clients " << client->player->id << std::endl;
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
							//チームの本人が見つかったら
							if (team[teamGrantID].clients.at(i)->sock == client->sock)
							{
								team[teamGrantID].clients.at(i)->startCheck = startcheck.check;

								int s = send(client->sock, buffer, sizeof(StartCheck), 0);

								if (startcheck.check)
								{
									std::cout << team[i].TeamNumber << "のID　:　" << client->player->id << "準備OK" << std::endl;
								}
								else
								{
									std::cout << team[i].TeamNumber << "のID　:　" << client->player->id << "準備中" << std::endl;
								}

								break;
							}
						}
				}
				break;
				case TcpTag::Gamestart:
				{
					GameStart gamestart;
					
					memcpy_s(&gamestart, sizeof(gamestart), buffer, sizeof(GameStart));

					//チームを組んでるかどうか
					if (gamestart.teamnunber == 0)
					{
						//組んでなかったそのまま返す
						send(client->sock, buffer, sizeof(buffer), 0);
						std::cout << "ソロでゲームスタートID "<< client->player->id <<  std::endl;
						break;
					}
					else
					{

						int teamGrantID = client->player->teamGrantID;
						//ゲーム中はチームに参加できなくする
						team[teamGrantID].isJoin = false;
						std::cout << "チームでゲームスタートチーム " << std::endl;
						std::cout << "チーム番号 " << team[teamGrantID].TeamNumber << " 加入不可" << std::endl;
						//ホストからかどうか
						if (team[teamGrantID].clients.at(0)->player->id != gamestart.id)continue;

						bool isStartCheck = true;
						//同じチーム番号が見つかったて
						//全員が準備できているか
						for (int i = 1; i < team[teamGrantID].clients.size(); ++i)
						{
							//チームメンバーに準備出来てない人がいたら
							if (team[teamGrantID].clients.at(i)->startCheck != true)
							{
								isStartCheck = false;
								break;
							}
						}

						//全員の準備ができていたら
						if (isStartCheck)
						{
							for (int i = 0; i<team[teamGrantID].clients.size(); ++i)
							{
								int s = send(team[teamGrantID].clients.at(i)->sock, buffer, sizeof(buffer), 0);
								std::cout << "ゲームスタート送信 "<< team[teamGrantID].clients.at(i)->player->id<< std::endl;
								std::cout<<"ID : " << team[teamGrantID].clients.at(i)->player->id << "uaddr "<< team[teamGrantID].clients.at(i)->uAddr.sin_addr.S_un.S_addr <<
								"uport"<< team[teamGrantID].clients.at(i)->uAddr.sin_port << std::endl;

								std::cout <<"teamGrantID : " << team[teamGrantID].clients.at(i)->player->teamGrantID << std::endl;
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
					//ホストからかどうか
					if (team[teamGrantID].clients.at(0)->sock == client->sock)
					{
						for(int i=0;i< team[teamGrantID].clients.size();++i)
						{ 
							//チーム全員の準備チェックを外す
							team[teamGrantID].clients.at(i)->startCheck = false;
						}
					}
					//チームにはいれるようにする
					team[teamGrantID].isJoin = true;
					std::cout	<<"チーム番号 " << team[teamGrantID].TeamNumber << " 加入可能" << std::endl;

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
					//チームのホストに敵のダメージ判定を送る
					int s = send(team[teamGrantID].clients.at(0)->sock, buffer, sizeof(buffer), 0);

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
				//			//std::cout << "送信元id " << client->player->id << std::endl;
				//			//std::cout << "送信先id " << Client->player->id << std::endl;
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

