
//================================================================================================
// File:        immigrant.c
// Case:        IOS proj2, 15. 4. 2020
// Author:      David Mihola, FIT, xmihol00@stud.fit.vutbr.cz
// Compiled:    gcc (Ubuntu 9.2.1-9ubuntu2) 9.2.1 20191008
// Description: Definitions of functions, that represents immigrants
//================================================================================================

#include "immigrant.h"

void immigrant_main_process(arguments_t *arguments)
{
    //creates its own signal handling
	sigset_t block_mask;
    sigset_t empty_mask;
	struct sigaction sigusr1_sigquit;

    sigemptyset(&empty_mask);
    sigusr1_sigquit.sa_handler = sig_quit_usr1_handler;
    sigusr1_sigquit.sa_mask = empty_mask;
    sigusr1_sigquit.sa_flags = 0;
    if (sigaction(SIGUSR1, &sigusr1_sigquit, NULL) || sigaction(SIGQUIT, &sigusr1_sigquit, NULL)) 
    {
		perror ("immigrant main process");
		exit(ERR_EXIT);
	}
    sigemptyset(&block_mask);
    sigaddset(&block_mask, SIGUSR1);
    sigaddset(&block_mask, SIGQUIT);

    //unblocking the previously blocked signals in the main process
    if (sigprocmask(SIG_UNBLOCK, &block_mask, NULL))
    {
        perror("immigrant main process sigprocmask");
        exit(ERR_EXIT);
    }

    //installing signalr hadler for SIGUSR2, by which the generator is notyfied about the emty building
    signal(SIGUSR2, sigusr2_handler);

    //to differecniate the pseudo-random sequence of numbers from the main proces sequence
    //overflow of unsigned is defined
    srand(time(NULL) + getpid());

    //copying the number of generated processes in one cycle

    pid_t immigrant_pid;

    do //until the timer is up or SIGQUIT is send by the user or no run-time was specified (only one cycle is performed)
    {
        for (long i = 0; i < arguments->im_proc; i++)
        {   
            //genrator wait for random time
            if (arguments->im_gen_time > 1)
                usleep((rand() % arguments->im_gen_time) * 1000);

            //blocking the terminating signals while generating new processes (immigrants), there is no need for them to recieve
            //any of the blocked signals, as all generated immigrants has to always ends successfuly
            if (sigprocmask(SIG_BLOCK, &block_mask, &empty_mask))
            {
                perror("immigrant main process sigprocmask");
                exit(ERR_EXIT);
            }
            //fork has to be executed and the newly created immigrant has to register before the generator can continue,
            //it is necessary to maintain the accurate count of created immigrants, who has to be judged
            sem_wait(together_semaphore);
            if ((immigrant_pid = fork()) == 0)
            {
                sem_wait(var_change_semaphore);
                shared_mem[NI]++;
                sem_post(together_semaphore);
                immigrant_process(arguments->certif_pick_time, arguments->im_proc);
            }
            else if (immigrant_pid == -1)
            {
                //if fork of one immigrant fails, all other immigrants are killed and the immigrant main process ends unsuccessfuly
                //main process does the memory management
                perror("immigrant main process fork");
                exit(ERR_EXIT);
            }
            //the generator can only start generating again, after the newly created immigrant registers
            sem_wait(together_semaphore);
            sem_post(together_semaphore);
            //unblocking the signals for termination
            if (sigprocmask(SIG_UNBLOCK, &block_mask, NULL))
            {
                perror("immigrant main process sigprocmask");
                exit(ERR_EXIT);
            }

        }

        if (arguments->run_time)
            sem_wait(gen_notif_semaphore);

        for (long j = 0; j < arguments->im_proc; j++)
        {
            wait(NULL); //clearing/(waiting, when -b wasn't specified) for immigrants
        }

        //blocking the signals form comming in this critical section, when the counter is changed
        if (sigprocmask(SIG_BLOCK, &block_mask, &empty_mask))
        {
            perror("immigrant main process sigprocmask");
            exit(ERR_EXIT);
        }
        if (arguments->run_time) //if next round of generation should be started
        {
            sem_wait(var_change_semaphore);
            shared_mem[IP] += arguments->im_proc; //increasing the total number of generated processes by one cycle
            sem_post(var_change_semaphore);

            sem_post(jg_leave_semaphore); //lettig the judge to continue
        }
        if (sigprocmask(SIG_UNBLOCK, &block_mask, NULL))
        {
            perror("immigrant main process sigprocmask");
            exit(ERR_EXIT);
        }
    }
    while(arguments->run_time); //if no run-time was entered, then this evaluates to false and the process ends
    
    exit(0);
}

