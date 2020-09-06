
//================================================================================================
// File:        proj2-func.c
// Case:        IOS proj2-bonus, 15. 4. 2020
// Author:      David Mihola, FIT, xmihol00@stud.fit.vutbr.cz
// Compiled:    gcc (Ubuntu 9.2.1-9ubuntu2) 9.2.1 20191008
// Description: Definition of initiating functions and terminating functions of the proj2-bonus program
//================================================================================================

#include "proj2.h"
#include "immigrant.h"
#include "judge.h"

int argument_check(char **argv, int argc, arguments_t *arguments)
{
    int option;
    opterr = 0;
    bool b = true;
    char *strtol_check = NULL;

    arguments->run_time = 0;

    while((option = getopt(argc, argv, ":b:")) != -1)
    {
        if (option == 'b')
        {   
            if (b)
            {
                arguments->run_time = strtol(optarg, &strtol_check, DECIMAL);

                if(strtol_check != NULL && *strtol_check != '\0') // when the prametr isn't a number
                {
                    fprintf(stderr, "main process: Invalid option -b\n"
                                    "Run with arguments: [-b RUN_TIME: >= 1 && <= 7000] [--] PI: >= 1 <= SEM_VALUE_MAX, "
                                    "IG: >= 1 && <= 2000, JG: >= 1 && <= 2000, IT >= 1 && <= 2000.\n");
                    return ERR_EXIT;
                }
                else if (ARGS_RUN_TIME_CHECK(arguments->run_time)) // when the parametr inst't in the limit
                {
                    fprintf(stderr, "main process: Invalid option -b\n"
                                    "Run with arguments: [-b RUN_TIME: >= 1 && <= 7000] [--] PI: >= 1 <= SEM_VALUE_MAX, "
                                    "IG: >= 1 && <= 2000, JG: >= 1 && <= 2000, IT >= 1 && <= 2000.\n");
                    return ERR_EXIT;    
                }

            }
            else //when -b is specidfied multiple times
            {
                fprintf(stderr, "main process: Option -b can be specified only once\n"
                                "Run with arguments: [-b RUN_TIME: >= 1 && <= 7000] [--] PI: >= 1 <= SEM_VALUE_MAX, "
                                "IG: >= 1 && <= 2000, JG: >= 1 && <= 2000, IT >= 1 && <= 2000.\n");
                return ERR_EXIT;
            }
        }
        else if (option == ':') // -b without parametr
        {
            fprintf(stderr, "main process: Option -b requires a parametr\n"
                            "Run with arguments: [-b RUN_TIME: >= 1 && <= 7000] [--] PI: >= 1 <= SEM_VALUE_MAX, "
                            "IG: >= 1 && <= 2000, JG: >= 1 && <= 2000, IT >= 1 && <= 2000.\n");
            return ERR_EXIT;
        }
        else if (option == '?') // invalid option
        {
            fprintf(stderr, "main process: Invalid option\n"
                            "Run with arguments: [-b RUN_TIME: >= 1 && <= 7000] [--] PI: >= 1 <= SEM_VALUE_MAX, "
                            "IG: >= 1 && <= 2000, JG: >= 1 && <= 2000, IT >= 1 && <= 2000.\n");
            return ERR_EXIT;
        }
    }


    if (argc - optind + 1 != PARAM_NUM)
    {
        fprintf(stderr, "main process: Wrong number of arguments.\n"
                        "Run with arguments: [-b RUN_TIME: >= 1 && <= 7000] [--] PI: >= 1 <= SEM_VALUE_MAX, IG: >= 1 && <= 2000,"
                        " JG: >= 1 && <= 2000, IT >= 1 && <= 2000.\n");
        return ERR_EXIT;
    }
    
    //the maximamal limit of immigrants has to be <= INT_MAX, as semaphores takes INT as parametr
    arguments->im_proc = strtol(IM_PROC, &strtol_check, DECIMAL);
    if (PROCES_NUM_CHECK(arguments->im_proc) || (strtol_check != NULL && *strtol_check != '\0'))
    {
        ARGS_ERR_PRINT(1); //error message + usage
        return ERR_EXIT;
    }

    arguments->im_gen_time = strtol(IM_GEN_TM, &strtol_check, DECIMAL);
    if ((strtol_check != NULL && *strtol_check != '\0') || ARGS_TIME_CHECK(arguments->im_gen_time))
    {
        ARGS_ERR_PRINT(2); //error message + usage
        return ERR_EXIT;
    }
    arguments->im_gen_time++; //increased by 1, so that modulo gives the right value

    arguments->jg_enter_time = strtol(JG_ENTER_TM, &strtol_check, DECIMAL);
    if ((strtol_check != NULL && *strtol_check != '\0') || ARGS_TIME_CHECK(arguments->jg_enter_time))
    {
        ARGS_ERR_PRINT(3); //error message + usage
        return ERR_EXIT;
    }
    arguments->jg_enter_time++; //increased by 1, so that modulo gives the right value

    arguments->certif_pick_time = strtol(CERTIF_PICK_TM, &strtol_check, DECIMAL);
    if ((strtol_check != NULL && *strtol_check != '\0') || ARGS_TIME_CHECK(arguments->certif_pick_time))
    {
        ARGS_ERR_PRINT(4); //error message + usage
        return ERR_EXIT;
    }
    arguments->certif_pick_time++; //increased by 1, so that modulo gives the right value

    arguments->jg_verdict_time = strtol(JG_VERDICT_TM, &strtol_check, DECIMAL);
    if ((strtol_check != NULL && *strtol_check != '\0') || ARGS_TIME_CHECK(arguments->jg_verdict_time))
    {
        ARGS_ERR_PRINT(5); //error message + usage
        return ERR_EXIT;
    }
    arguments->jg_verdict_time++; //increased by 1, so that modulo gives the right value

    return 0;
}

