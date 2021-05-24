#ifndef TIMER_H
#define TIMER_H

#include <tonc_memmap.h>
#include <tonc_memdef.h>

#include "math.h"
#include "logutils.h"


#define TIMER_MAX_DURATION 0x7FFFFFFF

typedef enum TimerType {
    TIMER_PERF, 
    TIMER_REGULAR
} TimerType;


typedef struct Timer {
    TimerType type;
    FIXED_12 time, duration, deltatime;
    bool stopped, done;

    s32 __prevTimerState;
} Timer;

Timer timerNew(FIXED_12 duration, TimerType type);
void timerStart(Timer *timer);
void timerTick(Timer *timer);

void timerStop(Timer *timer);
void timerResume(Timer *timer);
void timerInit(void);


#define PERF_NAME_MAX_SIZE 64
#define MAX_PERF_DATA 16
#define PERF_MOVING_AVG_LEN 16

typedef struct PerformanceData {
    char name[PERF_NAME_MAX_SIZE];
    int id;
    Timer timer;
    FIXED_12 avgTime;
    int avgSamples;
} PerformanceData;

int performanceDataRegister(const char* name);// invariant: has to be called once per second TODO: allow for TIMER_REGULAR
void performanceStart(int perfId);

void performanceEnd(int perfId);
void performanceGather(void);
void performancePrintAll(void);

#endif