// ST_callcenter_tuner_window.cpp
#include <iostream>
#include <fstream>
#include <queue>
#include <vector>
#include <cmath>
#include <cstring>
#include <limits>
#include <cstdio>
#include <utility>
#include <algorithm>
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
                bool ok = true;
                if (outLinks[i].pNextSwitch != nullptr) {
                    ok = outLinks[i].pNextSwitch->RequestLine(hop + 1, route);
                }
                if (!ok) {
                    // este ramo n o conseguiu alocar recursivamente   tenta a pr xima rota
                    continue;
                }
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

    // Getters para m tricas de bloqueio
    char getName() const { return name; }
    long getBlockedCalls() const { return blockedCalls; }
    long getTotalCalls() const { return totalCalls; }
    double getBlockingProbability() const {
        return (totalCalls > 0) ? (double)blockedCalls / totalCalls : 0.0;
    }
};

CSwitch network[6];

// Estrutura para armazenar m tricas de bloqueio por comutador
struct SwitchBlockingMetrics {
    char name;
    long totalCalls;
    long blockedCalls;
    double blockingProbability;
};

struct Config {
    double bhca;
    double holdTime;
    int nLines_A_C, nLines_A_D, nLines_B_D, nLines_D_E, nLines_D_F;
    int nLines_C_E, nLines_E_F, nLines_F_CC;
    int numOperators;
    double simulationTime;

    // Propor  o de tr fego por origem (A, B, C) - deve somar 1.0
    double trafficRatio_A, trafficRatio_B, trafficRatio_C;

    Config() : bhca(0.0), holdTime(0.0), nLines_A_C(0), nLines_A_D(0), nLines_B_D(0),
        nLines_D_E(0), nLines_D_F(0), nLines_C_E(0), nLines_E_F(0), nLines_F_CC(0),
        numOperators(0), simulationTime(0.0),
        trafficRatio_A(0.33), trafficRatio_B(0.33), trafficRatio_C(0.34) {
    }
};

struct Metrics {
    long totalCallsServed, totalArrivals, blockedCalls;
    double avgWaitingAll, avgWaitingWaited, operatorUtil, probWaitGT360, meanQueueSize, carriedErlangs;
    double avgOperatorsBusy;
    long arrivalsInWindow;
    long blockedInWindow;
    long servedInWindow;
    int maxOperatorsBusy;

    // M tricas de bloqueio por comutador D, E, F
    SwitchBlockingMetrics switchD, switchE, switchF;
    double maxBlockingDiff; // Diferen a m xima de bloqueio entre D, E, F
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

    std::vector<double> arrivalTimes;
    std::vector<double> blockedTimes;
    std::vector<std::pair<double, int>> operatorUsageEvents;

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
    // Rede: A, B, C s o origens ( ndices 0, 1, 2)
    // D, E, F s o comutadores intermedi rios/destino ( ndices 3, 4, 5)
    // F conecta ao Call Center (destino final - Ponto 4)
    network[5] = CSwitch('F', cfg.nLines_F_CC, nullptr);
    network[4] = CSwitch('E', cfg.nLines_E_F, &(network[5]));
    network[3] = CSwitch('D', cfg.nLines_D_F, &(network[5]), cfg.nLines_D_E, &(network[4]));
    network[2] = CSwitch('C', cfg.nLines_C_E, &(network[4]));
    network[1] = CSwitch('B', cfg.nLines_B_D, &(network[3]));
    network[0] = CSwitch('A', cfg.nLines_A_D, &(network[3]), cfg.nLines_A_C, &(network[2]));
}

// Seleciona o switch de entrada basado nas propor  es de tr fego
static int SelectEntrySwitch(const Config& cfg) {
    float r = urand();
    if (r < cfg.trafficRatio_A) return 0;       // Switch A
    if (r < cfg.trafficRatio_A + cfg.trafficRatio_B) return 1;  // Switch B
    return 2;  // Switch C
}