int shared_mem_init(int jg_sem_open_val)
{
    //creates memory mapping, which can be shared between processes
    MMAP_ARR(shared_mem, SHARED_MEM_ARR_SIZE);
    if(shared_mem == (void *)-1)
    {
        perror("shared memory initialization");

        return ERR_EXIT;
    }

    //initiates the shared memory to 0
    memset(shared_mem, 0, SHARED_MEM_ARR_SIZE*sizeof(long));

    //opens file for writing
    fptr_p2_out = fopen("proj2.out", "w");
    if (fptr_p2_out == NULL)
    {
        perror("shared memory initialization");

        if(UNMAP_ARR(shared_mem, SHARED_MEM_ARR_SIZE))
            fprintf(stderr, "Memory unmaping failed.\n");
        
        return ERR_EXIT;
    }

    //opens semaphores that are needed for synchronization of the processes

    if((var_change_semaphore = sem_open("/xmihol00_var_sem_bonus", O_CREAT | O_EXCL, 0666, 1)) == SEM_FAILED)
    {
        perror("shared memory initialization");
  
        if(fclose(fptr_p2_out))
            fprintf(stderr, "File could not be closed properly.\n");

        if(UNMAP_ARR(shared_mem, SHARED_MEM_ARR_SIZE))
            fprintf(stderr, "Memory unmaping failed.\n");
        
        return ERR_EXIT;
    }

    if((jg_entry_semaphore = sem_open("/xmihol00_jge_sem_bonus", O_CREAT | O_EXCL, 0666, 1)) == SEM_FAILED)
    {
        perror("shared memory initialization");
  
        if(fclose(fptr_p2_out))
            fprintf(stderr, "File could not be closed properly.\n");

        if(UNMAP_ARR(shared_mem, SHARED_MEM_ARR_SIZE))
            fprintf(stderr, "Memory unmaping failed.\n");

        if(sem_close(var_change_semaphore))
            fprintf(stderr, "var_change_semaphore could not be closed properly.\n");

        if(sem_unlink("/xmihol00_var_sem_bonus"))
            fprintf(stderr, "/xmihol00_var_sem_bonus could not be unlinked properly.\n");
        
        return ERR_EXIT;
    }

    if((decision_semaphore = sem_open("/xmihol00_dec_sem_bonus", O_CREAT | O_EXCL, 0666, 0)) == SEM_FAILED)
    {
        perror("shared memory initialization");
  
        if(fclose(fptr_p2_out))
            fprintf(stderr, "File could not be closed properly.\n");

        if(UNMAP_ARR(shared_mem, SHARED_MEM_ARR_SIZE))
            fprintf(stderr, "Memory unmaping failed.\n");

        if(sem_close(var_change_semaphore))
            fprintf(stderr, "var_change_semaphore could not be closed properly.\n");

        if(sem_unlink("/xmihol00_var_sem_bonus"))
            fprintf(stderr, "/xmihol00_var_sem_bonus could not be unlinked properly.\n");
        
        if(sem_close(jg_entry_semaphore))
            fprintf(stderr, "jg_entry_semaphore could not be closed properly.\n");

        if(sem_unlink("/xmihol00_jge_sem_bonus"))
            fprintf(stderr, "/xmihol00_jge_sem_bonus could not be unlinked properly.\n");
        
        return ERR_EXIT;
    }

    if((jg_wait_semaphore = sem_open("/xmihol00_jgw_sem_bonus", O_CREAT | O_EXCL, 0666, 0)) == SEM_FAILED)
    {
        perror("shared memory initialization");
  
        if(fclose(fptr_p2_out))
            fprintf(stderr, "File could not be closed properly.\n");

        if(UNMAP_ARR(shared_mem, SHARED_MEM_ARR_SIZE))
            fprintf(stderr, "Memory unmaping failed.\n");

        if(sem_close(var_change_semaphore))
            fprintf(stderr, "var_change_semaphore could not be closed properly.\n");

        if(sem_unlink("/xmihol00_var_sem_bonus"))
            fprintf(stderr, "/xmihol00_var_sem_bonus could not be unlinked properly.\n");
        
        if(sem_close(jg_entry_semaphore))
            fprintf(stderr, "jg_entry_semaphore could not be closed properly.\n");

        if(sem_unlink("/xmihol00_jge_sem_bonus"))
            fprintf(stderr, "/xmihol00_jge_sem_bonus could not be unlinked properly.\n");
        
        if(sem_close(decision_semaphore))
            fprintf(stderr, "decision_semaphore could not be closed properly.\n");

        if(sem_unlink("/xmihol00_dec_sem_bonus"))
            fprintf(stderr, "/xmihol00_dec_sem_bonus could not be unlinked properly.\n");

        return ERR_EXIT;
    }

    if((pick_up_semaphore = sem_open("/xmihol00_pic_sem_bonus", O_CREAT | O_EXCL, 0666, jg_sem_open_val)) == SEM_FAILED)
    {
        perror("shared memory initialization");
  
        if(fclose(fptr_p2_out))
            fprintf(stderr, "File could not be closed properly.\n");

        if(UNMAP_ARR(shared_mem, SHARED_MEM_ARR_SIZE))
            fprintf(stderr, "Memory unmaping failed.\n");

        if(sem_close(var_change_semaphore))
            fprintf(stderr, "var_change_semaphore could not be closed properly.\n");

        if(sem_unlink("/xmihol00_var_sem_bonus"))
            fprintf(stderr, "/xmihol00_var_sem_bonus could not be unlinked properly.\n");
        
        if(sem_close(jg_entry_semaphore))
            fprintf(stderr, "jg_entry_semaphore could not be closed properly.\n");

        if(sem_unlink("/xmihol00_jge_sem_bonus"))
            fprintf(stderr, "/xmihol00_jge_sem_bonus could not be unlinked properly.\n");
        
        if(sem_close(decision_semaphore))
            fprintf(stderr, "decision_semaphore could not be closed properly.\n");

        if(sem_unlink("/xmihol00_dec_sem_bonus"))
            fprintf(stderr, "/xmihol00_dec_sem_bonus could not be unlinked properly.\n");
        
        if(sem_close(jg_wait_semaphore))
            fprintf(stderr, "jg_wait_semaphore could not be closed properly.\n");

        if(sem_unlink("/xmihol00_jgw_sem_bonus"))
            fprintf(stderr, "/xmihol00_jgw_sem_bonus could not be unlinked properly.\n");

        return ERR_EXIT;
    }

    if((together_semaphore = sem_open("/xmihol00_tgh_sem_bonus", O_CREAT | O_EXCL, 0666, 1)) == SEM_FAILED)
    {
        perror("shared memory initialization");
  
        if(fclose(fptr_p2_out))
            fprintf(stderr, "File could not be closed properly.\n");

        if(UNMAP_ARR(shared_mem, SHARED_MEM_ARR_SIZE))
            fprintf(stderr, "Memory unmaping failed.\n");

        if(sem_close(var_change_semaphore))
            fprintf(stderr, "var_change_semaphore could not be closed properly.\n");

        if(sem_unlink("/xmihol00_var_sem_bonus"))
            fprintf(stderr, "/xmihol00_var_sem_bonus could not be unlinked properly.\n");
        
        if(sem_close(jg_entry_semaphore))
            fprintf(stderr, "jg_entry_semaphore could not be closed properly.\n");

        if(sem_unlink("/xmihol00_jge_sem_bonus"))
            fprintf(stderr, "/xmihol00_jge_sem_bonus could not be unlinked properly.\n");
        
        if(sem_close(decision_semaphore))
            fprintf(stderr, "decision_semaphore could not be closed properly.\n");

        if(sem_unlink("/xmihol00_dec_sem_bonus"))
            fprintf(stderr, "/xmihol00_dec_sem_bonus could not be unlinked properly.\n");
        
        if(sem_close(jg_wait_semaphore))
            fprintf(stderr, "jg_wait_semaphore could not be closed properly.\n");

        if(sem_unlink("/xmihol00_jgw_sem_bonus"))
            fprintf(stderr, "/xmihol00_jgw_sem_bonus could not be unlinked properly.\n");

        if(sem_close(pick_up_semaphore))
            fprintf(stderr, "pick_up_semaphore could not be closed properly.\n");

        if(sem_unlink("/xmihol00_pic_sem_bonus"))
            fprintf(stderr, "/xmihol00_pic_sem_bonus could not be unlinked properly.\n");

        return ERR_EXIT;
    }

    if((jg_leave_semaphore = sem_open("/xmihol00_jgl_sem_bonus", O_CREAT | O_EXCL, 0666, 0)) == SEM_FAILED)
    {
        perror("shared memory initialization");
  
        if(fclose(fptr_p2_out))
            fprintf(stderr, "File could not be closed properly.\n");

        if(UNMAP_ARR(shared_mem, SHARED_MEM_ARR_SIZE))
            fprintf(stderr, "Memory unmaping failed.\n");

        if(sem_close(var_change_semaphore))
            fprintf(stderr, "var_change_semaphore could not be closed properly.\n");

        if(sem_unlink("/xmihol00_var_sem_bonus"))
            fprintf(stderr, "/xmihol00_var_sem_bonus could not be unlinked properly.\n");
        
        if(sem_close(jg_entry_semaphore))
            fprintf(stderr, "jg_entry_semaphore could not be closed properly.\n");

        if(sem_unlink("/xmihol00_jge_sem_bonus"))
            fprintf(stderr, "/xmihol00_jge_sem_bonus could not be unlinked properly.\n");
        
        if(sem_close(decision_semaphore))
            fprintf(stderr, "decision_semaphore could not be closed properly.\n");

        if(sem_unlink("/xmihol00_dec_sem_bonus"))
            fprintf(stderr, "/xmihol00_dec_sem_bonus could not be unlinked properly.\n");
        
        if(sem_close(jg_wait_semaphore))
            fprintf(stderr, "jg_wait_semaphore could not be closed properly.\n");

        if(sem_unlink("/xmihol00_jgw_sem_bonus"))
            fprintf(stderr, "/xmihol00_jgw_sem_bonus could not be unlinked properly.\n");

        if(sem_close(pick_up_semaphore))
            fprintf(stderr, "pick_up_semaphore could not be closed properly.\n");

        if(sem_unlink("/xmihol00_pic_sem_bonus"))
            fprintf(stderr, "/xmihol00_pic_sem_bonus could not be unlinked properly.\n");

        if(sem_close(together_semaphore))
            fprintf(stderr, "together_semaphore could not be closed properly.\n");

        if(sem_unlink("/xmihol00_tgh_sem_bonus"))
            fprintf(stderr, "/xmihol00_tgh_sem_bonus could not be unlinked properly.\n");

        return ERR_EXIT;
    }

    if((gen_notif_semaphore = sem_open("/xmihol00_gnf_sem_bonus", O_CREAT | O_EXCL, 0666, 0)) == SEM_FAILED)
    {
        perror("shared memory initialization");
  
        if(fclose(fptr_p2_out))
            fprintf(stderr, "File could not be closed properly.\n");

        if(UNMAP_ARR(shared_mem, SHARED_MEM_ARR_SIZE))
            fprintf(stderr, "Memory unmaping failed.\n");

        if(sem_close(var_change_semaphore))
            fprintf(stderr, "var_change_semaphore could not be closed properly.\n");

        if(sem_unlink("/xmihol00_var_sem_bonus"))
            fprintf(stderr, "/xmihol00_var_sem_bonus could not be unlinked properly.\n");
        
        if(sem_close(jg_entry_semaphore))
            fprintf(stderr, "jg_entry_semaphore could not be closed properly.\n");

        if(sem_unlink("/xmihol00_jge_sem_bonus"))
            fprintf(stderr, "/xmihol00_jge_sem_bonus could not be unlinked properly.\n");
        
        if(sem_close(decision_semaphore))
            fprintf(stderr, "decision_semaphore could not be closed properly.\n");

        if(sem_unlink("/xmihol00_dec_sem_bonus"))
            fprintf(stderr, "/xmihol00_dec_sem_bonus could not be unlinked properly.\n");
        
        if(sem_close(jg_wait_semaphore))
            fprintf(stderr, "jg_wait_semaphore could not be closed properly.\n");

        if(sem_unlink("/xmihol00_jgw_sem_bonus"))
            fprintf(stderr, "/xmihol00_jgw_sem_bonus could not be unlinked properly.\n");

        if(sem_close(pick_up_semaphore))
            fprintf(stderr, "pick_up_semaphore could not be closed properly.\n");

        if(sem_unlink("/xmihol00_pic_sem_bonus"))
            fprintf(stderr, "/xmihol00_pic_sem_bonus could not be unlinked properly.\n");

        if(sem_close(together_semaphore))
            fprintf(stderr, "together_semaphore could not be closed properly.\n");

        if(sem_unlink("/xmihol00_tgh_sem_bonus"))
            fprintf(stderr, "/xmihol00_tgh_sem_bonus could not be unlinked properly.\n");
        
        if(sem_close(jg_leave_semaphore))
            fprintf(stderr, "jg_leave_semaphore could not be closed properly.\n");

        if(sem_unlink("/xmihol00_jgl_sem_bonus"))
            fprintf(stderr, "/xmihol00_jgl_sem_bonus could not be unlinked properly.\n");

        return ERR_EXIT;
    }

    return 0;
}

