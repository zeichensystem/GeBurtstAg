#include <memory.h>
#include <tonc_memmap.h>
#include <tonc_memdef.h>

#include "timer.h"
#include "math.h"
#include "logutils.h"

static PerformanceData performanceData[MAX_PERF_DATA];
static int currentPerformanceId;


void timerInit(void) 
{
    REG_TM0D = 0x10000 - 4;  // 4 ticks till overflow (at 16384 Hz (1024 cycles), that means we have a overflow every 4 * 16384^-1 =  2^-12 s, or 0.2 ms)
    REG_TM0CNT = TM_ENABLE | TM_FREQ_1024;
    REG_TM1CNT = TM_ENABLE | TM_CASCADE;  // will increment in 2^-12 seconds (4098 Hz), can be used as a .12 fixed point number then, overflows every 16 seconds
    
    REG_TM2D = 0x10000 - 0xffff; // 0xffff ticks till overflow, or about 1 seconds (65536 Hz)
    REG_TM2CNT = TM_ENABLE | TM_FREQ_256;
    
    for (int i = 0; i < MAX_PERF_DATA; ++i) {
        performanceData[i].avgTime = 0;
    }
}


Timer timerNew(FIXED_12 duration, TimerType type) 
{
    Timer timer;
    timer.type = type;
    timer.duration = duration;
    timer.time = 0;
    timer.__prevTimerState = 0;
    timer.done = false;
    timer.stopped = true;
    timer.deltatime = 0;
    return timer;
}

void timerStart(Timer *timer) 
{
    timer->__prevTimerState = timer->type == TIMER_REGULAR ? REG_TM1D : REG_TM2D;
    timer->time = 0;
    timer->stopped = false;
    timer->done = false;
}

void timerStop(Timer *timer) 
{
    timer->stopped = true;
}

void timerResume(Timer *timer) 
{
    timer->stopped = false;
    timer->__prevTimerState = timer->type == TIMER_REGULAR ? REG_TM1D : REG_TM2D;
}

void timerRewind(Timer *timer) 
{
    timer->time = 0;
}

void timerTick(Timer *timer) 
{ // Consecutive calls of a regular timer must be within ~15 (2^-12 * 0xffff) seconds of each other (so just call it each frame and don't worry); performance timers every second.
    if (timer->stopped || timer->done) {
        return;
    }

    FIXED_12 timerState = timer->type == TIMER_REGULAR ? REG_TM1D : REG_TM2D;

    if (timer->__prevTimerState > timerState) {
        timer->deltatime = (0xffff + timerState) - timer->__prevTimerState; // handle overflow
    } else {
        timer->deltatime = timerState - timer->__prevTimerState;
    }
    timer->time += timer->deltatime;
    timer->__prevTimerState = timerState;

    if (timer->time >= timer->duration) {
        timer->done = true;
        timer->stopped = true;
    }
}


int performanceDataRegister(const char* name) 
{ // Invariant: has to be called once per second TODO: allow for TIMER_REGULAR
    assertion(currentPerformanceId < MAX_PERF_DATA, "timer.c/performanceStart(): currentPerfId < MAX_PERF_DATA");
    PerformanceData *perfData = performanceData + currentPerformanceId;
    perfData->id = currentPerformanceId;
    perfData->timer = timerNew(TIMER_MAX_DURATION, TIMER_PERF); 
    perfData->avgSamples = 1;
    perfData->avgTime = 0;
    strlcpy(perfData->name, name, PERF_NAME_MAX_SIZE);
    currentPerformanceId++;
    return perfData->id;
}


void performanceStart(int perfId) 
{
    assertion(perfId < currentPerformanceId, "perfStart");
    PerformanceData *perfData = performanceData + perfId;
    timerResume(&perfData->timer);
    // timerStart(&perfData->timer);
}

void performanceEnd(int perfId) 
{
    assertion(perfId < currentPerformanceId, "perfEnd");
    PerformanceData *perfData = performanceData + perfId;
    timerTick(&perfData->timer);
    timerStop(&perfData->timer);
}

void performanceReset(void) 
{
    for (int i = 0; i < currentPerformanceId; ++i) {
        // performanceData[i].avgTime = performanceData[i].avgTime + fx12div(performanceData[i].timer.time - performanceData[i].avgTime, int2fx12(performanceData[i].avgSamples)); 
        performanceData[i].avgTime = performanceData[i].timer.time;
        performanceData[i].avgSamples++;
        timerRewind(&performanceData[i].timer);
     }
}

void performancePrintAll(void) 
{
    for (int i = 0; i < currentPerformanceId; ++i) {
        PerformanceData perf = performanceData[i];
        if (perf.avgTime) {
            int shift = perf.timer.type == TIMER_PERF ? 4 : 0;
            mgba_printf("%s: %f ms", perf.name, fx12ToFloat(perf.avgTime * 1000 >> shift));
        }

    }
}

FIXED_12 performanceGetSeconds(const PerformanceData *perfData) 
{
    int shift = perfData->timer.type == TIMER_PERF ? 4 : 0;
    return perfData->timer.time >> shift;
}