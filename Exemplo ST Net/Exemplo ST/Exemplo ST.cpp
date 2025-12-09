// ST_callcenter_tuner_window.cpp
#include <iostream>
#include <fstream>
#include <queue>
#include <vector>
#include <cmath>
#include <cstring>
#include <limits>
#include <cstdio>
#include "CEvent.h"

CEventManager eventManager;

class CSwitch;

struct CallData {
    long callNumber;
    int entrySwitch;
    int route[4];
    double startTime;
    double serviceTime;
    double serviceStartTime;
};

struct SwitchLink {
    int numLines;
    int occupiedLines;
    double carriedCallsTime;
    double carriedCallsTimeSq;
    long carriedCallsCount;
    CSwitch* pNextSwitch;
};

class CSwitch {
    char name;
    long blockedCalls;
    long totalCalls;
    int nLinks;
    SwitchLink outLinks[2];
    std::ofstream outFile;

public:
    CSwitch() : name('\0'), blockedCalls(0), totalCalls(0), nLinks(0) {
        memset(&(outLinks), 0, sizeof(outLinks));
    }

    CSwitch(char cName, int nlines1, CSwitch* pSwitch1, int nlines2 = 0, CSwitch* pSwitch2 = nullptr)
        : name(cName), blockedCalls(0), totalCalls(0), nLinks(nlines2 == 0 ? 1 : 2) {
        memset(&(outLinks), 0, sizeof(outLinks));

        outLinks[0].numLines = nlines1; outLinks[0].pNextSwitch = pSwitch1; outLinks[0].occupiedLines = 0;
        outLinks[1].numLines = nlines2; outLinks[1].pNextSwitch = pSwitch2; outLinks[1].occupiedLines = 0;

        char fileName[100]; snprintf(fileName, sizeof(fileName), "switch_%c_.txt", cName);
        outFile.open(fileName, std::ofstream::out);
    }

    void Reset() {
        blockedCalls = totalCalls = 0;
        for (int i = 0; i < 2; i++) {
            outLinks[i].occupiedLines = 0;
            outLinks[i].carriedCallsTime = 0.0;
            outLinks[i].carriedCallsTimeSq = 0.0;
            outLinks[i].carriedCallsCount = 0;
        }
    }

    bool RequestLine(int hop, int route[4]) {
        totalCalls++;
        for (int i = 0; i < nLinks; i++) {
            if (outLinks[i].numLines == 0) continue;
            if (outLinks[i].occupiedLines < outLinks[i].numLines) {
                if (outLinks[i].pNextSwitch != nullptr)
                    if (!outLinks[i].pNextSwitch->RequestLine(hop + 1, route)) return false;
                outLinks[i].occupiedLines++;
                route[hop] = i;
                return true;
            }
        }
        blockedCalls++;
        return false;
    }

    void ReleaseLine(int hop, CallData* pCall, double currentTime) {
        double duration = currentTime - pCall->startTime;
        int idx = pCall->route[hop];
        if (outLinks[idx].pNextSwitch != nullptr)
            outLinks[idx].pNextSwitch->ReleaseLine(hop + 1, pCall, currentTime);
        if (outLinks[idx].occupiedLines > 0) outLinks[idx].occupiedLines--;
        outLinks[idx].carriedCallsTime += duration;
        outLinks[idx].carriedCallsTimeSq += duration * duration;
        outLinks[idx].carriedCallsCount++;
    }
};

CSwitch network[6];

struct Config {
    double bhca;
    double holdTime;
    int nLines_A_C, nLines_A_D, nLines_B_D, nLines_D_E, nLines_D_F;
    int nLines_C_E, nLines_E_F, nLines_F_CC;
    int numOperators;
    double simulationTime;

    Config() : bhca(0.0), holdTime(0.0), nLines_A_C(0), nLines_A_D(0), nLines_B_D(0),
        nLines_D_E(0), nLines_D_F(0), nLines_C_E(0), nLines_E_F(0), nLines_F_CC(0),
        numOperators(0), simulationTime(0.0) {
    }
};

struct Metrics {
    long totalCallsServed, totalArrivals, blockedCalls;
    double avgWaitingAll, avgWaitingWaited, operatorUtil, probWaitGT360, meanQueueSize;
};

struct CallRecord {
    double startTime;
    double waitingTime;
    double serviceTime;
};

struct RunState {
    Config cfg;
    long totalCalls, blockedCalls, servedCallsCount, waitedCallsCount, countWaitGT360;
    double carriedServiceTime, reqServiceTime, cumWaitingTimeAll, cumWaitingTimeWaited;
    double lastEventTime, queueArea, cumOperatorBusyTime;
    int busyOperators;
    std::queue<CallData*> waitingQueue;
    std::vector<CallRecord> callHistory;

