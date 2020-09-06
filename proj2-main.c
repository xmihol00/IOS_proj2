
//================================================================================================
// File:        proj2-main.c
// Case:        IOS proj2, 15. 4. 2020
// Author:      David Mihola, FIT, xmihol00@stud.fit.vutbr.cz
// Compiled:    gcc (Ubuntu 9.2.1-9ubuntu2) 9.2.1 20191008
// Description: Definition of the main function
//================================================================================================


#include "proj2.h"
#include "immigrant.h"
#include "judge.h"

//declaration of globaly shared variables
long *shared_mem = NULL;
FILE *fptr_p2_out = NULL;
sem_t *var_change_semaphore = NULL;
sem_t *jg_entry_semaphore = NULL;
sem_t *decision_semaphore = NULL;
sem_t *jg_wait_semaphore = NULL;
sem_t *pick_up_semaphore = NULL;
sem_t *together_semaphore = NULL;
sem_t *jg_leave_semaphore = NULL;
sem_t *gen_notif_semaphore = NULL;

int main (int argc, char *argv[])
{
    //signal handling
    sigset_t block_mask;
    sigset_t empty_mask;
	struct sigaction sig_quit;

    sigemptyset(&empty_mask);
    sig_quit.sa_handler = sig_quit_handler_end;
    sig_quit.sa_mask = empty_mask;
    sig_quit.sa_flags = 0;
    //the program can now end 
    if (sigaction(SIGQUIT, &sig_quit, NULL)) 
    {
		perror ("main process sigaction");
		exit(1);
	}
    
    //creating mask, that blocks choosen signals
    sigemptyset(&block_mask);
    sigaddset(&block_mask, SIGQUIT);
    sigaddset(&block_mask, SIGUSR1);

    //to ensure that the pseudo-random numbers are different at each run.
    srand(time(NULL) - getpid());

    //all semaphores are unlinked befor new memory initialization is performed, this assures, that the program can be run again,
    //when it was terminated with an unexpected signal
    all_sems_unlink();

    arguments_t arguments = {0, };

    //parsing of arguments
    if(argument_check(argv, argc, &arguments))
        return ERR_EXIT;


    //during memory allocation the terminating signal is blocked
    if (sigprocmask(SIG_BLOCK, &block_mask, &empty_mask))
    {
        perror("main process sigprocmask");
        exit(ERR_EXIT);
    }
    //memory allocation
    if(shared_mem_init(arguments.im_proc))
        return ERR_EXIT;

    //installing new signal handler
    sig_quit.sa_handler = main_sig_quit_handler_free;

    //after memory allocation signal handler, that deallocates the memory is installed and activated    
    if (sigaction(SIGQUIT, &sig_quit, NULL))
    {
		perror ("main process sigaction");
		exit(1);
	}
    //and signals are unblocked
    if (sigprocmask(SIG_UNBLOCK, &block_mask, NULL))
    {
        perror("main process sigprocmask");
        exit(ERR_EXIT);
    }

    //storing the number of arguments to be generated in the first round of generation
    shared_mem[IP] = arguments.im_proc;
    shared_mem[GN] = arguments.run_time;

    pid_t immigrant_main_pid = 0;
    pid_t judge_pid = 0;

    //signals are blocked when new processes are created, handling of these signals is then set appropriately in the generated
    //processes, the main process remains to block them
    if (sigprocmask(SIG_BLOCK, &block_mask, &empty_mask))
    {
        perror("main process sigprocmask");
        shared_mem_free();
        exit(ERR_EXIT);
    }

    //cretes the immigrant generator, judge and timer
    main_process_fork(&immigrant_main_pid, &judge_pid, &arguments);
    
    //If the sigguit signal came before the child processes were created, so it was not delivered to them, or the timer
    //ended before the immigrant generator processes was created, than the signals are caught only by the main process
    //The main process redelivers them, on the other hand, multiple signals signaling the same event could be send, but that
    //is solved in the child's processes handlers
    sigpending(&empty_mask);
    if (sigismember(&empty_mask, SIGQUIT) || sigismember(&empty_mask, SIGUSR1))
        kill(0, SIGQUIT);

    //handeling of fork errors
    int wstatus = 0;
    waitpid(immigrant_main_pid, &wstatus, 0); //waiting for immigrant main process to finish
    if(WEXITSTATUS(wstatus) == ERR_EXIT) //immigrant process fails to fork immigrants
    {
        //judge has to be killed (the initial value for judge_pid is set to 0, 
        //when judge wasn't created yet, no processes will be killed)
        kill(0, SIGKILL); //killing all imigrants and the judge
        
        //all child processes are killed at this point, program can end (unsuccessfuly), after deallocating resources
        shared_mem_free();
        return ERR_EXIT;
    }
    else if (WIFSIGNALED(wstatus)) //immigrant main process and all immigrants were killed by a singal
    {                              //no judge was created, the terminating signal was send by unsuccessful fork of judge process
        shared_mem_free();
        return ERR_EXIT;
    }

    waitpid(judge_pid, NULL, 0); //waiting for judge to finish when the program runs successfuly
    if (arguments.run_time) //checking if the timer was created
        wait(NULL); //waiting for the timer process to finish
	
    //deallocating all the resources
    shared_mem_free();

    return 0;
}
