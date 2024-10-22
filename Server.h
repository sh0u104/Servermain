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

	enum class NetworkTag : unsigned short
	{
		Message,		// チャットメッセージ
		Move,			// 移動
		Attack,			// 攻撃
		Sync,			// 同期
		TeamCreate,     //チーム作成
		Teamjoin,       //チーム加入
		Teamleave,      //チームを抜ける
		Teamsync,       // チームにIDを送信
		StartCheck,     //スタート準備ができてるか
		Gamestart,       //ゲームを始めでいいか
		GameEnd,        //ゲームが終わったら
		SignUp,			//アカウント作成
		SignIn,			//アカウントにログイン
		GeustLogin,		// サーバーにログイン
		Login,          //受信用
		Logout,			// サーバーからログアウト
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
		NetworkTag cmd;
		char text[32];
	};


	struct PlayerInput
	{
		NetworkTag cmd;
		short id;
		DirectX::XMFLOAT3 velocity;
		DirectX::XMFLOAT3 position;
		Player::State state;
		DirectX::XMFLOAT3 angle;
	};
	
	struct PlayerLogin
	{
		NetworkTag cmd;
		short id;
	};

	struct GeustLogin
	{
		NetworkTag cmd;
	};

	struct SignIn
	{
		NetworkTag cmd;
		char name[10];
		char pass[10];
		bool result;
	};

	struct SignUp
	{

		NetworkTag cmd;
		char name[10];
		char pass[10];
	};

	struct PlayerLogout
	{
		NetworkTag cmd;
		short id;
	};
	struct PlayerSync
	{
		NetworkTag cmd;
		short id;
		DirectX::XMFLOAT3 position;
		DirectX::XMFLOAT3 velocity;
		DirectX::XMFLOAT3 angle;
	};

	struct TeamCreate
	{
		NetworkTag cmd;
		short id;
		int number;
		bool Permission;
	};

	struct Teamjoin
	{
		NetworkTag cmd;
		short id;
		int number;
	};

	struct TeamLeave
	{
		NetworkTag cmd;
		short id;
		bool isLeader;
	};

	struct Teamsync
	{
		NetworkTag cmd;
		short id[4] = {};
	};

	struct StartCheck
	{
		NetworkTag cmd;
		short id;
		short teamnunber;
		bool check;
	};

	struct GameStart
	{
		NetworkTag cmd;
		short id;
		short teamnunber;
	};
	struct GameEnd
	{
		NetworkTag cmd;
		short teamnunber;
	};

	struct IdSearch
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

	};
	struct FriendApproval
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
	};

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