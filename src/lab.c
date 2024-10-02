#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <sys/types.h>
#include <termios.h>
#include <unistd.h>
#include "lab.h"
#include <readline/readline.h>
#include <readline/history.h>
#include <pwd.h>
#include <limits.h>
#include <errno.h>
#include <termios.h>
#include <signal.h>


/**
 * This class defines the methods used to manage the shell
 * @author Levi Recla
 * @date 10/01/2024
 */

  /**
   * @brief Set the shell prompt. This function will attempt to load a prompt
   * from the requested environment variable, if the environment variable is
   * not set a default prompt of "shell>" is returned.  This function calls
   * malloc internally and the caller must free the resulting string.
   *
   * @param env The environment variable
   * @return const char* The prompt
   */
char *get_prompt(const char *env) {
    const char *env_prompt = getenv(env);
    char *prompt;

    if (env_prompt && *env_prompt) {
        prompt = strdup(env_prompt);
        if (!prompt) {
            fprintf(stderr, "Error: strdup failed in get_prompt\n");
            exit(1);
        }
    } else {
        prompt = strdup("shell>");
        if (!prompt) {
            fprintf(stderr, "Error: strdup failed in get_prompt\n");
            exit(1);
        }
    }

    return prompt;
}

  /**
   * Changes the current working directory of the shell. Uses the linux system
   * call chdir. With no arguments the users home directory is used as the
   * directory to change to.
   *
   * @param dir The directory to change to
   * @return  On success, zero is returned.  On error, -1 is returned, and
   * errno is set to indicate the error.
   */
int change_dir(char **dir) {
    char *path;

    if (dir[1] == NULL) {
        // No argument provided, change to home directory
        path = getenv("HOME");
        if (path == NULL) {
            // getenv failed, use getuid and getpwuid
            uid_t uid = getuid();
            struct passwd *pw = getpwuid(uid);
            if (pw == NULL) {
                fprintf(stderr, "Error: Could not determine home directory.\n");
                return -1;
            }
            path = pw->pw_dir;
        }
    } else {
        path = dir[1];
    }

    if (chdir(path) != 0) {
        perror("chdir");
        return -1;
    }

    return 0;
}

  /**
   * @brief Convert line read from the user into to format that will work with
   * execvp. We limit the number of arguments to ARG_MAX loaded from sysconf.
   * This function allocates memory that must be reclaimed with the cmd_free
   * function.
   *
   * @param line The line to process
   *
   * @return The line read in a format suitable for exec
   */
char **cmd_parse(const char *line) {
    char *line_copy = strdup(line);  // Make a copy of the input line
    if (!line_copy) {
        perror("strdup");
        return NULL;
    }

    int bufsize = 64;  // Reasonable initial size
    int position = 0;
    char **tokens = malloc(bufsize * sizeof(char *));
    if (!tokens) {
        perror("malloc");
        free(line_copy);  // Free line_copy if malloc for tokens fails
        return NULL;
    }

    char *token = strtok(line_copy, " \t\r\n\a");
    while (token != NULL) {
        tokens[position] = strdup(token);  // Duplicate each token
        if (!tokens[position]) {
            perror("strdup");
            // Free all previously allocated tokens in case of failure
            for (int i = 0; i < position; i++) {
                free(tokens[i]);
            }
            free(tokens);      // Free the tokens array
            free(line_copy);   // Free the line_copy
            return NULL;
        }
        position++;

        // Reallocate the tokens array if necessary
        if (position >= bufsize) {
            bufsize *= 2;  // Double the buffer size
            char **new_tokens = realloc(tokens, bufsize * sizeof(char *));
            if (!new_tokens) {
                perror("realloc");
                // Free previously allocated tokens
                for (int i = 0; i < position; i++) {
                    free(tokens[i]);
                }
                free(tokens);      // Free the tokens array
                free(line_copy);   // Free the line_copy
                return NULL;
            }
            tokens = new_tokens;
        }

        token = strtok(NULL, " \t\r\n\a");
    }

    tokens[position] = NULL;  // Null-terminate the tokens array

    free(line_copy);  // Free the copy of the input line
    return tokens;
}





  /**
   * @brief Free the line that was constructed with parse_cmd
   *
   * @param line the line to free
   */