    RunState() : cfg(), totalCalls(0), blockedCalls(0), servedCallsCount(0), waitedCallsCount(0),
        countWaitGT360(0), carriedServiceTime(0.0), reqServiceTime(0.0),
        cumWaitingTimeAll(0.0), cumWaitingTimeWaited(0.0), lastEventTime(0.0),
        queueArea(0.0), cumOperatorBusyTime(0.0), busyOperators(0) {
    }

    void ResetSwitches() { for (int i = 0; i < 6; i++) network[i].Reset(); }
};

static float urand() { float u; do { u = ((float)rand()) / (float)RAND_MAX; } while (u == 0.0f || u >= 1.0f); return u; }
static float expon(float media) { return (float)(-media * log(urand())); }

static void UpdateQueueArea(RunState& st, double currentTime) {
    if (currentTime > st.lastEventTime) {
        st.queueArea += (double)(long)st.waitingQueue.size() * (currentTime - st.lastEventTime);
        st.lastEventTime = currentTime;
    }
}

static void InitNetwork(const Config& cfg) {
    network[5] = CSwitch('F', cfg.nLines_F_CC, nullptr);
    network[4] = CSwitch('E', cfg.nLines_E_F, &(network[5]));
    network[3] = CSwitch('D', cfg.nLines_D_F, &(network[5]), cfg.nLines_D_E, &(network[4]));
    network[2] = CSwitch('C', cfg.nLines_C_E, &(network[4]));
    network[1] = CSwitch('B', cfg.nLines_B_D, &(network[3]));
    network[0] = CSwitch('A', cfg.nLines_A_D, &(network[3]), cfg.nLines_A_C, &(network[2]));
}

static void InitializeRun(RunState& st, const Config& cfg, unsigned int seed) {
    st.cfg = cfg; st.totalCalls = 0; st.blockedCalls = 0; st.carriedServiceTime = st.reqServiceTime = 0.0;
    while (!st.waitingQueue.empty()) st.waitingQueue.pop();
    st.busyOperators = st.servedCallsCount = st.waitedCallsCount = st.countWaitGT360 = 0;
    st.cumWaitingTimeAll = st.cumWaitingTimeWaited = st.lastEventTime = st.queueArea = st.cumOperatorBusyTime = 0.0;
    st.callHistory.clear();
    eventManager.Reset();
    InitNetwork(cfg);
    st.ResetSwitches();
    srand(seed);
    eventManager.AddEvent(new CEvent(expon(3600.0f / (float)cfg.bhca), SETUP));
}

static void SetupRun(RunState& st, CEvent* pEvent) {
    eventManager.AddEvent(new CEvent(pEvent->m_time + expon(3600.0f / (float)st.cfg.bhca), SETUP));

    CallData* pNewCall = new CallData;
    pNewCall->serviceTime = expon((float)st.cfg.holdTime);
    pNewCall->startTime = pEvent->m_time;
    pNewCall->entrySwitch = 2;
    pNewCall->callNumber = st.totalCalls;
    pNewCall->serviceStartTime = 0.0;

    st.totalCalls++;
    st.reqServiceTime += pNewCall->serviceTime;

    if (network[pNewCall->entrySwitch].RequestLine(0, pNewCall->route)) {
        UpdateQueueArea(st, pEvent->m_time);
        if (st.busyOperators < st.cfg.numOperators) {
            pNewCall->serviceStartTime = pEvent->m_time;
            st.busyOperators++;
            st.cumOperatorBusyTime += pNewCall->serviceTime;
            eventManager.AddEvent(new CEvent(pEvent->m_time + pNewCall->serviceTime, RELEASE, pNewCall));
        }
        else {
            st.waitingQueue.push(pNewCall);
        }
    }
    else {
        st.blockedCalls++;
        delete pNewCall;
    }
}

static void ReleaseRun(RunState& st, CEvent* pEvent) {
    CallData* pCall = (CallData*)pEvent->GetData();
    double currentTime = pEvent->m_time;

    network[pCall->entrySwitch].ReleaseLine(0, pCall, currentTime);

    st.carriedServiceTime += (currentTime - pCall->startTime);

    if (st.busyOperators > 0) st.busyOperators--;

    double waiting = 0.0;
    if (pCall->serviceStartTime > 0.0)
        waiting = pCall->serviceStartTime - pCall->startTime;

    st.cumWaitingTimeAll += waiting;
    if (waiting > 0.0) {
        st.cumWaitingTimeWaited += waiting;
        st.waitedCallsCount++;
    }
    if (waiting > 360.0) st.countWaitGT360++;

    st.callHistory.push_back({ pCall->startTime, waiting, pCall->serviceTime });

    st.servedCallsCount++;

    UpdateQueueArea(st, currentTime);

    if (!st.waitingQueue.empty()) {
        CallData* pNext = st.waitingQueue.front();
        st.waitingQueue.pop();

        pNext->serviceStartTime = currentTime;

        st.busyOperators++;
        st.cumOperatorBusyTime += pNext->serviceTime;

        eventManager.AddEvent(new CEvent(currentTime + pNext->serviceTime, RELEASE, pNext));
    }
}

