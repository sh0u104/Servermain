#pragma once
#include <DirectXMath.h>
class Player
{
public:

	enum class State
	{
		Idle,
		Move,
		Land,
		Jump,
		JumpFlip,
	};
	//enum class State
	//{
	//	Idle,
	//	Move,
	//	Jump,
	//	Land,
	//	JumpFlip,
	//	Attack,
	//	Damage,
	//	Death,
	//	Revive,
	//	None,
	//};

	int id = -1;
	DirectX::XMFLOAT3 position{ 0,0,0 };
	DirectX::XMFLOAT3 angle{ 0,0,0 };
	DirectX::XMFLOAT3 velocity{ 0,0,0 };
	State state = State::Idle;

	//4Œ…
	int teamnumber = 0;
	//1Œ…
	int teamGrantID = 0;
};

