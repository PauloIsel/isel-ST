#include "pch.h"
#include "CEvent.h"



CEventManager::CEventManager()
{
	Reset();
}

void CEventManager::Reset()
{
	while (!eventQueue.empty()) {
		CEvent* pEvent = eventQueue.top();
		delete pEvent;
		eventQueue.pop();
	}
		
}

CEvent* CEventManager::NextEvent()
{
	CEvent * nextEvent = eventQueue.top();
	eventQueue.pop();
	return nextEvent;
}

void CEventManager::AddEvent(CEvent *pEvent)
{
	eventQueue.push(pEvent);
}

CEventManager::~CEventManager()
{
	Reset();
};