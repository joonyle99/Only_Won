#include "pch.h"
#include "PlayerMovement.h"

#include "GameObject.h"
#include "InputManager.h"
#include "SceneComponent.h"
#include "EventManager.h"

void PlayerMovement::KeyDirControl()
{
	// MoveDir과 LookDir을 키보드 입력값으로 설정한다

	const float xInupt = InputManager::GetInstance()->GetAxisRaw("Horizontal");
	const float yInupt = InputManager::GetInstance()->GetAxisRaw("Vertical");

	m_MoveDir = { xInupt, yInupt };
	m_MoveDir.Normalize();

	// 마지막 LookDir을 보관하는 방법??

	const framework::Vector2D vec = { xInupt, yInupt };

	// 입력이 있을때만 LookDir을 설정한다
	if(vec.Length() >= 0.1f)
		m_LookDir = { xInupt, yInupt };

	m_LookDir.Normalize();
}

void PlayerMovement::TotalDirControl(int controllerIndex)
{
	MoveDirControl(controllerIndex);
	LookDirControl(controllerIndex);
}

void PlayerMovement::MoveDirControl(int controllerIndex)
{
	// 이전 이동 방향을 기록
	m_PrevMoveDir = m_MoveDir;

	// 이동 방향 초기화
	m_MoveDir = { 0.f, 0.f };

	// 왼쪽 / 오른쪽 스틱 입력값
	const framework::Vector2D rawThumbLeft = InputManager::GetPadAxisLeftThumb(controllerIndex);
	const framework::Vector2D rawThumbRight = InputManager::GetPadAxisRightThumb(controllerIndex);

	// 보정된 왼쪽 스틱 입력값
	framework::Vector2D thumbLeft = { max(-1, rawThumbLeft.x), max(-1, (-1) * rawThumbLeft.y) };

	// 입력값에 따른 이동 방향 설정
	m_MoveDir = { thumbLeft.x, thumbLeft.y };

	// 이동 방향의 단위 벡터를 구한다
	m_MoveDir.Normalize();

	// 오른쪽 스틱의 입력값이 없는 경우 이동 방향을 바라보는 방향으로 설정한다
	if (rawThumbRight.x == 0.f || rawThumbRight.y == 0.f)
	{
		// 이동 방향이 없는 경우에만 바라보는 방향으로 설정한다 (자동으로 이동방향 없으면 바라보는 방향은 이전 바라보는 방향으로 설정됨)
		if (m_MoveDir != framework::Vector2D::Zero())
			m_LookDir = m_MoveDir;
	}
}

void PlayerMovement::LookDirControl(int controllerIndex)
{
	// 오른쪽 스틱 입력값
	const framework::Vector2D rawThumbRight = InputManager::GetPadAxisRightThumb(controllerIndex);

	// 보정된 오른쪽 스틱의 입력값
	framework::Vector2D thumbRight = { max(-1, rawThumbRight.x), max(-1, (-1) * rawThumbRight.y) };

	// 오른쪽 스틱에 입력값이 있는 경우에만 방향을 바꿔준다
	if (thumbRight.x != 0.f || thumbRight.y != 0.f)
		m_LookDir = { thumbRight.x, thumbRight.y };

	// 바라보는 방향의 단위 벡터를 구한다
	m_LookDir.Normalize();
}

