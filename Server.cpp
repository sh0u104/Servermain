#include "Server.h"

#include <stdio.h>
#include <WinSock2.h>
#include <iostream>

#include <fstream>
#include <string>
#include <ctime>
#include <iomanip>

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
	// サーバー起動時のログ
	WriteLog(LogLevel::Info, "サーバーが起動しました");
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

	//ログ
	WriteLog(LogLevel::Info, "サーバーの初期化終了");

	// サーバ側からコマンド入力で終了されるまでループする。
	// キーボードでexitを入力するとループを抜けるための別スレッドを用意
	std::thread th(&Server::Exit, this);

	// クライアントからの受付処理
	int size = sizeof(struct sockaddr_in);
	do {

		std::shared_ptr<Client> newClient = std::make_shared<Client>();
		// 接続要求を処理する
		newClient->sock = accept(sock, (struct sockaddr*)&newClient->addr, &size);
		if (newClient->sock != INVALID_SOCKET)
		{
			WriteLog(LogLevel::Info, "新しいクライアントが接続しました (ソケット: " + std::to_string(newClient->sock) + ")");
			//接続後処理
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
	 // スレッドの終了を待機してから解放
	for (auto& thread : recvThreads) {
		if (thread->joinable()) {
			thread->join();
		}
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
void Server::Recieve(std::shared_ptr<Client> client)
{
	bool Loop = true;
	std::cout  << "Loop開始" << std::endl;
	do {
		//UDP
		{
			char Buffer[256]{};
			int addrSize = sizeof(sockaddr_in);
			sockaddr_in temp;
			int udpSize = recvfrom(uSock, Buffer, sizeof(Buffer), 0, reinterpret_cast<sockaddr*>(&temp), &addrSize);
			

			if (udpSize == -1)
			{
				memset(Buffer, 0, sizeof(Buffer));
				int error = WSAGetLastError();
				// エラーメッセージを作成
				std::string errorMessage = "recvfrom error: " + std::to_string(error);

				if (error == WSAEWOULDBLOCK) {
					// データがまだ来ていない場合、処理をスキップ
				}
				else {
					// 他のエラーの場合、ループを終了
					if (error == WSAECONNRESET&& client->uAddr.sin_port == temp.sin_port)
					{
						std::cout << client->player->id << "は接続が途切れた" << std::endl;
						WriteLog(LogLevel::Error, "ID:" + std::to_string(client->player->id) + " は接続が途切れました (WSAECONNRESET)。");
						Loop = false;
					}
					else
					{
						std::cout << client->player->id << " recv error" << error << std::endl;
						std::string errorMessage = "recv エラー（UDP）: " + std::to_string(error);
						WriteLog(LogLevel::Error, errorMessage);
					}
				}
			}
		    if (udpSize == 0)
			{
				// クライアントが接続を閉じた場合の処理
				std::cout << "接続を閉じた" << std::endl;
				WriteLog(LogLevel::Info, "ID:" + std::to_string(client->player->id) + " が接続を閉じました。");
				Loop = false;
				break;
			}
			

			if (udpSize > 0)
			{
				short type = 0;
				memcpy_s(&type, sizeof(type), Buffer, sizeof(short));
			    //clientsに登録されてるかどうか
				if (HasSameData(clients, temp))
				{
					//本人に返すping測定用
					if (static_cast<UdpTag>(type) == UdpTag::Ping && client->uAddr.sin_port == temp.sin_port)
					{	
						sendto(uSock, Buffer, sizeof(Buffer), 0, (struct sockaddr*)(&client->uAddr), addrSize);
					}
					//チームに誰かいるか
					if (client->team)
					{
						//if (client->team->clients.size() > 0 && client->player->id != 0)
						//チームのclientsかどうか
						if (HasSameData(client->team->clients, temp))
						{
							switch (static_cast<UdpTag>(type))
							{
							case UdpTag::Move:
							{
								//自分なら飛ばす
								if (temp.sin_port == client->uAddr.sin_port)
									break;

								PlayerData playerData;
								memcpy_s(&playerData, sizeof(playerData), Buffer, sizeof(PlayerData));

								if (client->sendCount < playerData.sendCount)
								{
									client->sendCount = playerData.sendCount;
									udpSize = sendto(uSock, Buffer, sizeof(Buffer), 0, (struct sockaddr*)(&client->uAddr), addrSize);
								}

							}
							break;
							//ホストのエネミー情報をチームメンバーに送る用
							case UdpTag::EnemyMove:
							{
								//自分なら飛ばす
								if (temp.sin_port == client->uAddr.sin_port)
									break;

								udpSize = sendto(uSock, Buffer, sizeof(Buffer), 0, (struct sockaddr*)(&client->uAddr), addrSize);

							}
							break;
							}
						}
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
			int tcpSize = recv(client->sock, buffer, sizeof(buffer), 0);
			
			if (tcpSize == -1) {
				int error = WSAGetLastError();
				if (error == WSAEWOULDBLOCK) {
					// データがまだ来ていない場合、処理をスキップして次のループへ
				}
				else {
					// 他のエラーの場合、ループを終了
					if (error == WSAECONNRESET)
					{
						std::cout << client->player->id << "は接続が途切れた" << std::endl;
						WriteLog(LogLevel::Error,"ID:" + std::to_string(client->player->id) + " は接続が途切れました (WSAECONNRESET)。");
						Loop = false;
					}
					else
					{
						std::cout << client->player->id << " recv error" << error << std::endl;
						std::string errorMessage = "recv エラー（TCP）: " + std::to_string(error);
						WriteLog(LogLevel::Error, errorMessage);
					}
					
				}
			}
			if (tcpSize == 0)
			{
				Loop = false;
				std::cout << client->player->id << " が接続を閉じました" << std::endl;
				WriteLog(LogLevel::Info,"ID:"+ std::to_string(client->player->id) + " が接続を閉じました。");
			}

			if (tcpSize > 0)
			{
				short type = 0;
				memcpy_s(&type, sizeof(type), buffer, sizeof(short));
				switch (static_cast<TcpTag>(type))
				{
				case TcpTag::Logout:
				{

					PlayerLogout logout;
					memcpy_s(&logout, sizeof(logout), buffer, sizeof(PlayerLogout));

					WriteLog(LogLevel::Info, "クライアント (ID:" + std::to_string(logout.id) + ") がログアウトしました");

					std::cout << "ID " << logout.id << " 退出" << std::endl;
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
					char sendbuffer[2048];
					memcpy_s(&teamcreate, sizeof(teamcreate), buffer, sizeof(TeamCreate));

					auto team = std::make_shared<Team>();
					++teamnumbergrant;

					client->player->id = teamcreate.id;
					client->player->teamnumber = teamnumbergrant;
					client->team = team.get();
					team->isJoin = true;
					team->TeamNumber = teamnumbergrant;
					team->clients.emplace_back(client);
					teams.emplace_back(team);

					std::cout << "ID : " << client->player->id << "がチーム作成 ID : " << team->TeamNumber << std::endl;
					std::cout << "チーム番号 " << team->TeamNumber << " 加入可能" << std::endl;
					teamcreate.number = teamnumbergrant;
					teamcreate.Permission = true;
					memcpy_s(sendbuffer, sizeof(sendbuffer), &teamcreate, sizeof(TeamCreate));
					int s = send(client->sock, sendbuffer, sizeof(TeamCreate), 0);

					WriteLog(LogLevel::Info, "ID: " + std::to_string(client->player->id) + " がチームを作成しました" + std::to_string(team->TeamNumber));
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

					//ゲストログインじゃない時の為に
					client->player->id = teamjoin.id;

					for (int i = 0; i < teams.size(); ++i)
					{
						//チーム加入番号と一致しなければ
						if (teams.at(i)->TeamNumber != teamjoin.number)continue;
						//チームの全員埋まっていたら
						if (teams.at(i)->clients.size() >= 4)continue;

						//チームに入れるタイミングかどうか？
						if (teams.at(i)->isJoin)
						{
							join = true;
							client->player->teamnumber = teamjoin.number;
	
							teams.at(i)->clients.emplace_back(client);
							client->team = teams.at(i).get();
							//入れたら送る
							int s = send(client->sock, buffer, sizeof(Teamjoin), 0);
							std::cout << client->player->id << "がチームID : " << teamjoin.number << " に加入" << std::endl;

							//チームに入ろうとした人は誰がチームにいるかわからんから
							teamsync.cmd = TcpTag::Teamsync;
							//チームにいる人分
							for (int j = 0; j < teams.at(i)->clients.size(); ++j)
							{
								teamsync.id[j] = teams.at(i)->clients.at(j)->player->id;
								//本人に以外に送るため
								if (teamjoin.id != teamsync.id[j])
								{
									//チームにいる人に加入者の情報を送る
									int s = send(teams.at(i)->clients.at(j)->sock, buffer, sizeof(buffer), 0);
									std::cout << "ID " << teams.at(i)->clients.at(j)->player->id << " : にID　" << client->player->id << "の加入情報を送信" << std::endl;
								}
							}

							//参加者本人にすでにいる人の情報を取得送る
							memcpy_s(&syncbuffer, sizeof(syncbuffer), &teamsync, sizeof(Teamsync));
							int se = send(client->sock, syncbuffer, sizeof(Teamsync), 0);

							std::cout << "チーム情報";
							for (int i = 0; i < client->team->clients.size(); ++i)
							{
								std::cout << client->team->clients.at(i)->player->id << ",";
							}
							std::cout << std::endl;
							WriteLog(LogLevel::Info, "ID: " + std::to_string(client->player->id) +
								" がチームID:"+ std::to_string(client->player->teamnumber) + "に加入しました");
						}
					}

					//チームに加入出来なかったら
					if (!join)
					{
						teamjoin.number = -1;
						memcpy_s(&failurebuffer, sizeof(failurebuffer), &teamjoin, sizeof(Teamjoin));
						std::cout << "ID : " << teamjoin.id << "がチームに加入失敗 受信チーム番号" << teamjoin.number << std::endl;
						int s = send(client->sock, failurebuffer, sizeof(failurebuffer), 0);
						WriteLog(LogLevel::Info, "ID: " + std::to_string(client->player->id) +
							" がチームID:" + std::to_string(client->player->teamnumber) + "に加入失敗");
					}
				}
				break;
				//チームから抜けたら
				case TcpTag::Teamleave:
				{
					TeamLeave teamLeave;
					memcpy_s(&teamLeave, sizeof(teamLeave), buffer, sizeof(TeamLeave));

					//チームメンバーに送信
					for (int i = 0; i < client->team->clients.size(); ++i)
					{
						std::cout << "送信先id " << client->team->clients.at(i)->player->id << std::endl;
						int s = send(client->team->clients.at(i).get()->sock, buffer, sizeof(buffer), 0);

						std::cout << "ID " << teamLeave.id << "はチームから抜けた " << std::endl;
					}

					//ホストが抜けた時
					if (teamLeave.isHost)
					{
						//チームに入れなくする
						client->team->isJoin = false;
						std::cout <<"ID " << teamLeave.id << "のホストがチームを解散したチーム番号" << client->team->TeamNumber <<std::endl;
						for (int i = 0; i < client->team->clients.size(); ++i)
						{
							if (client != client->team->clients.at(i))
							RemoveClientFromTeam(client->team->clients.at(i), teams);
						}
					}
					else
					{
						std::cout << "ID" << teamLeave.id  << "　がチームから抜けたチーム番号 " << client->team->TeamNumber << std::endl;
					}
					RemoveClientFromTeam(client, teams);

					

				}
				break;
				case TcpTag::StartCheck:
				{
					StartCheck startcheck;
					memcpy_s(&startcheck, sizeof(startcheck), buffer, sizeof(StartCheck));

					client->startCheck= startcheck.check;
					//int s = send(client->sock, buffer, sizeof(StartCheck), 0);
					//チームに各々の準備チェックを送る
					for (int i = 0; i < client->team->clients.size(); ++i)
					{
						int s = send(client->team->clients.at(i)->sock, buffer, sizeof(buffer), 0);
					}

					if (startcheck.check)
					{
						std::cout << client->team->TeamNumber << "のID　:　" << client->player->id << "準備OK" << std::endl;
						WriteLog(LogLevel::Info, "チームID:" + std::to_string(client->team->TeamNumber) + "のID: " + std::to_string(client->player->id) + "準備OK");
					}
					else
					{
						std::cout << client->team->TeamNumber << "のID　:　" << client->player->id << "準備中" << std::endl;
						WriteLog(LogLevel::Info, "チームID:" + std::to_string(client->team->TeamNumber) + "のID: " + std::to_string(client->player->id) + "準備中");
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
						WriteLog(LogLevel::Info, "ソロでゲームが開始されました　ID ： "+ std::to_string(client->player->id));
						break;
					}
					else
					{
						
						//ホストからかどうか
						//if (team[teamGrantID].clients.at(0)->player->id != gamestart.id)continue;

						bool isStartCheck = true;

						
						//チーム全員が準備できているか
						for (int i = 1; i < client->team->clients.size(); ++i)
						{
							//チームメンバーに準備出来てない人がいたら
							if (client->team->clients.at(i)->startCheck != true)
							{
								isStartCheck = false;
								break;
							}
						}

						//ゲーム中はチームに参加できなくする
						client->team->isJoin = false;

						std::cout << "チームでゲームスタート" << std::endl;
						std::cout << "チーム番号 " << client->team->TeamNumber << " 加入不可" << std::endl;
						WriteLog(LogLevel::Info, "チームID:" + std::to_string(client->team->TeamNumber) + "ゲームスタート");
						WriteLog(LogLevel::Info, "チームID:" + std::to_string(client->team->TeamNumber) + "への加入は不可となりました");
						
						
						std::string teamLog = "チームID:" + std::to_string(client->team->TeamNumber) + "の構成: ";
						//全員の準備ができていたら
						if (isStartCheck)
						{
							for (int i = 0; i< client->team->clients.size(); ++i)
							{
								int s = send(client->team->clients.at(i)->sock, buffer, sizeof(buffer), 0);

								std::cout << "ゲームスタート送信 "<< client->team->clients.at(i)->player->id<< std::endl;
								WriteLog(LogLevel::Info, "ゲームスタートをID:" + std::to_string(client->team->clients.at(i)->player->id) + "に送信しました");
								//ログ用
								int playerId = client->team->clients.at(i)->player->id;
								teamLog += "ID:" + std::to_string(playerId);
								teamLog += ", ";
							}
							WriteLog(LogLevel::Info, teamLog);
						}
					}
				}
				break;
				case TcpTag::GameEnd:
				{
					
					GameEnd gameend;
					memcpy_s(&gameend, sizeof(GameEnd), buffer, sizeof(GameEnd));

					int teamGrantID = client->player->teamGrantID;
					
					//チームを組んでるかどうか
					if (client->team)
					{

						for (int i = 0; i < client->team->clients.size(); ++i)
						{
							//チーム全員の準備チェックを外す
							client->team->clients.at(i)->startCheck = false;
						}
						//チームにはいれるようにする
						client->team->isJoin = true;
						std::cout << "チーム番号 " << client->team->TeamNumber << " 加入可能" << std::endl;
						WriteLog(LogLevel::Info, "チームID: " + std::to_string(client->team->TeamNumber) + " はゲームクリア");
						WriteLog(LogLevel::Info, "チームID: " + std::to_string(client->team->TeamNumber) + " は加入可能な状態です");
					}
				}
				break;
				case TcpTag::GeustLogin:
				{

					++this->id;
					client->player->id = this->id;
					std::cout << "send login : " << this->id << "->" << client->player->id << std::endl;
					client->geustFlag = true;
					WriteLog(LogLevel::Info, "クライアントがゲストログインしました（ID: " + std::to_string(client->player->id) + "）");

					Login(client->sock, this->id);
				}
				break;
				case TcpTag::EnemyDamage:
				{
					//チームのホストに敵のダメージ判定を送る
					int s = send(client->team->clients.at(0)->sock, buffer, sizeof(buffer), 0);

				}
				break;
				case TcpTag::Message:
				{
					Message massage;
					memcpy(&massage, buffer, sizeof(Message));
					std::cout << "チーム番号" << client->team->TeamNumber<<"のID :"<<client->player->id << "のMessageを取得 "<< massage.text << std::endl;

					for (int i = 0; i < client->team->clients.size(); ++i)
					{
						if (client->team->clients.at(i)->player->id != client->player->id)
							std::cout <<"チーム番号" << client->team->TeamNumber << "の" << client->player->id << "に送信 " << std::endl;
							//continue;
						int s = send(client->team->clients.at(i)->sock, buffer, sizeof(buffer), 0);

					}
				}
				break;
				}
			}

		}
	} while (Loop);
	std::cout << "ID " << client->player->id <<"の";
	std::cout << "Loop停止" << std::endl;
	EraseClient(client);
	std::cout << "チーム数 "<<teams.size() << std::endl;
	std::cout << "クライアント数 "<<clients.size() << std::endl;
}




void Server::EraseClient(std::shared_ptr<Client> client)
{
	// クライアントを検索
	auto it = std::find(clients.begin(), clients.end(), client);
	if (it != clients.end()) {
		// ソケットを閉じる
		int r = closesocket(client->sock);
		if (r != 0) {
			std::cout << "ソケット消去失敗" << std::endl;
		}
		client->sock = INVALID_SOCKET;

		// クライアントをリストから削除
		clients.erase(it);
		std::cout << "消去成功" << std::endl;
	}
	else {
		std::cout << "消去失敗" << std::endl;
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
		// `client->team`が指すチームを検索
		auto it = std::find_if(teams.begin(), teams.end(),
			[&client](const std::shared_ptr<Team>& team) {
				return team.get() == client->team;
			});

		if (it != teams.end()) {
			// チームの`clients`から該当する`client`を削除
			auto& clients = (*it)->clients;
			clients.erase(std::remove(clients.begin(), clients.end(), client), clients.end());

			// チームが空になったら削除する場合
			if (clients.empty()) {
				std::cout << "Team ID " << (*it)->TeamNumber << "消去"<<std::endl;
				teams.erase(it);
			}

			// クライアントの所属チームをリセット
			client->team = nullptr;
		}
	}
}

void Server::WriteLog(LogLevel level, const std::string& message)
{
	// ログファイルを追記モードで開く
	std::ofstream logFile("server.log", std::ios::app);  // 追記モード
	if (!logFile.is_open()) {
		std::cout << "ログファイルのオープンに失敗しました。" << std::endl;
		return;
	}

	// 現在の時刻を取得
	time_t now = time(nullptr);
	tm localTime;  // tm 構造体をスタックに定義
	localtime_s(&localTime, &now);  // localtime_s は第二引数に tm のポインタを取る

	// ログレベルを文字列に変換
	std::string levelStr;
	switch (level) {
	case LogLevel::Info:  levelStr = "INFO"; break;
	case LogLevel::Warn:  levelStr = "WARN"; break;
	case LogLevel::Error: levelStr = "ERROR"; break;
	case LogLevel::Debug: levelStr = "DEBUG"; break;
	default: levelStr = "UNKNOWN"; break;
	}

	// ログの書き込み
	logFile << "[" << std::put_time(&localTime, "%Y-%m-%d %H:%M:%S")  // 時刻
		<< "][" << levelStr << "] "  // ログレベル
		<< message << "\n";  // メッセージ

// ファイルを閉じる
	logFile.close();
}