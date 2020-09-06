
//================================================================================================
// File:        immigrant.h
// Case:        IOS proj2, 15. 4. 2020
// Author:      David Mihola, FIT, xmihol00@stud.fit.vutbr.cz
// Compiled:    gcc (Ubuntu 9.2.1-9ubuntu2) 9.2.1 20191008
// Description: Declaration of functions, that represents immigrants
//================================================================================================

#ifndef __IMMIGRANT_H__
#define __IMMIGRANT_H__

#include "proj2.h"

/**
 * @brief function that generates the immigrants and waits for them to finish, exits with 0 when successful, otherwise with 1,
 *        when something fails (all the other processes are killed at this point)
 * @param im_proc the number of immigrants, who have to be generated
 * @param im_gen_time the time window in milliseconds, in which the new immigrant has to be randomly generated
 * @param certif_pick_time the time window in milliseconds, in which the immigrant has to randomly pick his certificat 
 */
void immigrant_main_process(arguments_t *arguments);

/**
 * @brief function that represents the immigrant, which has to enter the court, be judged and pick up his certificat
 * @param certif_pick_time the time window in milliseconds, in which the immigrant has to randomly pick his certificat 
 * @param im_proc the number of immigrants, who have to be generated 
 */
void immigrant_process(long certif_pick_time, long im_proc);

/**
 * @brief handles the SIGUSR1 (send by timer) and SIGQUIT (send by user) signals. Performes a safe termination of the processes
 *        and the program ends successfuly
 * @param signal has to functionality, the signal handler API just requires it
 */ 
void sig_quit_usr1_handler(int sig);

/**
 * @brief handles the SIGUSR2 (send by the last immigrant) signal. And allows to continue in safe termination.
 * @param signal has to functionality, the signal handler API just requires it.
 */
void sigusr2_handler(int sig);

#endif //__IMMIGRANT_H__