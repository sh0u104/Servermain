#pragma once
class Enemy
{
public:

	// �X�e�[�g
	enum class State
	{
		Wander,
		Idle,
		Pursuit,
		Attack,
		IdleBattle,
		Damage,
		Death
	};

	short id = 0;
	DirectX::XMFLOAT3 position{ 0,0,0 };
	DirectX::XMFLOAT3 angle{ 0,0,0 };
	DirectX::XMFLOAT3 velocity{ 0,0,0 };
	State state = State::Wander;

	//4��
	int teamnumber = 0;
	//1��
	int teamID = 0;
};