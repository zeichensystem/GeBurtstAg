#ifndef KEYSEQ_H
#define KEYSEQ_H

#include "timer.h"

#define KEY_SEQ_MAX_LEN 8
typedef struct KeySeqWatcher {
    u32 matchSeq[KEY_SEQ_MAX_LEN];
    int matchSeqLen;
    int currentSeqIdx;
    Timer __timer;
    bool keyAlreadyMatchedInCurrentInterval;
} KeySeqWatcher;


KeySeqWatcher keySeqWatcherNew(FIXED_12 keyInterval, const u32 matchSeq[KEY_SEQ_MAX_LEN], const int matchSeqLen);
bool keySeqWatcherUpdate(KeySeqWatcher *kseq); // Returns true if the set key sequence was matched. 

#endif