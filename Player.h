#pragma once
#pragma once
#include <DirectXMath.h>
class Player
{
public:
	enum class State
	{
		Idle,
		Move,
		Jump,
		Land,
		JumpFlip,
		Attack,
		Damage,
		Death,
		Revive,
		None,
	};

	short id = 0;
	DirectX::XMFLOAT3 position{ 0,0,0 };
	DirectX::XMFLOAT3 angle{ 0,0,0 };
	DirectX::XMFLOAT3 velocity{ 0,0,0 };
	State state = State::Idle;
	int teamnumber = 0;
	int teamID = 0;
};

