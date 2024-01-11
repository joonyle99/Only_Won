#pragma once

/// <summary>
/// Resource ��� �󵵸� ��Ÿ���� ī����.
/// ���� ���ҽ��� ���� ����Ƚ���� �����ϰ�, RefCnt�� 0�� �� ��� ���ҽ��� �����Ѵ�.
/// </summary>
class ReferenceCounter
{
private:
	unsigned int m_RefCount;

public:
	ReferenceCounter() : m_RefCount(0) { }

	// virtual Ű���� ������� �ı��ڸ� �����Լ��� ����� delete �� ȣ���Ҷ� 
	// �ı��ڸ� ȣ���� �ڽ��� �ı��� -> �θ��� �ı��� ������ ȣ�� �ǵ��� �Ѵ�.
	// �ڽ�Ŭ�������� ������ ���·� ������ �ص� virtual ���¸� �����Ѵ�
	virtual ~ReferenceCounter() { }

	unsigned int AddRef()
	{
		m_RefCount++;
		return m_RefCount;
	}

	unsigned int Release()
	{
		m_RefCount--;
		if (m_RefCount == 0)
		{
			// �ı��ڿ� virtual Ű���带 ��������Ƿ� �ڽ�Ŭ������ �ı��ڰ� ȣ��ȴ�.
			delete this;
			return 0;
		}
		return m_RefCount;
	}

	unsigned int GetRefCount() const
	{
		return m_RefCount;
	}
};