void main_process_fork(pid_t *immigrant_main_pid, pid_t *judge_pid, arguments_t *arguments)
{    
    pid_t timer_pid;
    //all new generated processes and the main process block the QUIT, USR1 and USR2 signals,     
    //the immigrant_genrator and rt_timer will set their handler for the signal immidiately, judge will continue ignoring
    if (arguments->run_time) //if the run-time option is specified, timer is created
    {    
        if ((timer_pid = fork()) == 0)
        {
            rt_timer(arguments->run_time);
        }
        else if (timer_pid == -1)
        {
            perror("timer fork");

            //allocated resources are freed
            shared_mem_free();
            exit(ERR_EXIT);
        }
    }

    //immigrant process is forked
    if (((*immigrant_main_pid) = fork()) == 0)
    {
        immigrant_main_process(arguments);
    }
    else if ((*immigrant_main_pid) == -1)
    {
        perror("immigrant fork");
        //no other processes were created at this point, the program can end (unsucessfully) after clearing allocated resources

        kill(0, SIGKILL); //all other generated processes are killed, apart from the main process       
        shared_mem_free();
        exit(ERR_EXIT);
    }

    //the judge process is forked
    if (((*judge_pid) = fork()) == 0)
    {
        judge_main_process(arguments->jg_enter_time, arguments->jg_verdict_time, arguments->run_time);
    }
    else if ((*judge_pid) == -1)
    {
        perror("judge fork");
        kill(0, SIGKILL); //all other generated processes are killed, apart from the main process
        shared_mem_free();
        exit(ERR_EXIT);
    }

}

