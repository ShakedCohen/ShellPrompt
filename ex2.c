// Shaked Cohen
// 207881962

#include <stdio.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

#define MAX_LENGTH_OF_COMMAND 1024
#define MAX_AMOUNT_OF_COMMAND_ARGS 512
#define MAX_AMOUNT_OF_BACKGROUND_PROCESSES 512

#define PRINT_ERROR fprintf(stderr, "Error in system call");
#define UPDATE_PREV_PROCESS_WD strcpy(previous_pwd,current_pwd);
#define PRINT_PROMPT_BEGIN printf("> ");
#define NOT_SET "not set"
#define HOME "HOME"

// Process struct contains pid and cmd the process do
typedef struct Process {
    pid_t pid;
    char cmd[MAX_LENGTH_OF_COMMAND];
} Process;

typedef struct Params_Info {

    char *parm_list[MAX_AMOUNT_OF_COMMAND_ARGS + 1];
    int struct_last_arg_index;
    int struct_background_flag;

} Params_Info;

void extract_params(char *all_cmd_line, Params_Info *params){

    // cmd_args contains the command name and then the args (array of strings)
    char cmd_args[MAX_AMOUNT_OF_COMMAND_ARGS][MAX_LENGTH_OF_COMMAND];

    // break full cmd string into args in order to store at cmd_arg array
    char *token;
    const char delimiter[2] = " ";
    //first token
    token = strtok(all_cmd_line, delimiter);
    int cmd_args_counter = 0;
    // the rest of the tokens
    while (token != NULL) {
        strcpy(cmd_args[cmd_args_counter], token);
        token = strtok(NULL, delimiter);
        cmd_args_counter++;
    }

    // add /bin/ before every command before send to exec
    char full_cmd[MAX_LENGTH_OF_COMMAND];
    strcpy(full_cmd, "/bin/");
    strcat(full_cmd, cmd_args[0]);
    strcpy(cmd_args[0], full_cmd);

    //add null at the end for execv
    int background_flag = 0;
    int parm_list_counter = 0;
    int i = 0;
    for (i = 0; i < cmd_args_counter; i++) {

        if (i == cmd_args_counter - 1 && strcmp(cmd_args[i], "&") == 0) {
            background_flag = 1;
            break;
        }
        params->parm_list[i] = cmd_args[i];
        parm_list_counter++;
    }

    params->parm_list[parm_list_counter] = NULL;
    parm_list_counter++;
    // index of last arg
    int last_arg_place = parm_list_counter - 2;
    params->struct_last_arg_index = last_arg_place;
    params->struct_background_flag = background_flag;

}

int main() {

    // Process_array in order to store background processes
    Process jobs_arr[MAX_AMOUNT_OF_BACKGROUND_PROCESSES];
    int process_counter = 0;

    // previous_pwd - path to previous working directory
    char previous_pwd[MAX_LENGTH_OF_COMMAND] = NOT_SET; // default

    while (1) {
        PRINT_PROMPT_BEGIN
        char all_cmd_line[MAX_LENGTH_OF_COMMAND];
        //dummy
        scanf(" ");
        //read all line
        fgets(all_cmd_line, MAX_LENGTH_OF_COMMAND - 1, stdin);
        //remove \n at the end, replace with \0
        all_cmd_line[strlen(all_cmd_line) - 1] = '\0';

        // create backup of line
        char cmd_copy[MAX_LENGTH_OF_COMMAND];
        strcpy(cmd_copy, all_cmd_line);

        Params_Info params;
        extract_params(all_cmd_line, &params);

        // exit
        if (strcmp(params.parm_list[0], "/bin/exit") == 0) {
            pid_t currPID = getpid();
            printf("%d\n", currPID);
            exit(0);
        }
        // cd
        else if (strcmp(params.parm_list[0], "/bin/cd") == 0) {
            pid_t currPID = getpid();
            printf("%d\n", currPID);

            // save current working dir
            char current_pwd[MAX_LENGTH_OF_COMMAND];
            if (getcwd(current_pwd, MAX_LENGTH_OF_COMMAND) == NULL) {
                PRINT_ERROR
            }
            // for global path:
            // "cd" or "cd ~ ..."
            if (strcmp(params.parm_list[params.struct_last_arg_index], "/bin/cd") == 0 ||
            strcmp(params.parm_list[1], "~") == 0) {
                // set working directory to HOME for global
                if (chdir(getenv(HOME)) != -1) {
                    // if successfully changed wd to home, update prev p working dir
                    UPDATE_PREV_PROCESS_WD
                } else {
                    PRINT_ERROR
                }
            } else if (strcmp(params.parm_list[1], "-") == 0) {
                // go to previous folder
                // chdir to father dir
                if (strcmp(previous_pwd, NOT_SET) != 0) {
                    // go to previous folder
                    if (chdir(previous_pwd) == -1) {
                        PRINT_ERROR
                    } else {
                        UPDATE_PREV_PROCESS_WD
                    }
                } else {
                    //OLDWD not set
                    fprintf(stderr, "cd: OLDWD not set\n");
                }
            } else {
                // change pwd path using chdir
                // set working directory to param path
                if (chdir(params.parm_list[1]) == -1) {
                    PRINT_ERROR
                } else {
                    UPDATE_PREV_PROCESS_WD
                }
            }
        }

        // jobs command
        else if (strcmp(params.parm_list[0], "/bin/jobs") == 0) {
            Process temp_process_arr[MAX_AMOUNT_OF_BACKGROUND_PROCESSES];
            int temp_num_of_processes = 0;

            // delete finished processes, update jobs array to include living processes alone
            // prints  the jobs array
            int i =0;
            for (i = 0; i < process_counter; i++) {
                // if pid is alive
                if (waitpid(jobs_arr[i].pid, NULL, WNOHANG) == 0) {
                    // pid alive, save it
                    temp_process_arr[temp_num_of_processes] = jobs_arr[i];
                    temp_num_of_processes++;
                }
            }
            // temp process arr now holds only active processes
            // copy back to jobs arr
            for (i = 0; i < temp_num_of_processes; i++) {
                jobs_arr[i] = temp_process_arr[i];
            }
            process_counter = temp_num_of_processes;

            // prints active processes
            for (i = 0; i < process_counter; i++) {
                printf("%ld ", (long) jobs_arr[i].pid);
                // clear the "&" if exists
                const char symbol[2] = "&";
                char *clear_sign_cmd;
                clear_sign_cmd = strtok(jobs_arr[i].cmd, symbol);
                printf("%s\n", clear_sign_cmd);
            }
        } else {
            // standard command
            pid_t pid;
            // create process for cmd
            if ((pid = fork()) == -1)
                perror("fork error");
            else if (pid == 0) {
                // child process
                if (strcmp(params.parm_list[0], "/bin/man") == 0){
                    //man requires special treatment
                    params.parm_list[0] = "man";
                    execvp("man", params.parm_list);
                } else {
                    execv(params.parm_list[0], params.parm_list);
                }
                PRINT_ERROR // shouldnt reach here
            } else {
                // parent shell process
                // print pid of child to screen
                printf("%d\n", pid);
                // if background_flag is on
                // run child in background
                // otherwise wait for child
                if (params.struct_background_flag) {
                    // append to jobs arr
                    Process curr_process;
                    curr_process.pid = pid;
                    strcpy(curr_process.cmd, cmd_copy); // save cmd all line
                    jobs_arr[process_counter] = curr_process;
                    process_counter++;
                } else {
                    // foreground process, wait for child
                    while (waitpid(pid, NULL, WNOHANG) == 0){
                    }
                }
            }
        }
    }
}