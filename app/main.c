#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <unistd.h>
#include <string.h>
#include <stdbool.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <readline/readline.h>
#include <readline/history.h>
#include "../src/lab.h"

#define MAX_JOBS 1024

//Defines the status of a job
typedef enum { RUNNING, DONE } job_status_t;

//Defines a job with a process ID, command, status, and whether it has been reported
struct job {
    int id;               // Job number
    pid_t pid;            // Process ID
    char *command;        // Command line
    job_status_t status;  // Job status (RUNNING or DONE)
    bool reported;        // Whether the job's DONE status has been reported via 'jobs' command
};

struct shell sh;            // Declare the shell structure
struct job jobs[MAX_JOBS];  // Array to keep track of background jobs
int job_count = 0;          // Counter for job IDs

/**
 * @brief Parse command line args from the user when the shell was launched
 *
 * @param argc Number of args
 * @param argv The arg array
 */
int main(int argc, char *argv[]) {

    //Parse command line args
    parse_args(argc, argv);

    char *line;
    char **cmd;

    sh_init(&sh);     // Initialize the shell

    using_history();  // Initialize history

    // Get the prompt from the environment or fallback to default
    sh.prompt = get_prompt("MY_PROMPT");

    // Check if prompt is NULL
    if (sh.prompt == NULL) {
        fprintf(stderr, "Error: prompt is NULL\n");
        exit(1);
    }

    // Shell loop to get user input and parse it
    while (1) {
        // Check for completed background jobs
        for (int i = 0; i < job_count; i++) {
            if (jobs[i].status == RUNNING) {
                int status;
                pid_t result = waitpid(jobs[i].pid, &status, WNOHANG);
                if (result == -1) {
                    perror("waitpid");
                } else if (result > 0) {
                    // Background job has completed
                    jobs[i].status = DONE;
                    jobs[i].reported = false;
                    printf("[%d] Done %s\n", jobs[i].id, jobs[i].command);
                }
            }
        }

        line = readline(sh.prompt);  // Readline with the prompt

        if (line == NULL) {  // Handle EOF (Ctrl+D)
            printf("\n");
            break;
        }

        if (line && *line) {
            add_history(line);  // Add the line to history
        }

        // Detect if the command is to be run in the background
        bool is_background = false;
        size_t line_length = strlen(line);
        if (line_length > 0) {
            // Remove trailing whitespace
            while (line_length > 0 && isspace((unsigned char)line[line_length - 1])) {
                line[--line_length] = '\0';
            }

            // Check for '&' at the end
            if (line_length > 0 && line[line_length - 1] == '&') {
                is_background = true;
                line[--line_length] = '\0';  // Remove '&' from the line

                // Remove any trailing whitespace after removing '&'
                while (line_length > 0 && isspace((unsigned char)line[line_length - 1])) {
                    line[--line_length] = '\0';
                }
            }
        }

        // Parse the command
        cmd = cmd_parse(line);
        if (cmd != NULL && cmd[0] != NULL) {
            if (strcmp(cmd[0], "exit") == 0) {
                cmd_free(cmd);
                free(line);
                break;  // Exit the shell loop on "exit"
            } else if (strcmp(cmd[0], "jobs") == 0) {
                // Handle 'jobs' command here
                for (int i = 0; i < job_count; i++) {
                    if (jobs[i].status == RUNNING) {
                        printf("[%d] %d Running %s &\n", jobs[i].id, jobs[i].pid, jobs[i].command);
                    } else if (jobs[i].status == DONE && !jobs[i].reported) {
                        printf("[%d] Done    %s &\n", jobs[i].id, jobs[i].command);
                        jobs[i].reported = true;  // Mark as reported via 'jobs' command
                    }
                }
                // Remove jobs that are DONE and reported
                int shift = 0;
                for (int i = 0; i < job_count; i++) {
                    if (jobs[i].status == DONE && jobs[i].reported) {
                        // Free the command string
                        free(jobs[i].command);
                        shift++;
                    } else if (shift > 0) {
                        // Shift the job down in the array
                        jobs[i - shift] = jobs[i];
                    }
                }
                job_count -= shift;
            } else if (!do_builtin(&sh, cmd)) {
                // Execute external command
                pid_t pid = fork();
                if (pid == -1) {
                    // Fork failed
                    perror("fork");
                } else if (pid == 0) {
                    // Child process
                    pid_t child_pid = getpid();

                    // Put the child in its own process group
                    if (setpgid(child_pid, child_pid) < 0) {
                        perror("setpgid");
                        exit(EXIT_FAILURE);
                    }

                    // If foreground process, take control of the terminal
                    if (!is_background) {
                        tcsetpgrp(sh.shell_terminal, child_pid);
                    }

                    // Reset signals to default in the child
                    signal(SIGINT, SIG_DFL);
                    signal(SIGQUIT, SIG_DFL);
                    signal(SIGTSTP, SIG_DFL);
                    signal(SIGTTIN, SIG_DFL);
                    signal(SIGTTOU, SIG_DFL);

                    execvp(cmd[0], cmd);
                    // If execvp returns, an error occurred
                    fprintf(stderr, "Error: Command not found: %s\n", cmd[0]);
                    exit(EXIT_FAILURE);
                } else {
                    // Parent process

                    // Assign job ID and store background job info
                    if (is_background) {
                        int job_id = ++job_count;
                        if (job_id > MAX_JOBS) {
                            fprintf(stderr, "Error: Maximum number of background jobs reached.\n");
                            job_count--;
                        } else {
                            jobs[job_id - 1].id = job_id;
                            jobs[job_id - 1].pid = pid;
                            jobs[job_id - 1].command = strdup(line);  // Store the original command
                            jobs[job_id - 1].status = RUNNING;
                            jobs[job_id - 1].reported = false;
                            printf("[%d] %d %s &\n", job_id, pid, jobs[job_id - 1].command);
                        }
                    } else {
                        // Wait for the child process to complete
                        int status;
                        pid_t wpid;

                        // Wait for foreground process
                        do {
                            wpid = waitpid(pid, &status, WUNTRACED);
                            if (wpid == -1) {
                                perror("waitpid");
                                break;
                            }
                        } while (!WIFEXITED(status) && !WIFSIGNALED(status));

                        // Return control of the terminal to the shell
                        tcsetpgrp(sh.shell_terminal, sh.shell_pgid);

                        // Restore shell's terminal settings
                        tcgetattr(sh.shell_terminal, &sh.shell_tmodes);
                    }
                }
            }
            cmd_free(cmd);  // Free the parsed command
        }
        free(line);  // Free the input line returned by readline
    }

    free(sh.prompt);  // Free the dynamically allocated prompt

    return 0;
}