static void InitializeRun(RunState& st, const Config& cfg, unsigned int seed) {
    st.cfg = cfg; st.totalCalls = 0; st.blockedCalls = 0; st.carriedServiceTime = st.reqServiceTime = 0.0;
    while (!st.waitingQueue.empty()) st.waitingQueue.pop();
    st.busyOperators = st.servedCallsCount = st.waitedCallsCount = st.countWaitGT360 = 0;
    st.cumWaitingTimeAll = st.cumWaitingTimeWaited = st.lastEventTime = st.queueArea = st.cumOperatorBusyTime = 0.0;
    st.callHistory.clear();
    st.arrivalTimes.clear();
    st.blockedTimes.clear();
    st.operatorUsageEvents.clear();
    eventManager.Reset();
    InitNetwork(cfg);
    st.ResetSwitches();
    srand(seed);
    eventManager.AddEvent(new CEvent(expon(3600.0f / (float)cfg.bhca), SETUP));
}

static void RecordOperatorEvent(RunState& st, double time) {
    st.operatorUsageEvents.push_back(std::make_pair(time, st.busyOperators));
}

static void SetupRun(RunState& st, CEvent* pEvent) {
    // Gerar próxima chegada apenas se ainda dentro do tempo de simulação
    double nextArrival = pEvent->m_time + expon(3600.0f / (float)st.cfg.bhca);
    if (nextArrival < st.cfg.simulationTime) {
        eventManager.AddEvent(new CEvent(nextArrival, SETUP));
    }

    st.arrivalTimes.push_back(pEvent->m_time);

    CallData* pNewCall = new CallData;
    pNewCall->serviceTime = expon((float)st.cfg.holdTime);
    pNewCall->startTime = pEvent->m_time;

    // Seleciona origem baseado nas propor  es configuradas (Pontos 1(A), 2(B), 3(C))
    pNewCall->entrySwitch = SelectEntrySwitch(st.cfg);

    pNewCall->callNumber = st.totalCalls;
    pNewCall->serviceStartTime = 0.0;
    memset(pNewCall->route, 0, sizeof(pNewCall->route));

    st.totalCalls++;
    st.reqServiceTime += pNewCall->serviceTime;

    if (network[pNewCall->entrySwitch].RequestLine(0, pNewCall->route)) {
        UpdateQueueArea(st, pEvent->m_time);
        if (st.busyOperators < st.cfg.numOperators) {
            pNewCall->serviceStartTime = pEvent->m_time;
            st.busyOperators++;
            RecordOperatorEvent(st, pEvent->m_time);
            st.cumOperatorBusyTime += pNewCall->serviceTime;
            eventManager.AddEvent(new CEvent(pEvent->m_time + pNewCall->serviceTime, RELEASE, pNewCall));
        }
        else {
            // Sistema de espera FIFO ideal com capacidade infinita (sem desist ncias)
            st.waitingQueue.push(pNewCall);
        }
    }
    else {
        st.blockedCalls++;
        st.blockedTimes.push_back(pEvent->m_time);
        delete pNewCall;
    }
}

static void ReleaseRun(RunState& st, CEvent* pEvent) {
    CallData* pCall = (CallData*)pEvent->GetData();
    double currentTime = pEvent->m_time;

    network[pCall->entrySwitch].ReleaseLine(0, pCall, currentTime);

    st.carriedServiceTime += (currentTime - pCall->startTime);

    if (st.busyOperators > 0) {
        st.busyOperators--;
        RecordOperatorEvent(st, currentTime);
    }

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

    // Sem tempos de transi  o entre atendimentos - atendimento imediato
    if (!st.waitingQueue.empty()) {
        CallData* pNext = st.waitingQueue.front();
        st.waitingQueue.pop();

        pNext->serviceStartTime = currentTime;

        st.busyOperators++;
        RecordOperatorEvent(st, currentTime);
        st.cumOperatorBusyTime += pNext->serviceTime;

        eventManager.AddEvent(new CEvent(currentTime + pNext->serviceTime, RELEASE, pNext));
    }

    // CRÍTICO: Limpar o ponteiro no evento ANTES de deletar o CallData
    // para evitar double-delete no destrutor de CEvent
    pEvent->SetData(nullptr);
    delete pCall;
}