void cmd_free(char **line) {
    if (line) {
        for (int i = 0; line[i] != NULL; i++) {
            free(line[i]);  // Free each individual token
        }
        free(line);  // Free the array itself
    }
}


  /**
   * @brief Trim the whitespace from the start and end of a string.
   * For example "   ls -a   " becomes "ls -a". This function modifies
   * the argument line so that all printable chars are moved to the
   * front of the string
   *
   * @param line The line to trim
   * @return The new line with no whitespace
   */
char *trim_white(char *line) {
    char *end;

    // Check for NULL or empty string
    if (line == NULL || *line == '\0') {
        return line;
    }

    // Trim leading spaces
    while (isspace((unsigned char)*line)) {
        line++;
    }

    // If all spaces (or empty string after trimming), return the empty string
    if (*line == '\0') {
        return line;
    }

    // Trim trailing spaces
    end = line + strlen(line) - 1;
    while (end > line && isspace((unsigned char)*end)) {
        end--;
    }

    // Null-terminate the string after the last non-whitespace character
    *(end + 1) = '\0';

    return line;
}

    /**
   * @brief Takes an argument list and checks if the first argument is a
   * built in command such as exit, cd, jobs, etc. If the command is a
   * built in command this function will handle the command and then return
   * true. If the first argument is NOT a built in command this function will
   * return false.
   *
   * @param sh The shell
   * @param argv The command to check
   * @return True if the command was a built in command
   */
bool do_builtin(struct shell *sh, char **argv) {
    if (strcmp(argv[0], "cd") == 0) {
        if (change_dir(argv) != 0) {
            // Error message already printed in change_dir
        }
        return true;  // Command was a built-in
    } else if (strcmp(argv[0], "exit") == 0) {
        exit(0);  // Exit the shell
    } else if (strcmp(argv[0], "history") == 0) {
        // Handle the history command
        HIST_ENTRY **history_list = history_list;
        if (history_list) {
            for (int i = 0; history_list[i] != NULL; i++) {
                printf("%d: %s\n", i + history_base, history_list[i]->line);
            }
        }
        return true;  // Command was a built-in
    }

    return false;  // Not a built-in command
}



    /**
   * @brief Initialize the shell for use. Allocate all data structures
   * Grab control of the terminal and put the shell in its own
   * process group. NOTE: This function will block until the shell is
   * in its own program group. Attaching a debugger will always cause
   * this function to fail because the debugger maintains control of
   * the subprocess it is debugging.
   *
   * @param sh
   */
  void sh_init(struct shell *sh) {
    // Check if shell is running interactively
    sh->shell_terminal = STDIN_FILENO;
    sh->shell_is_interactive = isatty(sh->shell_terminal);

    if (sh->shell_is_interactive) {
        while (tcgetpgrp(sh->shell_terminal) != (sh->shell_pgid = getpgrp())) {
            kill(-sh->shell_pgid, SIGTTIN);
        }

        //Ignore interactive and job-control signals
        signal(SIGINT, SIG_IGN);   // Ignore Ctrl+C
        signal(SIGQUIT, SIG_IGN);  // Ignore Ctrl
        signal(SIGTSTP, SIG_IGN);  // Ignore Ctrl+Z
        signal(SIGTTIN, SIG_IGN);
        signal(SIGTTOU, SIG_IGN);

        //Put shell in own process group
        sh->shell_pgid = getpid();
        if (setpgid(sh->shell_pgid, sh->shell_pgid) < 0) {
            perror("Couldn't put the shell in its own process group");
            exit(1);
        }

        //Get control of terminal
        tcsetpgrp(sh->shell_terminal, sh->shell_pgid);

        //Save default terminal attributes for shell
        tcgetattr(sh->shell_terminal, &sh->shell_tmodes);

    }

      }



  /**
   * @brief Destroy shell. Free any allocated memory and resources and exit
   * normally.
   *
   * @param sh
   */
  void sh_destroy(struct shell *sh){

  }

  /**
   * @brief Parse command line args from the user when the shell was launched
   *
   * @param argc Number of args
   * @param argv The arg array
   */
  void parse_args(int argc, char **argv){
    int opt;
    while ((opt = getopt(argc, argv, "v")) != -1) {
      switch (opt) {
        case 'v':
            // Print the version using lab_VERSION_MAJOR and lab_VERSION_MINOR
            printf("Shell version: %d.%d\n", lab_VERSION_MAJOR, lab_VERSION_MINOR);
            exit(0);  // Exit after printing the version
        default:
          // Handle invalid options (in case other flags are added later)
          printf("Usage: %s [-v]\n", argv[0]);
          exit(1);
      }
    }
  }


