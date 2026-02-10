#include "pch.h"
#include "CollisionManager.h"

#include "World.h"

#include "BoxCollider2D.h"

void CollisionManager::Initialize()
{
	// 모든 그룹의 조합을 순회
	for (UINT i = 0; i < static_cast<UINT>(GROUP_TYPE::END); ++i)
	{
		for (UINT j = i; j < static_cast<UINT>(GROUP_TYPE::END); ++j)
			m_CollCheckArr[i][j] = m_CollCheckArr[j][i] = false;
	}
}

void CollisionManager::Update(float deltaTime, const std::vector<GameObject*>* gameObjects)
{
	// Player, Item, Environment, UI, ..
	for (UINT i = 0; i < static_cast<UINT>(GROUP_TYPE::END); ++i)
	{
		// Player, Item, Environment, UI, ..
		// j = i로 A -> B만 진행하고 B -> A는 하지 않는다
		for (UINT j = i; j < static_cast<UINT>(GROUP_TYPE::END); ++j)
		{
			// i 그룹과 j 그룹이 충돌체크가 필요한 조합인지 확인
			if (m_CollCheckArr[i][j] == true && m_CollCheckArr[j][i] == true)
			{
				// i 그룹과 j 그룹의 오브젝트 리스트를 가져온다
				const std::vector<GameObject*>& thisGroup  = gameObjects[i];
				const std::vector<GameObject*>& otherGroup = gameObjects[j];

				// 그룹간 충돌체크
				CollisionGroupUpdate(thisGroup, otherGroup, deltaTime);
			}
		}
	}
}

void CollisionManager::Finalize()
{
}

void CollisionManager::EnableCollisionType(GROUP_TYPE left, GROUP_TYPE right)
{
	m_CollCheckArr[static_cast<UINT>(left)][static_cast<UINT>(right)] = true;
	m_CollCheckArr[static_cast<UINT>(right)][static_cast<UINT>(left)] = true;
}

