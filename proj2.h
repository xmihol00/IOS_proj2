
//================================================================================================
// File:        proj2.h
// Case:        IOS proj2, 15. 4. 2020
// Author:      David Mihola, FIT, xmihol00@stud.fit.vutbr.cz
// Compiled:    gcc (Ubuntu 9.2.1-9ubuntu2) 9.2.1 20191008
// Description: Declaration of initiating and terminating functions and macros
//================================================================================================

#ifndef __PROJ2_H__
#define __PROJ2_H__

#include <stdio.h>
#include <signal.h>
#include <sys/types.h>
#include <unistd.h>
#include <errno.h>
#include <sys/wait.h>
#include <sys/mman.h>
#include <semaphore.h>
#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>
#include <limits.h>
#include <getopt.h>
#include <stdbool.h>
#include <time.h>

#ifdef DEBUG
    #ifdef NOISE__TEST1
        #define NOISE_TEST1 usleep(rand() % (getpid() / 10 + 1))
    #else
        #define NOISE_TEST1
    #endif

    #ifdef NOISE__TEST2
        #define NOISE_TEST2 if(!(rand() % 10)) usleep(rand() % (getpid() / 5 + 1))
    #else
        #define NOISE_TEST2
    #endif

    #ifdef NOISE__TEST3
        #define NOISE_TEST3 if(getpid() & 1) usleep(rand() % (getpid() | 0x00000AC0F))
    #else
        #define NOISE_TEST3 
    #endif

    #ifdef NOISE__TEST4
        #define NOISE_TEST4  usleep(rand() % (getpid() / 2 +1))
    #else
        #define NOISE_TEST4 
    #endif

    #ifdef NOISE__TEST5
        #define NOISE_TEST5  usleep(1)
    #else
        #define NOISE_TEST5 
    #endif
#endif //DEBUG

/** acceptable number of parameters */
#define PARAM_NUM 6

/** number of generated immigrants */
#define IM_PROC        argv[1 + optind - 1]

/** maximal time, in which immigrant has to be generated */
#define IM_GEN_TM      argv[2 + optind - 1]

/** maximal time, in which judge enters the building again */
#define JG_ENTER_TM    argv[3 + optind - 1]

/** maximal time, in which the certificat has to be picked up by immigrant */
#define CERTIF_PICK_TM argv[4 + optind - 1]

/** maximal time, in which juge has to provide judgment */
#define JG_VERDICT_TM  argv[5 + optind - 1]

/** macro for printing the error message, when argument parsing fails */
#define ARGS_ERR_PRINT(ARG_NUM) fprintf(stderr, "main process: Argument number: %d is incorrect.\nRun with arguments: [-b RUN_TIME: >= 1 && <= 7000] [--] PI: >= 1 && <= SEM_VALUE_MAX, IG: >= 1 && <= 2000, JG: >= 1 && <= 2000, IT >= 1 && <= 2000.\n", (ARG_NUM))

/** failure exit code */
#define ERR_EXIT 1

/** constant for converting string to decimal number */
#define DECIMAL 10

/** evaluates to 1 when the NUM of processes is _NOT_ in the interval (0, INT_MAX + 1), otherwise 0 */
#define PROCES_NUM_CHECK(NUM) (NUM <= 0 || NUM > SEM_VALUE_MAX)

/** evaluates to 1 when the TIME is _NOT_ in the time interval (0, 2000) inclusive, otherwise 0*/
#define ARGS_TIME_CHECK(TIME) (TIME < 0 || TIME > 2000) 

/** evaluates to 1 when the RUN_TIME is _NOT_ in the time interval (1, 7000) inclusive, otherwise 0*/
#define ARGS_RUN_TIME_CHECK(TIME) (TIME < 1 || TIME > 7000) 

/** creates and maps a shared variable across processes of type TYPE and name NAME */
#define MMAP_VAR(NAME) NAME = mmap(NULL, sizeof(*(NAME)), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0)

/** creates and maps a shared array across processes of type TYPE, name NAME and size SIZE */
#define CREATE_MMAP_ARR(TYPE, NAME, SIZE) TYPE *NAME = mmap(NULL, (SIZE)*sizeof(NAME), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0)

/** maps a shared array across processes of type TYPE, name NAME and size SIZE */
#define MMAP_ARR(NAME, SIZE) NAME = mmap(NULL, (SIZE)*sizeof(*(NAME)), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0)

/** unmaps a shared variable across processes with name NAME */
#define UNMAP_VAR(NAME) munmap(NAME, sizeof(*(NAME)))

