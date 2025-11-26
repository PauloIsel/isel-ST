#pragma once

#include <list> // Substitui afxtempl.h para usar std::list

typedef enum
{
	SETUP,
	RELEASE
} EventType;

class CEvent
{
public:
	double			m_time;
	EventType		m_type;
	int				m_arrayPos;

	void* m_pData;
public:
	CEvent(double time, EventType type, void* pData = NULL);
	~CEvent() { if (m_pData) delete m_pData; };

	void SetData(void* pData) { m_pData = pData; };
	void* GetData() { return m_pData; };
};

const int ArraySize = 1000;

// Definição alternativa para POSITION usando std::list
typedef std::list<CEvent*>::iterator POSITION;

class CEventManager
{
	std::list<CEvent*> m_eventList;

	POSITION m_positionArray[ArraySize];
	double m_Tmin, m_Tmax;

public:
	CEventManager();
	~CEventManager();
	void Reset();
	CEvent* NextEvent();
	void AddEvent(CEvent* pEvent);
private:
};