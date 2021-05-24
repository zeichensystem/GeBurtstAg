#ifndef COMMONDEFS_H
#define COMMONDEFS_H

#define IWRAM_CODE_ARM  __attribute__((target("arm"), section(".iwram")))

#define M5_SCALED_W 160
#define M5_SCALED_H 100

#endif