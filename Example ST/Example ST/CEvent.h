#pragma once

#include <queue>

typedef enum
{
	SETUP,
	RELEASE
} EventType;



class CEvent {
public:
	// Construct sets time of event.
	CEvent(double t, EventType type) : m_time(t), m_type(type){ }

	double		m_time;
	EventType	m_type;
};

struct eventComparator {
	bool operator() (const CEvent*  left, const CEvent*  right) const {
		return left->m_time > right->m_time;
	}
};

class CEventManager{
	std::priority_queue <CEvent*, std::vector<CEvent*, std::allocator<CEvent*> >, eventComparator> eventQueue;
	   
public:
	CEventManager();
	~CEventManager();
	void Reset();
	CEvent* NextEvent();
	void AddEvent(CEvent* pEvent);
};