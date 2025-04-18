#include "timeWheel.h"
#include <memory.h>
#include <butil/logging.h>
#include <chrono>
#include <iostream>
#include <thread>

TimeWheel::TimeWheel()
    : m_steps(0), m_firstLevelCount(0), m_secondLevelCount(60), m_thirdLevelCount(0), m_increaseId(0) {
    memset(&m_timePos, 0, sizeof(m_timePos));
}

void* TimeWheel::loopForInterval(void* arg) {
    if (arg == NULL) {
        LOG(ERROR) << "no args!!!";
        return NULL;
    }
    TimeWheel* timeWheel = reinterpret_cast<TimeWheel*>(arg);
    while (true) {
        std::this_thread::sleep_for(std::chrono::milliseconds(timeWheel->m_steps));
        // printf("wake up\n");
        TimePos pos           = {0};
        TimePos m_lastTimePos = timeWheel->m_timePos;
        // update slot of current TimeWheel
        timeWheel->getTriggerTimeFromInterval(timeWheel->m_steps, pos);
        timeWheel->m_timePos = pos;
        {
            std::unique_lock<std::mutex> lock(timeWheel->m_mutex);
            // if minute changed, process in integral point (minute)
            if (pos.pos_min != m_lastTimePos.pos_min) {
                // printf("minutes changed\n");
                std::list<Event_t>* eventList =
                    &timeWheel->m_eventSlotList[timeWheel->m_timePos.pos_min + timeWheel->m_firstLevelCount
                                                + timeWheel->m_secondLevelCount];
                timeWheel->processEvent(*eventList);
                eventList->clear();
            } else if (pos.pos_sec != m_lastTimePos.pos_sec) {
                std::list<Event_t>* eventList =
                    &timeWheel->m_eventSlotList[timeWheel->m_timePos.pos_sec + timeWheel->m_firstLevelCount];
                timeWheel->processEvent(*eventList);
                eventList->clear();
            } else if (pos.pos_ms != m_lastTimePos.pos_ms) {
                std::list<Event_t>* eventList = &timeWheel->m_eventSlotList[timeWheel->m_timePos.pos_ms];
                timeWheel->processEvent(*eventList);
                eventList->clear();
            }
        }
    }

    return nullptr;
}

// init TimeWheel's step and maxmin, which detemine the max period of this wheel
void TimeWheel::initTimeWheel(int steps, int maxMin) {
    if (1000 % steps != 0) {
        LOG(ERROR) << "steps wrong !!" << steps;
        return;
    }
    m_steps           = steps;
    m_firstLevelCount = 1000 / steps;
    m_thirdLevelCount = maxMin;

    m_eventSlotList.resize(m_firstLevelCount + m_secondLevelCount + m_thirdLevelCount);
    int ret = pthread_create(&m_loopThread, NULL, loopForInterval, this);
    if (ret != 0) {
        LOG(ERROR) << "create thread error:" << strerror(errno) ;
        return;
    }
    // pthread_join(m_loopThread, NULL);
}

void TimeWheel::createTimingEvent(int interval, EventCallback_t callback) {
    if (interval < m_steps || interval % m_steps != 0
        || interval >= m_steps * m_firstLevelCount * m_secondLevelCount * m_thirdLevelCount) {
        LOG(ERROR) << "invalid interval";
        return;
    }
    LOG(INFO) << "start create event";
    Event_t event  = {0};
    event.interval = interval;
    event.cb       = callback;
    // set time start
    event.timePos.pos_min = m_timePos.pos_min;
    event.timePos.pos_sec = m_timePos.pos_sec;
    event.timePos.pos_ms  = m_timePos.pos_ms;
    event.id              = createEventId();
    // insert it to a slot of TimeWheel
    std::unique_lock<std::mutex> lock(m_mutex);
    insertEventToSlot(interval, event);
    LOG(INFO) << "create event over";
    //预期先启动时启动一次
    event.cb();
}

int TimeWheel::createEventId() {
    return m_increaseId++;
}

void TimeWheel::getTriggerTimeFromInterval(int interval, TimePos_t& timePos) {
    // get current time: ms
    int curTime = getCurrentMs(m_timePos);


    // caculate which slot this interval should belong to
    int futureTime = curTime + interval;

    timePos.pos_min = (futureTime / 1000 / 60) % m_thirdLevelCount;
    timePos.pos_sec = (futureTime % (1000 * 60)) / 1000;
    timePos.pos_ms  = (futureTime % 1000) / m_steps;
    return;
}

int TimeWheel::getCurrentMs(TimePos_t timePos) {
    return m_steps * timePos.pos_ms + timePos.pos_sec * 1000 + timePos.pos_min * 60 * 1000;
}

int TimeWheel::processEvent(std::list<Event_t>& eventList) {
    // printf("eventList.size=%d\n", eventList.size());

    // process the event for current slot
    for (auto event = eventList.begin(); event != eventList.end(); event++) {
        // caculate the current ms
        int currentMs = getCurrentMs(m_timePos);
        // caculate last  time(ms) this event was processed
        int lastProcessedMs = getCurrentMs(event->timePos);
        // caculate the distance between now and last time(ms)
        int distanceMs = (currentMs - lastProcessedMs + (m_secondLevelCount + 1) * 60 * 1000)
                         % ((m_secondLevelCount + 1) * 60 * 1000);

        // if interval == distanceMs, need process this event
        if (event->interval == distanceMs) {
            // process event
            event->cb();
            // get now pos as this event's start point
            event->timePos = m_timePos;
            // add this event to slot
            insertEventToSlot(event->interval, *event);
        } else {

            // although this event in this positon, but it not arriving timing, it will continue move to next slot
            // caculate by distance ms.
            insertEventToSlot(distanceMs, *event);
        }
    }
    return 0;
}

void TimeWheel::insertEventToSlot(int interval, Event_t& event) {

    TimePos_t timePos = {0};

    // caculate the which slot this event should be set to
    getTriggerTimeFromInterval(interval, timePos);
    {
        // if minutes not equal to current minute, first insert it to it's minute slot
        if (timePos.pos_min != m_timePos.pos_min) {
            m_eventSlotList[m_firstLevelCount + m_secondLevelCount + timePos.pos_min].push_back(event);
        }
        // if minutes is equal, but second changed, insert slot to this  integral point second
        else if (timePos.pos_sec != m_timePos.pos_sec) {
            m_eventSlotList[m_firstLevelCount + timePos.pos_sec].push_back(event);
        }
        // if minute and second is equal, mean this event will not be trigger in integral point, set it to ms slot
        else if (timePos.pos_ms != m_timePos.pos_ms) {
            m_eventSlotList[timePos.pos_ms].push_back(event);
        }
    }
    return;
}
