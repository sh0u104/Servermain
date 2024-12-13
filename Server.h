#pragma once

#pragma once
#include <Winsock2.h>
#include <iostream>
#include <thread>
#include <string>
#include <vector>
#include "Player.h"
#include "Enemy.h"
class Server
{
public:
	// 前方宣言
	struct Team;
	void UDPExecute();

	// クライアント情報
	struct Client {
		struct sockaddr_in addr {};
		struct sockaddr_in uAddr {};
		SOCKET sock = INVALID_SOCKET;
		//Player* player = nullptr;
		std::shared_ptr<Player> player;
		bool Geustflag = false;
		bool isRecvUdpAddr = false;
		bool startCheck = false;
		Team* team = nullptr; // 所属するチームを示すポインタ
	};
	struct Team
	{
		std::vector<std::shared_ptr<Client>> clients;
		int TeamNumber = 0;
		bool isJoin = false;
	};
	enum class TcpTag : unsigned short
	{
		SignUp,         //アカウント作成
		SignIn,         //アカウントでログイン
		GeustLogin,		// ゲストアカウントでログイン
		Logout,			// サーバーからログアウト
		TeamCreate,     //チーム作成
		Teamjoin,       //チーム加入
		Teamleave,      //チームを抜ける
		Teamsync,       // 後に入った人にチームメンバーのIDをおくる
		StartCheck,     //スタート準備ができてるか
		Gamestart,      //ゲームを始めでいいか
		GameEnd,        //ゲームが終わったら
		Move,
		Sync,			// 同期
		Login,
		UdpAddr,          //サーバーでUDPのアドレス保存用
		EnemyDamage,    //敵がダメージを受けたら
		Message,
	};
	enum class UdpTag : unsigned short
	{
		Message,		// チャットメッセージ
		Move,			// 移動
		EnemyMove,       //敵の移動
		Attack,			// 攻撃
		UdpAddr,          //サーバーでUDPのアドレス保存用
	};


	

	struct SenderData
	{
		char name[10];
		int ID;
	};

	struct User
	{
		int ID;
		char name[10];
	};

	struct SignData
	{
		char name[10];
		char pass[10];
		int ID;
		SOCKET sock;
		bool onlineflag;

		std::vector<SenderData>friendSenderSaveData;
		std::vector<User>friendList;

	};
	//std::vector<SignData>signData;

#define TeamJoinMax 4
#define TeamMax 10
	//Team team[TeamMax];
	std::vector<std::shared_ptr<Team>> teams;


	struct Message
	{
		UdpTag cmd;
		char text[32];
	};
	struct PlayerInput
	{
		UdpTag cmd;
		int id;
		DirectX::XMFLOAT3 velocity;
		DirectX::XMFLOAT3 position;
		Player::State state;
		DirectX::XMFLOAT3 angle;
	};
	



	struct PlayerLogin
	{
		TcpTag cmd;
		int id;
	};

	struct GeustLogin
	{
		TcpTag cmd;
	};

	struct SignIn
	{
		TcpTag cmd;
		char name[10];
		char pass[10];
		bool result;
	};

	struct SignUp
	{

		TcpTag cmd;
		char name[10];
		char pass[10];
	};

	struct PlayerLogout
	{
		TcpTag cmd;
		int id;
	};
	struct PlayerSync
	{
		TcpTag cmd;
		int id;
		DirectX::XMFLOAT3 position;
		DirectX::XMFLOAT3 velocity;
		DirectX::XMFLOAT3 angle;
	};

	struct TeamCreate
	{
		TcpTag cmd;
		int id;
		int number;
		bool Permission;
	};

	struct Teamjoin
	{
		TcpTag cmd;
		int id;
		int number;
	};

	struct TeamLeave
	{
		TcpTag cmd;
		int id;
		bool isHost;
	};

	struct Teamsync
	{
		TcpTag cmd;
		int id[4] = {};
	};

	struct StartCheck
	{
		TcpTag cmd;
		int id;
		short teamnunber;
		bool check;
	};

	struct GameStart
	{
		TcpTag cmd;
		int id;
		short teamnunber;
	};
	struct GameEnd
	{
		TcpTag cmd;
		short teamnunber;
	};

	struct SendUdpAddr
	{
		TcpTag cmd;
	};

	/*struct IdSearch
	{
		NetworkTag cmd;
		short id;
		bool result;
		char name[10];
	};

	struct FriendRequest
	{
		NetworkTag cmd;
		char name[10];
		short senderid;
		short Senderid;

	};*/
	/*struct FriendApproval
	{
		NetworkTag cmd;
		short myid;
		short youid;
		char myname[10];
		char youname[10];
	};

	struct SeeFriend
	{
		NetworkTag cmd;
		short myid;
		std::vector<User>friendList;
	};*/

	void Execute();
	void Exit();
	void Recieve(std::shared_ptr<Client> client);
	//void EraseClient(Client* client);
	void EraseClient(std::shared_ptr<Client> client);
	void Login(SOCKET clientsock, short ID);

	// 登録済のクライアントか判断を行う
	bool HasSameData(const std::vector<std::shared_ptr<Client>>& vec, const sockaddr_in& target);
	//teamsからclientのteamを消去
	void RemoveClientFromTeam(std::shared_ptr<Client> client, std::vector<std::shared_ptr<Team>>& teams);

private:
	int id = 0;
	SOCKET sock = INVALID_SOCKET;
	//std::vector<Client*> clients;
	//std::vector<std::thread*> recvThreads;
	std::vector<std::shared_ptr<std::thread>> recvThreads;
	std::vector<std::shared_ptr<Client>> clients;
	bool loop = true;

	int teamnumbergrant = 1000;
	

	struct sockaddr_in uAddr;
	SOCKET uSock = INVALID_SOCKET;
};