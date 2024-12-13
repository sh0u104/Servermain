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
	// �O���錾
	struct Team;
	void UDPExecute();

	// �N���C�A���g���
	struct Client {
		struct sockaddr_in addr {};
		struct sockaddr_in uAddr {};
		SOCKET sock = INVALID_SOCKET;
		//Player* player = nullptr;
		std::shared_ptr<Player> player;
		bool Geustflag = false;
		bool isRecvUdpAddr = false;
		bool startCheck = false;
		Team* team = nullptr; // ��������`�[���������|�C���^
	};
	struct Team
	{
		std::vector<std::shared_ptr<Client>> clients;
		int TeamNumber = 0;
		bool isJoin = false;
	};
	enum class TcpTag : unsigned short
	{
		SignUp,         //�A�J�E���g�쐬
		SignIn,         //�A�J�E���g�Ń��O�C��
		GeustLogin,		// �Q�X�g�A�J�E���g�Ń��O�C��
		Logout,			// �T�[�o�[���烍�O�A�E�g
		TeamCreate,     //�`�[���쐬
		Teamjoin,       //�`�[������
		Teamleave,      //�`�[���𔲂���
		Teamsync,       // ��ɓ������l�Ƀ`�[�������o�[��ID��������
		StartCheck,     //�X�^�[�g�������ł��Ă邩
		Gamestart,      //�Q�[�����n�߂ł�����
		GameEnd,        //�Q�[�����I�������
		Move,
		Sync,			// ����
		Login,
		UdpAddr,          //�T�[�o�[��UDP�̃A�h���X�ۑ��p
		EnemyDamage,    //�G���_���[�W���󂯂���
		Message,
	};
	enum class UdpTag : unsigned short
	{
		Message,		// �`���b�g���b�Z�[�W
		Move,			// �ړ�
		EnemyMove,       //�G�̈ړ�
		Attack,			// �U��
		UdpAddr,          //�T�[�o�[��UDP�̃A�h���X�ۑ��p
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

	// �o�^�ς̃N���C�A���g�����f���s��
	bool HasSameData(const std::vector<std::shared_ptr<Client>>& vec, const sockaddr_in& target);
	//teams����client��team������
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