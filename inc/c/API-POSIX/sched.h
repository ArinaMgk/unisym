#ifndef _SCHED_H
#define _SCHED_H

struct sched_param {
	int sched_priority;
};

#ifdef __cplusplus
extern "C" {
#endif

int sched_yield(void);

#ifdef __cplusplus
}
#endif

#endif
