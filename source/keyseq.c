#include <tonc.h>
#include <string.h>

#include "keyseq.h"
#include "timer.h"


KeySeqWatcher keySeqWatcherNew(FIXED_12 keyInterval, const u32 matchSeq[KEY_SEQ_MAX_LEN], const int matchSeqLen) 
{
    Timer t = timerNew(keyInterval, TIMER_REGULAR);
    KeySeqWatcher kseq = {.__timer=t, .matchSeqLen=matchSeqLen, .currentSeqIdx=0, .keyAlreadyMatchedInCurrentInterval=false};
    memcpy(kseq.matchSeq, matchSeq, sizeof kseq.matchSeq[0] * matchSeqLen);
    return kseq;

}

// Returns true if the set key sequence was matched. 
bool keySeqWatcherUpdate(KeySeqWatcher *kseq) 
{
    if (kseq->__timer.stopped && !kseq->__timer.done) { // If the timer hasn't been started before.
        timerStart(&kseq->__timer);
    }
    timerTick(&kseq->__timer);

    if (kseq->__timer.done) {
        kseq->currentSeqIdx = 0;
        kseq->keyAlreadyMatchedInCurrentInterval = false;
        timerStart(&kseq->__timer);
    }    

    if (key_hit(kseq->matchSeq[kseq->currentSeqIdx])) { // Currently pressed key matches the sequence.
        timerStart(&kseq->__timer);
        kseq->currentSeqIdx += 1;
        kseq->keyAlreadyMatchedInCurrentInterval = true;
        if (kseq->currentSeqIdx == kseq->matchSeqLen) { // The sequence is matched.
            kseq->currentSeqIdx = 0;
            kseq->keyAlreadyMatchedInCurrentInterval = false;
            return true;
        }
    } else if (key_hit(KEY_ANY) && kseq->keyAlreadyMatchedInCurrentInterval)  { // The currently pressed key is inconsistent with the sequence to be matched, so we reset the currently matched sequence length to zero.  
        kseq->currentSeqIdx = 0;
        kseq->keyAlreadyMatchedInCurrentInterval = false;
        timerStart(&kseq->__timer);
    }
    return false;
}