// Calcula m tricas de bloqueio dos comutadores D, E, F
static void ComputeSwitchBlockingMetrics(Metrics& m) {
    // D = network[3], E = network[4], F = network[5]
    m.switchD.name = network[3].getName();
    m.switchD.totalCalls = network[3].getTotalCalls();
    m.switchD.blockedCalls = network[3].getBlockedCalls();
    m.switchD.blockingProbability = network[3].getBlockingProbability();

    m.switchE.name = network[4].getName();
    m.switchE.totalCalls = network[4].getTotalCalls();
    m.switchE.blockedCalls = network[4].getBlockedCalls();
    m.switchE.blockingProbability = network[4].getBlockingProbability();

    m.switchF.name = network[5].getName();
    m.switchF.totalCalls = network[5].getTotalCalls();
    m.switchF.blockedCalls = network[5].getBlockedCalls();
    m.switchF.blockingProbability = network[5].getBlockingProbability();

    // Calcular diferen a m xima de probabilidade de bloqueio entre D, E, F
    double pD = m.switchD.blockingProbability;
    double pE = m.switchE.blockingProbability;
    double pF = m.switchF.blockingProbability;

    double diff1 = std::abs(pD - pE);
    double diff2 = std::abs(pD - pF);
    double diff3 = std::abs(pE - pF);

    m.maxBlockingDiff = std::max({ diff1, diff2, diff3 }) * 100.0; // Em percentagem
}

