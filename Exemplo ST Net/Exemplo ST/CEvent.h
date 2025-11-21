#pragma once

#include "afxtempl.h"

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

class CEventManager
{
	CList<CEvent*, CEvent*> m_eventList;

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