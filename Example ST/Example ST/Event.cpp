#include "stdafx.h"
#include "Event.h"


#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

CSimEvent::CSimEvent(double time, EventType type, void* pData)
{
	m_arrayPos = -1;
	m_time	= time;
	m_type  = type;

	m_pData = pData;
}


CSimEventManager::CSimEventManager()
{


}

CSimEvent* CSimEventManager::NextEvent()
{
	CSimEvent* pEvent = m_eventList.GetHead(); 
	ASSERT(pEvent->m_arrayPos >=0  && pEvent->m_arrayPos <ArraySize);
	if(m_positionArray[pEvent->m_arrayPos] == m_eventList.GetHeadPosition())
		m_positionArray[pEvent->m_arrayPos]=NULL;

	return m_eventList.RemoveHead();
}

void CSimEventManager::Reset()
{
	while(m_eventList.IsEmpty()==false)
		delete m_eventList.RemoveHead();

	m_Tmin = m_Tmax = 0;
	for(int i=0; i<ArraySize; i++)
		m_positionArray[i]=NULL;
}

void CSimEventManager::AddEvent(CSimEvent *pEvent)
{
	int index=0;
	POSITION searchPos, aux;
	int arrayIndex;

	if(m_eventList.IsEmpty())
	{
		m_eventList.AddHead(pEvent);
		pEvent->m_arrayPos=0;
		return;
	}

	m_Tmin = (m_eventList.GetHead())->m_time;

	if(pEvent->m_time < m_Tmin)
	{
		m_Tmin		= pEvent->m_time;
		arrayIndex	= 0;
		m_positionArray[arrayIndex] = m_eventList.GetHeadPosition();
	}
	else if(pEvent->m_time > m_Tmax)
	{
		m_Tmax		= pEvent->m_time;
		arrayIndex	= ArraySize-1;
		m_positionArray[arrayIndex] = m_eventList.GetTailPosition();
	}
 	else
	{
		arrayIndex = (int)((pEvent->m_time-m_Tmin)*(ArraySize-1)/(m_Tmax-m_Tmin));
		ASSERT(arrayIndex >= 0 && arrayIndex<ArraySize);
	};
	
	if(m_positionArray[arrayIndex] == NULL)
	{
		if(((float)arrayIndex)/((float)ArraySize) > 0.5)
			searchPos = m_eventList.GetTailPosition();
		else
			searchPos = m_eventList.GetHeadPosition();
	}else
		searchPos = m_positionArray[arrayIndex];
	
	CSimEvent* ev = m_eventList.GetAt(searchPos);
	if(ev->m_time >= pEvent->m_time ) { //reverse search
		m_eventList.GetPrev(searchPos);
		while(searchPos){
			aux = searchPos;
			ev	= m_eventList.GetPrev(searchPos);
			if( ev->m_time <= pEvent->m_time ) {
				searchPos = m_eventList.InsertAfter(aux, pEvent);
				break;
			}
			index--;
		};
		if( searchPos == NULL )
			searchPos = m_eventList.AddHead(pEvent);
	}
	else{							//forward search
		m_eventList.GetNext(searchPos);
		while( searchPos ) {
			aux = searchPos;
			ev = m_eventList.GetNext(searchPos);
			index++;
			if( ev->m_time >= pEvent->m_time ) {
				searchPos = m_eventList.InsertBefore(aux, pEvent);
				break;
			}
		}
		if( searchPos == NULL )
			searchPos = m_eventList.AddTail(pEvent);
	}

	pEvent->m_arrayPos = arrayIndex;
	m_positionArray[arrayIndex] = searchPos;
}

CSimEventManager::~CSimEventManager()
{
	while(!m_eventList.IsEmpty()){
		delete m_eventList.RemoveHead();
	}
};