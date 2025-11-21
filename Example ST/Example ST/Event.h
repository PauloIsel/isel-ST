
#if !defined(EVENT_H)
#define EVENT_H

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000


#include "AfxTempl.h"

typedef enum 
{
	SETUP, 
	RELEASE, 
	STATISTIC
} EventType;


class CSimEvent
{
public:
	double			m_time;
	EventType		m_type;
	int				m_arrayPos;

	void*			m_pData;
public:
	CSimEvent(double time, EventType type, void*pData=NULL );
	~CSimEvent(){ if(m_pData) delete m_pData;};

	void SetData(void* pData) {m_pData = pData;};
	void* GetData() {return m_pData;};
};

const int ArraySize = 300;

class CSimEventManager
{
	CList<CSimEvent*, CSimEvent*> m_eventList;

	POSITION m_positionArray[ArraySize];
	double m_Tmin, m_Tmax;

public:
	CSimEventManager();
	~CSimEventManager();
	void Reset();
	CSimEvent* NextEvent();
	void AddEvent(CSimEvent* pEvent);
private:
};

#endif 
