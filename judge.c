
//================================================================================================
// File:        judge.c
// Case:        IOS proj2, 15. 4. 2020
// Author:      David Mihola, FIT, xmihol00@stud.fit.vutbr.cz
// Compiled:    gcc (Ubuntu 9.2.1-9ubuntu2) 9.2.1 20191008
// Description: Definition of functions, that represent the judge
//================================================================================================

#include "judge.h"

void judge_main_process(long jg_enter_time, long jg_verdict_time, long run_time)
{
    long checked_imm = 0;
    long current_verdict = 0;
    int sem_val = 0;

    //to differenciate the pseudo-random sequence form its parent process
    srand(time(NULL));    
    sem_wait(var_change_semaphore);

    do{
        sem_post(var_change_semaphore);
        sem_trywait(jg_leave_semaphore);

        //judge needs pseudo-random time to enter the building
        if (jg_enter_time > 1)
            usleep((rand() % jg_enter_time) * 1000);

        //judge closes the entry to the building
        //no immigrants can enter the building, when judge is closing the entry, on the other hand, judge can't start closing
        //the entry, when immigrant is enetring, because there is only one door.
        //Only one person can go through the door at one time (either judge or immigrant)
        sem_wait(together_semaphore);
            sem_trywait(jg_entry_semaphore);
        sem_post(together_semaphore);

        sem_wait(var_change_semaphore);
        fprintf(fptr_p2_out, "%ld\t: %-22s: %-22s\n", ++shared_mem[PN], "JUDGE", "wants to enter");
        fflush(fptr_p2_out);
        sem_post(var_change_semaphore);

        //judge finds out if he needs to wait for immigrants    
        sem_wait(var_change_semaphore);
        if (shared_mem[NI] && shared_mem[NE] != shared_mem[NC])
        {
            fprintf(fptr_p2_out, "%ld\t: %-22s: %-22s: %ld\t: %ld\t: %ld\n", ++shared_mem[PN], "JUDGE", "waits for imm", shared_mem[NE], shared_mem[NC], shared_mem[NB]);
            fflush(fptr_p2_out);
            sem_post(var_change_semaphore);
            //judge waits for the rest of immigrants, who didn't check yet
            sem_wait(jg_wait_semaphore);
        }
        else
            sem_post(var_change_semaphore);

        //judge enters the building
        sem_wait(var_change_semaphore);
        fprintf(fptr_p2_out, "%ld\t: %-22s: %-22s: %ld\t: %ld\t: %ld\n", ++shared_mem[PN], "JUDGE", "enters", shared_mem[NE], shared_mem[NC], shared_mem[NB]);
        fflush(fptr_p2_out);
        sem_post(var_change_semaphore);
        
        //judge starts the confirmation
        sem_wait(var_change_semaphore);
        fprintf(fptr_p2_out, "%ld\t: %-22s: %-22s: %ld\t: %ld\t: %ld\n", ++shared_mem[PN], "JUDGE", "starts confirmation", shared_mem[NE], shared_mem[NC], shared_mem[NB]);
        fflush(fptr_p2_out);
        sem_post(var_change_semaphore);

        //judge needs pseudo-random time to confirm all cases
        if (jg_verdict_time > 1)
            usleep((rand() % jg_verdict_time) * 1000);

        sem_wait(var_change_semaphore);
        checked_imm += shared_mem[NE];
        current_verdict = shared_mem[NE];
        shared_mem[NE] = shared_mem[NC] = 0;

        fprintf(fptr_p2_out, "%ld\t: %-22s: %-22s: %ld\t: %ld\t: %ld\n", ++shared_mem[PN], "JUDGE", "ends confirmation", shared_mem[NE], shared_mem[NC], shared_mem[NB]);
        fflush(fptr_p2_out);

        //judge makes sure that currently judged immigrants won't mix with immigrants, who haven't been judged yet
        if (current_verdict)
        {
            do
            {
                sem_trywait(pick_up_semaphore);
                sem_getvalue(pick_up_semaphore, &sem_val);
            }
            while(sem_val > 0);
        }

        //allows to the currrently judged number of immigrants to pick up their certificates
        for (long i = 0; i < current_verdict; i++)
            sem_post(decision_semaphore);
        sem_post(var_change_semaphore);
  
        //judge needs pseudo-random time to leave the building
        if (jg_verdict_time > 1)
            usleep((rand() % jg_verdict_time) * 1000);

        //judge leaves the building
        sem_wait(var_change_semaphore);
        fprintf(fptr_p2_out, "%ld\t: %-22s: %-22s: %ld\t: %ld\t: %ld\n", ++shared_mem[PN], "JUDGE", "leaves", shared_mem[NE], shared_mem[NC], shared_mem[NB]);
        fflush(fptr_p2_out);
        sem_post(var_change_semaphore);

        //judge opens the entry for immigrants, so they can either leave or enter. Only one person can go through the door one time
        sem_post(jg_entry_semaphore);

        sem_wait(var_change_semaphore);
        //if the number of checked immigrants is the same as the number of processes, the judge has to wait, wheather new cycle
        //of processes will start or not. When the switch -b is not set, then the condition always evaluates to false.
        if (checked_imm >= shared_mem[IP] && shared_mem[GN])
        {   
            sem_post(var_change_semaphore);
            sem_wait(jg_leave_semaphore);
        }
        else
            sem_post(var_change_semaphore);
        
        sem_wait(var_change_semaphore);

    }while((shared_mem[GN] && run_time) || (checked_imm < shared_mem[IP] && !run_time));

    sem_post(var_change_semaphore);

    fprintf(fptr_p2_out, "%ld\t: %-22s: %-22s\n", ++shared_mem[PN], "JUDGE", "finishes");
    fflush(fptr_p2_out);
    sem_post(var_change_semaphore);

    exit(0);
}
