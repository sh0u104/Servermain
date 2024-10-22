#pragma once

#pragma once
#include <Winsock2.h>
#include <iostream>
#include <thread>
#include <string>
#include <vector>
#include "Player.h"

class Server
{
public:

	void UDPExecute();

	// クライアント情報
	struct Client {
		struct sockaddr_in addr {};
		SOCKET sock = INVALID_SOCKET;
		Player* player = nullptr;
		bool Geustflag = false;
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
	};
	enum class UdpTag : unsigned short
	{
		Message,		// チャットメッセージ
		//Move,			// 移動
		Attack,			// 攻撃

	};


	struct Team
	{
		int TeamNumber = 0;
		SOCKET sock[4] = {};
		int ID[4] = {};
		bool check[4] = {};
		int logincount = 0;
	};

	struct SenderData
	{
		char name[10];
		short ID;
	};

	struct User
	{
		short ID;
		char name[10];
	};

	struct SignData
	{
		char name[10];
		char pass[10];
		short ID;
		SOCKET sock;
		bool onlineflag;

		std::vector<SenderData>friendSenderSaveData;
		std::vector<User>friendList;

	};
	std::vector<SignData>signData;

#define TeamMax 10
	Team team[TeamMax];



	struct Message
	{
		UdpTag cmd;
		char text[32];
	};
	struct PlayerInput
	{
		TcpTag cmd;
		short id;
		DirectX::XMFLOAT3 velocity;
		DirectX::XMFLOAT3 position;
		Player::State state;
		DirectX::XMFLOAT3 angle;
	};
	



	struct PlayerLogin
	{
		TcpTag cmd;
		short id;
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
		short id;
	};
	struct PlayerSync
	{
		TcpTag cmd;
		short id;
		DirectX::XMFLOAT3 position;
		DirectX::XMFLOAT3 velocity;
		DirectX::XMFLOAT3 angle;
	};

	struct TeamCreate
	{
		TcpTag cmd;
		short id;
		int number;
		bool Permission;
	};

	struct Teamjoin
	{
		TcpTag cmd;
		short id;
		int number;
	};

	struct TeamLeave
	{
		TcpTag cmd;
		short id;
		bool isLeader;
	};

	struct Teamsync
	{
		TcpTag cmd;
		short id[4] = {};
	};

	struct StartCheck
	{
		TcpTag cmd;
		short id;
		short teamnunber;
		bool check;
	};

	struct GameStart
	{
		TcpTag cmd;
		short id;
		short teamnunber;
	};
	struct GameEnd
	{
		TcpTag cmd;
		short teamnunber;
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
	void Recieve(Client* client);
	void EraseClient(Client* client);

	void Login(SOCKET clientsock, short ID);
private:
	int id = 0;
	SOCKET sock = INVALID_SOCKET;
	std::vector<Client*> clients;
	std::vector<std::thread*> recvThreads;
	bool loop = true;

	int teamnumbergrant = 1000;
	

	struct sockaddr_in uAddr;
	SOCKET uSock = INVALID_SOCKET;
};