void rt_timer(long run_time)
{
    //the default handling of sig_quit is used
    sigset_t empty_mask;
    sigset_t block_mask;
    sigemptyset(&empty_mask);
    struct sigaction sigquit = {0, };
    sigquit.sa_handler = sig_quit_handler_end;
    sigquit.sa_mask = empty_mask;
    //signal(SIGQUIT, sig_quit_handler_end);
    if (sigaction(SIGQUIT, &sigquit, NULL))
    {
        perror("timer sigaction");
        kill(0, SIGKILL);
        shared_mem_free();
        exit(ERR_EXIT);
    }
    sigemptyset(&block_mask);
    sigaddset(&block_mask, SIGQUIT);
    if (sigprocmask(SIG_UNBLOCK, &block_mask, NULL))
    {
        perror("timer sigprocmask");
        kill(0, SIGKILL);
        shared_mem_free();
        exit(ERR_EXIT);
    }

    usleep(run_time * 1000); //waits run_time milliseconds
    //informs other processes

    kill(0, SIGUSR1);
    exit(0);
}

void shared_mem_free()
{
    //all of the allocated resources are freed

    if(UNMAP_ARR(shared_mem, SHARED_MEM_ARR_SIZE))
        fprintf(stderr, "Memory unmaping failed.\n");

    if(fclose(fptr_p2_out))
        fprintf(stderr, "File could not be closed properly.\n");

    if(sem_close(var_change_semaphore))
        fprintf(stderr, "var_change_semaphore could not be closed properly.\n");

    if(sem_unlink("/xmihol00_var_sem_bonus"))
        fprintf(stderr, "/xmihol00_var_sem_bonus could not be unlinked properly.\n");

    if(sem_close(jg_entry_semaphore))
        fprintf(stderr, "jg_entry_semaphore could not be closed properly.\n");

    if(sem_unlink("/xmihol00_jge_sem_bonus"))
        fprintf(stderr, "/xmihol00_jge_sem_bonus could not be unlinked properly.\n");

    if(sem_close(decision_semaphore))
        fprintf(stderr, "decision_semaphore could not be closed properly.\n");

    if(sem_unlink("/xmihol00_dec_sem_bonus"))
        fprintf(stderr, "/xmihol00_dec_sem_bonus could not be unlinked properly.\n");
    
    if(sem_close(jg_wait_semaphore))
        fprintf(stderr, "jg_wait_semaphore could not be closed properly.\n");

    if(sem_unlink("/xmihol00_jgw_sem_bonus"))
        fprintf(stderr, "/xmihol00_jgw_sem_bonus could not be unlinked properly.\n");
    
    if(sem_close(pick_up_semaphore))
        fprintf(stderr, "pick_up_semaphore could not be closed properly.\n");

    if(sem_unlink("/xmihol00_pic_sem_bonus"))
        fprintf(stderr, "/xmihol00_pic_sem_bonus could not be unlinked properly.\n");
    
    if(sem_close(together_semaphore))
        fprintf(stderr, "together_semaphore could not be closed properly.\n");

    if(sem_unlink("/xmihol00_tgh_sem_bonus"))
        fprintf(stderr, "/xmihol00_tgh_sem_bonus could not be unlinked properly.\n");
    
    if(sem_close(jg_leave_semaphore))
        fprintf(stderr, "jg_leave_semaphore could not be closed properly.\n");

    if(sem_unlink("/xmihol00_jgl_sem_bonus"))
        fprintf(stderr, "/xmihol00_jgl_sem_bonus could not be unlinked properly.\n");

    if(sem_close(gen_notif_semaphore))
        fprintf(stderr, "gen_notif_semaphore could not be closed properly.\n");

    if(sem_unlink("/xmihol00_gnf_sem_bonus"))
        fprintf(stderr, "/xmihol00_gnf_sem_bonus could not be unlinked properly.\n");
}

void all_sems_unlink()
{
    sem_unlink("/xmihol00_var_sem_bonus");
    sem_unlink("/xmihol00_jge_sem_bonus");
    sem_unlink("/xmihol00_dec_sem_bonus");
    sem_unlink("/xmihol00_jgw_sem_bonus");
    sem_unlink("/xmihol00_pic_sem_bonus");
    sem_unlink("/xmihol00_tgh_sem_bonus");
    sem_unlink("/xmihol00_jgl_sem_bonus");
    sem_unlink("/xmihol00_gnf_sem_bonus");
}

void main_sig_quit_handler_free(int signal)
{
    //engages the program safe termination
    (void)signal;
    shared_mem_free();
    exit(0);
}

void sig_quit_handler_end(int signal)
{
    (void)signal;
    exit(0);
}