void CollisionManager::CollisionGroupUpdate(const std::vector<GameObject*>& thisGroup, const std::vector<GameObject*>& otherGroup, float deltaTime)
{
	// thisGroup 순회
	for (UINT i = 0; i < thisGroup.size(); ++i)
	{
		// thisGroup의 모든 충돌체를 가져온다
		const std::vector<Collider2D*> thisColliders = thisGroup[i]->GetComponents<Collider2D>();
		for(Collider2D* thisCollider : thisColliders)
		{
			// 충돌체를 찾지 못했으면 다음 충돌체로 넘어간다
			if (!thisCollider)
				continue;

			// 같은 그룹이면 중복 방지를 위해 j=i 부터 시작
			const UINT startJ = (thisGroup == otherGroup) ? i : 0;

			// otherGroup 순회
			for (UINT j = startJ; j < otherGroup.size(); ++j)
			{
				// 같은 오브젝트끼리는 검사할 필요가 없다
				if (thisGroup[i] == otherGroup[j])
					continue;

				// otherGroup의 모든 충돌체를 가져온다
				const std::vector<Collider2D*> otherColliders = otherGroup[j]->GetComponents<Collider2D>();
				for (Collider2D* otherCollider : otherColliders)
				{
					// 충돌체를 찾지 못했으면 다음 충돌체로 넘어간다
					if (!otherCollider)
						continue;

					/// 두 충돌체 아이디 생성 left + right
					COLLIDER_ID colliderID = {};

					// 두 충돌체의 ID를 가져온다 -> left와 right의 ID를 바탕으로 ID를 생성한다 (Union : 같은 메모리공간 사용)
					colliderID.left_ID = thisCollider->GetID();
					colliderID.right_ID = otherCollider->GetID();

					// 충돌체의 ID를 찾는다
					auto iter = m_mapCollisionInfo.find(colliderID.ID);

					// 충돌 ID가 없으면 새로 생성
					if (m_mapCollisionInfo.end() == iter)
					{
						// map에 충돌 ID, 충돌 여부를 저장
						m_mapCollisionInfo.insert(std::make_pair(colliderID.ID, false));

						// 충돌 ID의 Iterator를 가져온다
						iter = m_mapCollisionInfo.find(colliderID.ID);
					}

					////////////////////////////////////////////////////////////////////////////////////
					//					iter->second : 두 충돌체 사이의 충돌 여부						  //
					////////////////////////////////////////////////////////////////////////////////////

					// 두 콜라이더가 충돌한 경우
					if (thisCollider->CheckIntersect(otherCollider))
					{
						// 충돌 타입이 Collision인 경우 충돌하지만 But 하나라도 NONE이면 충돌 X
						if ((thisCollider->m_CollisionType == COLLISION_TYPE::COLLISION && otherCollider->m_CollisionType == COLLISION_TYPE::COLLISION) &&
								(thisCollider->m_CollisionType != COLLISION_TYPE::NONE && otherCollider->m_CollisionType != COLLISION_TYPE::NONE))
						{
							// 이미 충돌해 있는 경우
							if (iter->second)
							{
								// IsCollision = true
								thisCollider->TurnOn_IsCollision(otherCollider);

								iter->second = true;

								thisCollider->OnCollisionStay(otherCollider);
								otherCollider->OnCollisionStay(thisCollider);
							}
							// 처음 충돌하는 경우
							else
							{
								// IsCollision = true
								thisCollider->TurnOn_IsCollision(otherCollider);

								iter->second = true;

								thisCollider->OnCollisionEnter(thisCollider, otherCollider, iter);
								otherCollider->OnCollisionEnter(otherCollider, thisCollider, iter);
							}

							// A 콜라이더->ProcessBlock(B 콜라이더) : A 콜라이더는 B 콜라이더에 대해 "밀려난다"
							thisCollider->ProcessBlock(otherCollider);

							// 플레이어끼리 밀리도록
							if(thisCollider->GetOwner()->GetGroupType() == GROUP_TYPE::PLAYER && otherCollider->GetOwner()->GetGroupType() == GROUP_TYPE::PLAYER)
								otherCollider->ProcessBlock(thisCollider);
						}
						// 충돌 타입이 Trigger인 경우 (한 콜라이더라도 TRIGGER면 트리거 발동)
						else if ((thisCollider->m_CollisionType == COLLISION_TYPE::TRIGGER || otherCollider->m_CollisionType == COLLISION_TYPE::TRIGGER) &&
									(thisCollider->m_CollisionType != COLLISION_TYPE::NONE && otherCollider->m_CollisionType != COLLISION_TYPE::NONE))
						{
							// 이미 충돌해 있는 경우
							if (iter->second)
							{
								thisCollider->TurnOn_IsTrigger(otherCollider);

								iter->second = true;

								thisCollider->OnTriggerStay(otherCollider);
								otherCollider->OnTriggerStay(thisCollider);
							}
							// 처음 충돌하는 경우
							else
							{
								thisCollider->TurnOn_IsTrigger(otherCollider);

								iter->second = true;

								thisCollider->OnTriggerEnter(thisCollider, otherCollider, iter);
								otherCollider->OnTriggerEnter(otherCollider, thisCollider, iter);
							}
						}
					}
					// 두 콜라이더가 충돌하지 않은 경우
					else
					{
						// 둘 다 충돌 타입이 Collision인 경우 + 둘 다 NONE이 아닌 경우
						if ((thisCollider->m_CollisionType == COLLISION_TYPE::COLLISION && otherCollider->m_CollisionType == COLLISION_TYPE::COLLISION) &&
								(thisCollider->m_CollisionType != COLLISION_TYPE::NONE && otherCollider->m_CollisionType != COLLISION_TYPE::NONE))
						{
							// 이미 충돌해 있었던 경우
							if (iter->second)
							{
								thisCollider->TurnOff_IsCollision(otherCollider);

								iter->second = false;

								thisCollider->OnCollisionExit(otherCollider);
								otherCollider->OnCollisionExit(thisCollider);
							}
							// 원래도 충돌하지 않았던 경우
							else
							{
								iter->second = false;
							}
						}
						// 충돌 타입이 Trigger인 경우
						else if ((thisCollider->m_CollisionType == COLLISION_TYPE::TRIGGER || otherCollider->m_CollisionType == COLLISION_TYPE::TRIGGER) &&
									(thisCollider->m_CollisionType != COLLISION_TYPE::NONE && otherCollider->m_CollisionType != COLLISION_TYPE::NONE))
						{
							// 이미 충돌해 있었던 경우
							if (iter->second)
							{
								thisCollider->TurnOff_IsTrigger(otherCollider);

								iter->second = false;

								thisCollider->OnTriggerExit(otherCollider);
								otherCollider->OnTriggerExit(thisCollider);
							}
							// 원래도 충돌하지 않았던 경우
							else
							{
								iter->second = false;
							}
						}
					}
				}
			}
		}
	}
}