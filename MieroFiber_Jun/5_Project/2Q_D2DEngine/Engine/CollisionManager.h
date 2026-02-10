#pragma once

#include "GameObject.h"

#include <list>
#include <vector>
#include <unordered_map>

class Collider2D;
class World;

union COLLIDER_ID
{
	struct
	{
		UINT left_ID;
		UINT right_ID;
	};

	ULONGLONG ID;
};

class CollisionManager
{
public:
	void Initialize();
	void Update(float deltaTime, const std::vector<GameObject*>* gameObjects);
	void Finalize();

public:
	void EnableCollisionType(GROUP_TYPE left, GROUP_TYPE right);

private:
	void CollisionGroupUpdate(const std::vector<GameObject*>& thisGroup, const std::vector<GameObject*>& otherGroup, float deltaTime);

private:
	std::unordered_map<ULONGLONG, bool> m_mapCollisionInfo;
	bool m_CollCheckArr[static_cast<UINT>(GROUP_TYPE::END)][static_cast<UINT>(GROUP_TYPE::END)] = {};
};