static Metrics RunSimulation(const Config& cfg, RunState& st, unsigned int seed) {
    InitializeRun(st, cfg, seed);

    CEvent* pEvent = eventManager.NextEvent();
    while (pEvent && pEvent->m_time < cfg.simulationTime) {
        switch (pEvent->m_type) {
        case SETUP: SetupRun(st, pEvent); break;
        case RELEASE: ReleaseRun(st, pEvent); break;
        }
        delete pEvent;
        pEvent = eventManager.NextEvent();
    }

    Metrics m = {};
    m.totalArrivals = st.totalCalls;
    m.blockedCalls = st.blockedCalls;
    m.totalCallsServed = st.servedCallsCount;
    m.avgWaitingAll = (m.totalCallsServed > 0) ? st.cumWaitingTimeAll / m.totalCallsServed : 0.0;
    m.avgWaitingWaited = (st.waitedCallsCount > 0) ? st.cumWaitingTimeWaited / st.waitedCallsCount : 0.0;
    m.operatorUtil = st.cumOperatorBusyTime / (cfg.numOperators * cfg.simulationTime);
    m.probWaitGT360 = (m.totalCallsServed > 0) ? (double)st.countWaitGT360 / m.totalCallsServed : 0.0;
    double arrivalsToCC = (double)m.totalCallsServed / cfg.simulationTime;
    m.meanQueueSize = arrivalsToCC * m.avgWaitingAll;
    return m;
}

static Metrics ComputeWindowMetrics(const std::vector<CallRecord>& records, double windowStart, double windowEnd, int numOperators)
{
    Metrics m = {};
    long waitedCount = 0;
    double sumWaitAll = 0.0;
    double sumWaitWaited = 0.0;
    double busyTime = 0.0;
    long countWaitGT360 = 0;

    long servedCalls = 0;
    for (const auto& rec : records) {
        if (rec.startTime >= windowStart && rec.startTime < windowEnd) {
            servedCalls++;
            sumWaitAll += rec.waitingTime;
            if (rec.waitingTime > 0.0) {
                sumWaitWaited += rec.waitingTime;
                waitedCount++;
            }
            if (rec.waitingTime > 360.0) countWaitGT360++;
            busyTime += rec.serviceTime;
        }
    }

    m.totalCallsServed = servedCalls;
    m.avgWaitingAll = (servedCalls > 0) ? (sumWaitAll / servedCalls) : 0.0;
    m.avgWaitingWaited = (waitedCount > 0) ? (sumWaitWaited / waitedCount) : 0.0;
    m.probWaitGT360 = (servedCalls > 0) ? (double)countWaitGT360 / servedCalls : 0.0;
    m.operatorUtil = busyTime / (numOperators * (windowEnd - windowStart));
    double arrivalsToWindow = (double)servedCalls / (windowEnd - windowStart);
    m.meanQueueSize = arrivalsToWindow * m.avgWaitingAll;

    return m;
}

int main() {
    Config cfg;
    cfg.bhca = 1560.0 * 0.15; cfg.holdTime = 100.0;
    cfg.nLines_A_C = 0; cfg.nLines_A_D = 0; cfg.nLines_B_D = 0;
    cfg.nLines_D_E = 0; cfg.nLines_D_F = 0;
    cfg.nLines_C_E = 1000; cfg.nLines_E_F = 1000;
    cfg.nLines_F_CC = 1000;
    cfg.numOperators = 7;
    cfg.simulationTime = 24 * 3600.0;

    RunState st;
    Metrics overall = RunSimulation(cfg, st, 12345u);

    double windowStart = 12 * 3600.0, windowEnd = 13 * 3600.0;
    Metrics wm = ComputeWindowMetrics(st.callHistory, windowStart, windowEnd, cfg.numOperators);

    std::cout << "Overall (24h) arrivals=" << overall.totalArrivals << " blocked=" << overall.blockedCalls
        << " served=" << overall.totalCallsServed << " operatorUtil=" << overall.operatorUtil << "\n\n";

    std::cout << "Metrics 12:00-13:00:\n";
    std::cout << "  mean queue = " << wm.meanQueueSize << "\n";
    std::cout << "  avg waiting all = " << wm.avgWaitingAll << " s (" << wm.avgWaitingAll / 60.0 << " min)\n";
    std::cout << "  avg waiting waited = " << wm.avgWaitingWaited << " s (" << wm.avgWaitingWaited / 60.0 << " min)\n";
    std::cout << "  P(T>360s) = " << wm.probWaitGT360 << "\n";
    std::cout << "  served calls = " << wm.totalCallsServed << "\n";

    return 0;
}