void PlayerMovement::PlayerMove(float deltaTime)
{
	/// 가속도 (속도의 변화율) 계산
	m_OriginalAccel = m_MoveDir * m_OriginalAccelSpeed * m_ReverseCoefficient;

	/// 입력 방향이 바뀌면 빨리 반대로 갈 수 있도록
	if (m_PrevMoveDir.x * m_MoveDir.x < 0.f)
		m_OriginalVelocity.x = 0.f;
	if (m_PrevMoveDir.y * m_MoveDir.y < 0.f)
		m_OriginalVelocity.y = 0.f;

	/// 속도 (위치의 변화율) 계산 - Original과 External 분리 적용
	m_OriginalVelocity += m_OriginalAccel * deltaTime;
	m_ExternalVelocity += m_ExternalAccel * deltaTime;

	// Velocity Limit
	m_OriginalVelocity.Limit(m_OriginalVelocityLimit);
	m_ExternalVelocity.Limit(m_ExternalVelocityLimit);

	// 이동 입력 없으면 마찰력 작용
	if (m_MoveDir == framework::Vector2D::Zero())
		OriginalFriction(deltaTime);

	// 외부 속도에 대한 마찰력은 항시 작동
	ExternalFriction(deltaTime);

	// 마찰력 적용 후 TotalVelocity 계산
	m_TotalVelocity = m_OriginalVelocity + m_ExternalVelocity;

	// 변화되는 속도로 플레이어를 이동
	m_pOwner->GetRootComponent()->AddRelativeLocation(m_TotalVelocity * deltaTime);
}

void PlayerMovement::OriginalFriction(float deltaTime)
{
	// OriginalFriction
	if (m_OriginalVelocity.Length() <= 50.f)
		m_OriginalVelocity = framework::Vector2D::Zero();
	else
	{
		const framework::Vector2D friction1 = m_OriginalVelocity.GetNormalize() * (-1) * m_OriginalAccelSpeed;
		m_OriginalVelocity += friction1 * deltaTime;
	}
}

void PlayerMovement::ExternalFriction(float deltaTime)
{
	// ExternalFriction
	if (m_ExternalVelocity.Length() <= 50.f)
		m_ExternalVelocity = framework::Vector2D::Zero();
	else
	{
		const framework::Vector2D friction2 = m_ExternalVelocity.GetNormalize() * (-1) * m_ExternalAccelSpeed;
		m_ExternalVelocity += friction2 * deltaTime;
	}
}

void PlayerMovement::KnockBack(framework::Vector2D dir, float power)
{
	m_ExternalVelocity = dir * power;
}

void PlayerMovement::ReverseMove()
{
	if (!m_IsReverseMove)
	{
		m_ReverseCoefficient = -1.f;
		m_IsReverseMove = true;
	}
	else
	{
		m_ReverseCoefficient = 1.f;
		m_IsReverseMove = false;
	}
}

void PlayerMovement::Update(const float deltaTime)
{
	// 활성화 상태 아니면 아무것도 하지 않는다
	if (!m_IsActive)
		return;

	// 1P ~ 4P까지의 컨트롤러 입력을 받아온다
	if (m_pOwner->GetName() == L"Player1")
	{
		TotalDirControl(0);

		// 컨트롤러가 연결되어 있지만, 왼쪽 스틱과 오른쪽 스틱 모두 입력이 없는 경우에 키보드로 실행
		if((InputManager::GetPadAxisLeftThumb(0).Length() < 0.001f) && (InputManager::GetPadAxisRightThumb(0).Length() < 0.001f))
		{
			KeyDirControl();
		}
	}
	else if (m_pOwner->GetName() == L"Player2")
		TotalDirControl(1);
	else if (m_pOwner->GetName() == L"Player3")
		TotalDirControl(2);
	else if (m_pOwner->GetName() == L"Player4")
		TotalDirControl(3);

	PlayerMove(deltaTime);

	/// Transition에 정보 전달
	const framework::EVENT_MOVEMENT_INFO movementInfo = { m_MoveDir, m_LookDir };

	if (m_pOwner->GetName() == L"Player1")
		EventManager::GetInstance()->SendEvent(eEventType::P1TransperMovement, movementInfo);
	else if (m_pOwner->GetName() == L"Player2")
		EventManager::GetInstance()->SendEvent(eEventType::P2TransperMovement, movementInfo);
	else if (m_pOwner->GetName() == L"Player3")
		EventManager::GetInstance()->SendEvent(eEventType::P3TransperMovement, movementInfo);
	else if (m_pOwner->GetName() == L"Player4")
		EventManager::GetInstance()->SendEvent(eEventType::P4TransperMovement, movementInfo);
}