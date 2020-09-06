
#ifndef __JUDGE_H__
#define __JUDGE_H__

#include "proj2.h"

/**
 * @brief function that represents the judge
 * @param im_proc the number of generated immigrants
 * @param jg_enter_time the time window in milliseconds in which the judge has to randomly enter the building
 * @param jg_verdict_time the time window in milliseconds in which the judge has to randomly make decision
 */
void judge_main_process(long jg_enter_time, long jg_verdict_time, long run_time);

#endif // __JUDGE_H__