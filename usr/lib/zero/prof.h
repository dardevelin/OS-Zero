#ifndef __ZERO_PROF_H__
#define __ZERO_PROF_H__

#include <stdint.h>
#include <sys/time.h>

#if defined(__x86_64__) || defined(__amd64__)
#include <zero/x86-64/prof.h>
#elif defined(__i386__)
#include <zero/ia32/prof.h>
#elif defined(__arm__)
#include <zero/arm/prof.h>
#endif

#define tvcmp(tv1, tv2)                                                 \
    (((tv2)->tv_sec - (tv1)->tv_sec) * 1000000                          \
     + ((tv2)->tv_usec - (tv1)->tv_usec))
#define tvgt(tv1, tv2)                                                  \
    (((tv1)->tv_sec > (tv2)->tv_sec)                                    \
     || ((tv1)->tv_sec == (tv2)->tv_sec && (tv1)->tv_usec > (tv2)->tv_usec))

#define tvaddconst(tv, u)                                               \
  do {                                                                  \
      (tv)->tv_sec += (u) / 1000000;                                    \
      (tv)->tv_usec += (u) % 1000000;                                   \
                                                                        \
      if ((tv)->tv_usec >= 1000000) {                                   \
          (tv)->tv_sec++;                                               \
          (tv)->tv_usec -= 1000000;                                     \
      } else if ((tv)->tv_usec < 0) {                                   \
          (tv)->tv_sec--;                                               \
          (tv)->tv_usec += 1000000;                                     \
      }                                                                 \
  } while (FALSE)

#define PROFCLK(id)                                                     \
    struct timeval _tv##id[2]
#define profinitclk(id)                                                 \
    memset(&_tv##id, 0, sizeof(_tv##id))
#define profstartclk(id)                                                \
    gettimeofday(&_tv##id[0], NULL)
#define profstopclk(id)                                                 \
    gettimeofday(&_tv##id[1], NULL)
#define profclkdiff(id)                                                 \
    tvcmp(&_tv##id[0], &_tv##id[1])

#endif /* __ZERO_PROF_H__ */