/** unmaps a shared variable array across processes with name NAME and size SIZE */
#define UNMAP_ARR(NAME, SIZE) munmap(NAME, (SIZE)*sizeof(*(NAME)))

/** mocros that are used for accessing the shared array accross processes */
#define SHARED_MEM_ARR_SIZE 8
#define PN 0
#define NE 1
#define NC 2
#define NB 3
#define NI 4
#define IP 5
#define IL 6
#define GN 7

/**
 * @brief holds the numeric data of the program arguments 
 */
typedef struct arguments
{
    long im_proc;               /** number of generated immigrants */
    long im_gen_time;           /** maximal time, in which immigrant has to be generated */
    long jg_enter_time;         /** maximal time, in which judge enters the building again */
    long certif_pick_time;      /** maximal time, in which the certificat has to be picked up by immigrant */
    long jg_verdict_time;       /** maximal time, in which juge has to provide judgment */
    long run_time;              /** the run-time of the program */

}arguments_t;

/*shared memory between processes (see: macros above)
  including: 
        PN: the current number of printed line
        NE: the number of immigrants, who entered the building
        NC: the number of immigrants, who checked in
        NB: the number of immigrants, who are in the building
        NI: the number of immigrants, which has been already generated
        IP: the total number of started immigrant processes
        IL: the total number of immigrants, who left the building
        GN: indicator, if new imigrants will be generated
*/
extern long *shared_mem;

///the file ponter to a shered file
extern FILE *fptr_p2_out;

///the semaphore, which controls the read/write to any of the shared variables
extern sem_t *var_change_semaphore;

///the semaphore, which controls the entry of the building (when judge is in no immigrants can get in/out)
extern sem_t *jg_entry_semaphore;

///the semaphore, which holds immigrants before their sentenc
extern sem_t *decision_semaphore;

///the semaphore, which holds the judge until there is the same number of entered immigrants as the number of chekced immigrants
extern sem_t *jg_wait_semaphore;

///the semaphore, which seperates the immigrants, who were sentenced and yet have to pick their certificat from those,
///who weren't sentenced yet, but have already checked
extern sem_t *pick_up_semaphore;

///the semaphore, which groups parts of code, which has to be performed together, together
extern sem_t *together_semaphore;

///the semaphore, on which the judge waits until all the processes in current cycle finishes, than he either continues or also finishes
extern sem_t *jg_leave_semaphore;

///the semaphore, which is opened in a signal handler of the immigrant generator process.
///the signal is sent after after all immigrants leave the building and the correct termination can countinue
extern sem_t *gen_notif_semaphore;


/**
 * @brief arguments parsing. ARGUMNETS THAT ARE USED FOR RANDOM WAITING ARE INCREASED BY _1_ 
 *        (so that modulo gives the appropriate result).
 * @param argv the program arguments
 * @param argc the neumber of arguments
 * @param arguments allocated structure, where the parsed arguments will be stored
 * @return 0 on success, otherwise ERR_EXIT (1) 
 **/
int argument_check(char **argv, int argc, arguments_t *arguments);

/**
 * @brief initiates the shared memory (including semaphores and file pointer)
 * @param im_proc the value to which the entery semaphore for immigrants is set
 * @return 0 when successful, otherwise 1
 */
int shared_mem_init(int im_proc);

/**
 * @brief function that creates the main immigrant process, that generates all of the immigrants and the judge process
 * @param immigrant_main_pid is the pid_t, where the immigrant main process pid is stored
 * @param judge_pid is the pid_t, where the judge process pid is stored
 * @param arguments parsed program arguments
 */
void main_process_fork(pid_t *immigrant_main_pid, pid_t *judge_pid, arguments_t *arguments);

/**
 * @brief function that frees all used resources
 */
void shared_mem_free();

/**
 * @brief unlinks all the semaphores before they are mmaped again, this assures that the program can be run after it was killed
 * with unexpected siganl.
 */
void all_sems_unlink();

/**
 * @brief handles the SIGQUIT and SIGUSR1 signals, deallocates allocated resources and exits the program
 * @param signal has to functionality, the signal handler API just requires it
 */
void main_sig_quit_handler_free(int signal);

/**
 * @brief handles the SIGQUIT and SIGUSR1 signals and terminates the process with exit code 0
 * @param signal has to functionality, the signal handler API just requires it
 */
void sig_quit_handler_end(int signal);

/**
 * @brief function that is started in a newly genereted process, waits a run_time milliseconds and then sends the SIGUSR1 signal
 * before ending
 * @param run_time lenght of the function run_time in milliseconds 
 */ 
void rt_timer(long run_time);

#endif //__PROJ2_H__