static Metrics RunSimulation(const Config& cfg, RunState& st, unsigned int seed) {
    InitializeRun(st, cfg, seed);

    CEvent* pEvent = eventManager.NextEvent();
    while (pEvent && pEvent->m_time < cfg.simulationTime) {
        switch (pEvent->m_type) {
        case SETUP:
            SetupRun(st, pEvent);
            break;
        case RELEASE:
            ReleaseRun(st, pEvent);
            break;
        }
        delete pEvent;
        pEvent = eventManager.NextEvent();
    }

    // Limpar evento restante se existir
    if (pEvent) {
        // Se tem dados, limpar manualmente
        if (pEvent->m_pData) {
            delete static_cast<CallData*>(pEvent->m_pData);
            pEvent->SetData(nullptr); // Evitar double-delete
        }
        delete pEvent;
    }

    // Limpar chamadas que ficaram na fila de espera
    while (!st.waitingQueue.empty()) {
        CallData* pCall = st.waitingQueue.front();
        st.waitingQueue.pop();
        delete pCall;
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
    m.avgOperatorsBusy = st.cumOperatorBusyTime / cfg.simulationTime;
    m.carriedErlangs = st.carriedServiceTime / cfg.simulationTime;

    // Calcular métricas de bloqueio por comutador
    ComputeSwitchBlockingMetrics(m);

    return m;
}

static long CountInInterval(const std::vector<double>& times, double start, double end) {
    long cnt = 0;
    for (double t : times) if (t >= start && t < end) ++cnt;
    return cnt;
}

static int ComputeMaxOperatorsBusy(const std::vector<std::pair<double, int>>& events, double windowStart, double windowEnd) {
    int current = 0;
    int maxbusy = 0;
    for (const auto& ev : events) {
        if (ev.first <= windowStart) current = ev.second;
        else break;
    }
    maxbusy = current;
    for (const auto& ev : events) {
        if (ev.first >= windowEnd) break;
        if (ev.first >= windowStart && ev.first < windowEnd) {
            if (ev.second > maxbusy) maxbusy = ev.second;
        }
    }
    return maxbusy;
}

static Metrics ComputeWindowMetrics(const RunState& st, double windowStart, double windowEnd, int numOperators)
{
    Metrics m = {};
    long waitedCount = 0;
    double sumWaitAll = 0.0;
    double sumWaitWaited = 0.0;
    double busyTime = 0.0;
    long countWaitGT360 = 0;

    long servedCalls = 0;
    double totalServiceTime = 0.0;

    for (const auto& rec : st.callHistory) {
        double serviceStart = rec.startTime + rec.waitingTime;
        double serviceEnd = serviceStart + rec.serviceTime;

        double overlapStart = std::max(serviceStart, windowStart);
        double overlapEnd = std::min(serviceEnd, windowEnd);
        if (overlapEnd > overlapStart) busyTime += (overlapEnd - overlapStart);

        if (rec.startTime >= windowStart && rec.startTime < windowEnd) {
            servedCalls++;
            sumWaitAll += rec.waitingTime;
            totalServiceTime += rec.serviceTime;
            if (rec.waitingTime > 0.0) {
                sumWaitWaited += rec.waitingTime;
                waitedCount++;
            }
            if (rec.waitingTime > 360.0) countWaitGT360++;
        }
    }

    m.carriedErlangs = busyTime / (windowEnd - windowStart);
    m.totalCallsServed = servedCalls;
    m.avgWaitingAll = (servedCalls > 0) ? (sumWaitAll / servedCalls) : 0.0;
    m.avgWaitingWaited = (waitedCount > 0) ? (sumWaitWaited / waitedCount) : 0.0;
    m.probWaitGT360 = (servedCalls > 0) ? (double)countWaitGT360 / servedCalls : 0.0;

    m.operatorUtil = busyTime / (numOperators * (windowEnd - windowStart));
    m.avgOperatorsBusy = busyTime / (windowEnd - windowStart);

    double arrivalsToWindow = (double)servedCalls / (windowEnd - windowStart);
    m.meanQueueSize = arrivalsToWindow * m.avgWaitingAll;

    m.arrivalsInWindow = CountInInterval(st.arrivalTimes, windowStart, windowEnd);
    m.blockedInWindow = CountInInterval(st.blockedTimes, windowStart, windowEnd);
    m.servedInWindow = servedCalls;

    m.maxOperatorsBusy = ComputeMaxOperatorsBusy(st.operatorUsageEvents, windowStart, windowEnd);

    return m;
}

static std::vector<Metrics> ComputeTimeSeries(
    const RunState& st,
    double simTime,
    int numOperators,
    double intervalSeconds)
{
    std::vector<Metrics> series;

    for (double t = 0; t < simTime; t += intervalSeconds) {
        double wStart = t;
        double wEnd = t + intervalSeconds;

        Metrics m = ComputeWindowMetrics(st, wStart, wEnd, numOperators);
        series.push_back(m);
    }
    return series;
}

int main() {
    Config cfg;
    cfg.bhca = 1560.0;
    cfg.holdTime = 100.0;

    cfg.trafficRatio_A = 0.05;
    cfg.trafficRatio_B = 0.80;
    cfg.trafficRatio_C = 0.15;

    cfg.nLines_A_D = 9;
    cfg.nLines_A_C = 3;
    cfg.nLines_B_D = 47;
    cfg.nLines_C_E = 55;
    cfg.nLines_D_E = 28;
    cfg.nLines_D_F = 42;
    cfg.nLines_E_F = 61;
    cfg.nLines_F_CC = 67;
    cfg.numOperators = 49;

    cfg.simulationTime = 24 * 3600.0;

    std::cout << "============================" << std::endl;
    std::cout << "   SIMULACAO CALL CENTER" << std::endl;
    std::cout << "============================" << std::endl;

    RunState st;
    Metrics overall = RunSimulation(cfg, st, 12345u);

    // EXTRAIR MÉTRICAS DA HORA DE PONTA (12:00-13:00)
    double peakHourStart = 12 * 3600.0;  // 12:00
    double peakHourEnd = 13 * 3600.0;    // 13:00
    Metrics peakHour = ComputeWindowMetrics(st, peakHourStart, peakHourEnd, cfg.numOperators);

    // Para a hora de ponta, usar as métricas específicas dessa janela
    long countWaitLT360 = peakHour.totalCallsServed - (long)(peakHour.probWaitGT360 * peakHour.totalCallsServed);
    double probWaitLT360 = (peakHour.totalCallsServed > 0) ? (double)countWaitLT360 / peakHour.totalCallsServed : 0.0;
    double peakBlockingProb = (peakHour.arrivalsInWindow > 0) ? (double)peakHour.blockedInWindow / peakHour.arrivalsInWindow : 0.0;

    // Também calcular global (24h) para comparação
    double globalBlockingProb = (overall.totalArrivals > 0) ? (double)overall.blockedCalls / overall.totalArrivals : 0.0;

    auto series = ComputeTimeSeries(st, cfg.simulationTime, cfg.numOperators, 60.0);

    std::ofstream csv("stability_timeseries.csv");
    csv << "tempo;L(t);W_wait_avg_s;p_util;OperatorsBusyAvg;Arrivals;Blocked;Served;MaxOperatorsBusy;CarriedErlangs\n";

    double t = 0;
    for (auto& m : series) {
        csv << t << ";"
            << m.meanQueueSize << ";"
            << m.avgWaitingAll << ";"
            << m.operatorUtil << ";"
            << m.avgOperatorsBusy << ";"
            << m.arrivalsInWindow << ";"
            << m.blockedInWindow << ";"
            << m.servedInWindow << ";"
            << m.maxOperatorsBusy << ";"
            << m.carriedErlangs
            << "\n";
        t += 60.0;
    }
    csv.close();

    std::cout << "\n=== Dados de Entrada ===" << std::endl;
    std::cout << "  Chamadas/hora ponta: " << cfg.bhca << std::endl;
    std::cout << "  Duracao media atendimento: " << cfg.holdTime << " s" << std::endl;
    std::cout << "  Trafego oferecido: " << (cfg.bhca * cfg.holdTime / 3600.0) << " Erlangs" << std::endl;
    std::cout << "  Distribuicao: Ponto1(C)=" << (cfg.trafficRatio_C * 100) << "%, Ponto2(A)="
        << (cfg.trafficRatio_A * 100) << "%, Ponto3(B)=" << (cfg.trafficRatio_B * 100) << "%" << std::endl;

    std::cout << "\n=== Dimensionamento das Ligacoes ===" << std::endl;
    std::cout << "  A->D: " << cfg.nLines_A_D << " linhas" << std::endl;
    std::cout << "  A->C: " << cfg.nLines_A_C << " linhas" << std::endl;
    std::cout << "  B->D: " << cfg.nLines_B_D << " linhas" << std::endl;
    std::cout << "  C->E: " << cfg.nLines_C_E << " linhas" << std::endl;
    std::cout << "  D->E: " << cfg.nLines_D_E << " linhas" << std::endl;
    std::cout << "  D->F: " << cfg.nLines_D_F << " linhas" << std::endl;
    std::cout << "  E->F: " << cfg.nLines_E_F << " linhas" << std::endl;
    std::cout << "  F->CC: " << cfg.nLines_F_CC << " linhas" << std::endl;
    std::cout << "  Operadores: " << cfg.numOperators << std::endl;

    std::cout << "\n========================================" << std::endl;
    std::cout << "   METRICAS GLOBAIS (24 HORAS)" << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << "  Chegadas totais: " << overall.totalArrivals << std::endl;
    std::cout << "  Chamadas bloqueadas: " << overall.blockedCalls << std::endl;
    std::cout << "  Chamadas servidas: " << overall.totalCallsServed << std::endl;
    std::cout << "  Probabilidade bloqueio (24h): " << (globalBlockingProb * 100.0) << "%" << std::endl;
    std::cout << "  Tempo espera medio (24h): " << overall.avgWaitingAll << " s" << std::endl;

    std::cout << "\n========================================" << std::endl;
    std::cout << "   METRICAS DA HORA DE PONTA (12:00-13:00)" << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << "  Chegadas: " << peakHour.arrivalsInWindow << std::endl;
    std::cout << "  Chamadas bloqueadas: " << peakHour.blockedInWindow << std::endl;
    std::cout << "  Chamadas servidas: " << peakHour.totalCallsServed << std::endl;
    std::cout << "  Probabilidade bloqueio: " << (peakBlockingProb * 100.0) << "%" << std::endl;

    bool bloqueioOk = (peakBlockingProb <= 0.015);
    std::cout << "    -> Objetivo <= 1.5%: " << (bloqueioOk ? "[OK]" : "[FALHOU]") << std::endl;

    std::cout << "\n=== Metricas de Bloqueio por Comutador (24h) ===" << std::endl;
    std::cout << "  Switch D: " << overall.switchD.blockedCalls << "/" << overall.switchD.totalCalls
        << " = " << (overall.switchD.blockingProbability * 100.0) << "%" << std::endl;
    std::cout << "  Switch E: " << overall.switchE.blockedCalls << "/" << overall.switchE.totalCalls
        << " = " << (overall.switchE.blockingProbability * 100.0) << "%" << std::endl;
    std::cout << "  Switch F: " << overall.switchF.blockedCalls << "/" << overall.switchF.totalCalls
        << " = " << (overall.switchF.blockingProbability * 100.0) << "%" << std::endl;

    std::cout << "  Diferenca maxima bloqueio (D,E,F): " << overall.maxBlockingDiff << "%" << std::endl;
    bool diffOk = (overall.maxBlockingDiff <= 0.5);
    std::cout << "    -> Objetivo <= 0.5%: " << (diffOk ? "[OK]" : "[FALHOU]") << std::endl;

    std::cout << "\n=== Metricas do Call Center (Hora de Ponta) ===" << std::endl;
    std::cout << "  Numero de operadores: " << cfg.numOperators << std::endl;
    std::cout << "  Utilizacao operadores: " << (peakHour.operatorUtil * 100.0) << "%" << std::endl;
    std::cout << "  Operadores ocupados (media): " << peakHour.avgOperatorsBusy << std::endl;
    std::cout << "  Tempo espera medio: " << peakHour.avgWaitingAll << " s (" << peakHour.avgWaitingAll / 60.0 << " min)" << std::endl;
    std::cout << "  Numero medio em fila: " << peakHour.meanQueueSize << std::endl;

    std::cout << "\n=== Tempo de Espera (Hora de Ponta) ===" << std::endl;
    std::cout << "  Atendidas em < 6 min: " << (probWaitLT360 * 100.0) << "%" << std::endl;
    bool esperaOk = (probWaitLT360 >= 0.50);
    std::cout << "    -> Objetivo >= 50%: " << (esperaOk ? "[OK]" : "[FALHOU]") << std::endl;

    std::cout << "\n========================================" << std::endl;
    std::cout << "   RESUMO DOS OBJETIVOS (HORA DE PONTA)" << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << "  Bloqueio <= 1.5%:           " << (bloqueioOk ? "OK" : "FALHOU") << std::endl;
    std::cout << "  50% atendidas < 6min:       " << (esperaOk ? "OK" : "FALHOU") << std::endl;
    std::cout << "  Diff bloqueio D,E,F <= 0.5%: " << (diffOk ? "OK" : "FALHOU") << std::endl;

    std::cout << "\n========================================" << std::endl;
    std::cout << "CSV 'stability_timeseries.csv' gerado." << std::endl;
    std::cout << "========================================" << std::endl;

    return 0;
}