void immigrant_process(long certif_pick_time, long im_proc)
{   
    //to differenciate the pseudo-random sequence from its parent proces
    //parametr of srand is an unsigned so overfow won't be an issue
    srand(time(NULL) * getpid());

    //variable that holds the state of a semaphore, that remembers if there are immigrants, who entered, but didn't check.
    int sem_val = 0;

    //proces starts
    //immigrant remembers his ID
    long proces_id = shared_mem[NI];
    fprintf(fptr_p2_out, "%ld\t: IMM %-18ld: starts\n", ++shared_mem[PN], proces_id);
    fflush(fptr_p2_out);
    sem_post(var_change_semaphore);
    

    sem_wait(together_semaphore);
        //entry to the building, immigrants cannot get in, when the judge is in. The immigrants are entering one by one.
        sem_wait(jg_entry_semaphore);
        sem_post(jg_entry_semaphore);
        //It cannot happen, that immigrant would stay here on this line, and judge would start judging, so this block of code
        //has to be perfomed at once
        sem_wait(var_change_semaphore);
        //immigrant enters the building
        fprintf(fptr_p2_out, "%ld\t: IMM %-18ld: %-22s: %ld\t: %ld\t: %ld\n", ++shared_mem[PN], proces_id, "enters", ++shared_mem[NE], shared_mem[NC], ++shared_mem[NB]);
        fflush(fptr_p2_out);
        sem_trywait(jg_wait_semaphore);
        sem_post(var_change_semaphore);
    sem_post(together_semaphore);

    //immigrant checks
    sem_wait(var_change_semaphore);
        fprintf(fptr_p2_out, "%ld\t: IMM %-18ld: %-22s: %ld\t: %ld\t: %ld\n", ++shared_mem[PN], proces_id, "checks", shared_mem[NE], ++shared_mem[NC], shared_mem[NB]);
        fflush(fptr_p2_out);
    
        //loading the semaphore value
        sem_getvalue(jg_wait_semaphore, &sem_val);
        //if the semaphore is closed and there is she same number of checked immigrants as the number of immigrants entered,
        //then open the semaphore for judge, otherwise close it
        if (sem_val <= 0 && shared_mem[NE] == shared_mem[NC])
            sem_post(jg_wait_semaphore);
        else if (shared_mem[NE] != shared_mem[NC])
            sem_trywait(jg_wait_semaphore);
      
        //immigrants wait here to be either judged by judge or for the immigrants, that have already been judged to pick their certificate
        //this separates the immigrants that have already been judged from those, who weren't. So it cannot happen, 
        //that immigrant could pick a certificate without beeing judged
        sem_wait(pick_up_semaphore);
    sem_post(var_change_semaphore);

    //immigrants waits for the decision by judge
    sem_wait(decision_semaphore);

    //if all currently judged immigrants crossed the decision semaphore, new batch of immigrants can move forward
    sem_getvalue(decision_semaphore, &sem_val);

    if(sem_val == 0)
    {
        //opening the door evry time for all immigrants, which is not very preices, but I still think more efficient, 
        //that creating a shared counter across processes of how many immigrants have passed and 
        //accessing the memory for both read and with additional smephore for write every time. 
        for (long i = 0; i < im_proc; i++)
            sem_post(pick_up_semaphore);
    }

    //immigrant asks for certificate
    sem_wait(var_change_semaphore);
    fprintf(fptr_p2_out, "%ld\t: IMM %-18ld: %-22s: %ld\t: %ld\t: %ld\n", ++shared_mem[PN], proces_id, "wants certificate", shared_mem[NE], shared_mem[NC], shared_mem[NB]);
    fflush(fptr_p2_out);
    sem_post(var_change_semaphore);

    //waits for random time in between 0 and certificateion pick up time
    if (certif_pick_time > 1)
        usleep((rand() % certif_pick_time) * 1000);

    //immigrant recieves certificate
    sem_wait(var_change_semaphore);
    fprintf(fptr_p2_out, "%ld\t: IMM %-18ld: %-22s: %ld\t: %ld\t: %ld\n", ++shared_mem[PN], proces_id, "got certificate", shared_mem[NE], shared_mem[NC], shared_mem[NB]);
    fflush(fptr_p2_out);
    sem_post(var_change_semaphore);

    //entry to the building, immigrants cannot get out, when the judge is in
    sem_wait(together_semaphore);
        //the entry
        sem_wait(jg_entry_semaphore);
        sem_post(jg_entry_semaphore);

        //immigrant leaves the building
        sem_wait(var_change_semaphore);
        fprintf(fptr_p2_out, "%ld\t: IMM %-18ld: %-22s: %ld\t: %ld\t: %ld\n", ++shared_mem[PN], proces_id, "leaves", shared_mem[NE], shared_mem[NC], --shared_mem[NB]);
        fflush(fptr_p2_out);
        sem_post(var_change_semaphore);
    sem_post(together_semaphore);

    sem_wait(var_change_semaphore);
    //sending signal to the generator, when the last immigrant leaves
    //just to fulfill the assignment, otherwise semaphore would be opened (now the generator's signal handler does the task)
    if (++shared_mem[IL] == shared_mem[IP])
    {
        kill(getppid(), SIGUSR2);
    }
    sem_post(var_change_semaphore);
    
    exit(0);
}

void sig_quit_usr1_handler(int sig)
{
    //the part of the code below can be executed only once, so after the firts terminating signal is delivered, 
    //others are ignored
    signal(SIGQUIT, SIG_IGN);
    signal(SIGUSR1, SIG_IGN);
    (void)sig; //dummy for the compiler

    //engages the program safe termination
    sem_wait(var_change_semaphore);

    long wait_for = shared_mem[NI] - shared_mem[IL];

    if (shared_mem[IL] != shared_mem[NI])
    {
        int sema_vala;
        sem_getvalue(gen_notif_semaphore, &sema_vala);
        shared_mem[IP] = shared_mem[NI];
        sem_post(var_change_semaphore);

        sem_wait(gen_notif_semaphore);
    }
    else
    {
        sem_post(var_change_semaphore);
    }
    
    sem_wait(var_change_semaphore);
    shared_mem[GN] = 0;
    sem_post(var_change_semaphore);
    sem_post(jg_leave_semaphore);

    for (long i = 0; i < wait_for; i++)
    {
        wait(NULL); //waiting for all of the immigrants to finish 
    }

    exit(0);
}

void sigusr2_handler(int sig)
{
    (void)sig; //dummy for the compiler

    int sema_val = 0;
    sem_getvalue(gen_notif_semaphore, &sema_val);
    if (!sema_val)
    {
        sem_post(gen_notif_semaphore);
    }
}
