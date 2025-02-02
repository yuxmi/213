/**
 * @file tsh.c
 * @brief A tiny shell program with job control
 *
 * TODO: Delete this comment and replace it with your own.
 * <The line above is not a sufficient documentation.
 *  You will need to write your program documentation.
 *  Follow the 15-213/18-213/15-513 style guide at
 *  http://www.cs.cmu.edu/~213/codeStyle.html.>
 *
 * @author Miu Nakajima <mnakajim@andrew.cmu.edu>
 */

#include "csapp.h"
#include "tsh_helper.h"

#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <getopt.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

/*
 * If DEBUG is defined, enable contracts and printing on dbg_printf.
 */
#ifdef DEBUG
/* When debugging is enabled, these form aliases to useful functions */
#define dbg_printf(...) printf(__VA_ARGS__)
#define dbg_requires(...) assert(__VA_ARGS__)
#define dbg_assert(...) assert(__VA_ARGS__)
#define dbg_ensures(...) assert(__VA_ARGS__)
#else
/* When debugging is disabled, no code gets generated for these */
#define dbg_printf(...)
#define dbg_requires(...)
#define dbg_assert(...)
#define dbg_ensures(...)
#endif

/* Function prototypes */
void eval(const char *cmdline);

void sigchld_handler(int sig);
void sigtstp_handler(int sig);
void sigint_handler(int sig);
void sigquit_handler(int sig);
void cleanup(void);
bool builtin_cmd(struct cmdline_tokens tok);

/**
 * @brief <Write main's function header documentation. What does main do?>
 *
 * TODO: Delete this comment and replace it with your own.
 *
 * "Each function should be prefaced with a comment describing the purpose
 *  of the function (in a sentence or two), the function's arguments and
 *  return value, any error cases that are relevant to the caller,
 *  any pertinent side effects, and any assumptions that the function makes."
 */
int main(int argc, char **argv) {
    int c;
    char cmdline[MAXLINE_TSH]; // Cmdline for fgets
    bool emit_prompt = true;   // Emit prompt (default)

    // Redirect stderr to stdout (so that driver will get all output
    // on the pipe connected to stdout)
    if (dup2(STDOUT_FILENO, STDERR_FILENO) < 0) {
        perror("dup2 error");
        exit(1);
    }

    // Parse the command line
    while ((c = getopt(argc, argv, "hvp")) != EOF) {
        switch (c) {
        case 'h': // Prints help message
            usage();
            break;
        case 'v': // Emits additional diagnostic info
            verbose = true;
            break;
        case 'p': // Disables prompt printing
            emit_prompt = false;
            break;
        default:
            usage();
        }
    }

    // Create environment variable
    if (putenv(strdup("MY_ENV=42")) < 0) {
        perror("putenv error");
        exit(1);
    }

    // Set buffering mode of stdout to line buffering.
    // This prevents lines from being printed in the wrong order.
    if (setvbuf(stdout, NULL, _IOLBF, 0) < 0) {
        perror("setvbuf error");
        exit(1);
    }

    // Initialize the job list
    init_job_list();

    // Register a function to clean up the job list on program termination.
    // The function may not run in the case of abnormal termination (e.g. when
    // using exit or terminating due to a signal handler), so in those cases,
    // we trust that the OS will clean up any remaining resources.
    if (atexit(cleanup) < 0) {
        perror("atexit error");
        exit(1);
    }

    // Install the signal handlers
    Signal(SIGINT, sigint_handler);   // Handles Ctrl-C
    Signal(SIGTSTP, sigtstp_handler); // Handles Ctrl-Z
    Signal(SIGCHLD, sigchld_handler); // Handles terminated or stopped child

    Signal(SIGTTIN, SIG_IGN);
    Signal(SIGTTOU, SIG_IGN);

    Signal(SIGQUIT, sigquit_handler);

    // Execute the shell's read/eval loop
    while (true) {
        if (emit_prompt) {
            printf("%s", prompt);

            // We must flush stdout since we are not printing a full line.
            fflush(stdout);
        }

        if ((fgets(cmdline, MAXLINE_TSH, stdin) == NULL) && ferror(stdin)) {
            perror("fgets error");
            exit(1);
        }

        if (feof(stdin)) {
            // End of file (Ctrl-D)
            printf("\n");
            return 0;
        }

        // Remove any trailing newline
        char *newline = strchr(cmdline, '\n');
        if (newline != NULL) {
            *newline = '\0';
        }

        // Evaluate the command line
        eval(cmdline);
    }

    return -1; // control never reaches here
}

/**
 * @brief <What does eval do?>
 *
 * TODO: Delete this comment and replace it with your own.
 *
 * NOTE: The shell is supposed to be a long-running process, so this function
 *       (and its helpers) should avoid exiting on error.  This is not to say
 *       they shouldn't detect and print (or otherwise handle) errors!
 */
void eval(const char *cmdline) {
    parseline_return parse_result;
    struct cmdline_tokens token;
    pid_t pid;
    int bg;
    sigset_t mask;
    sigset_t prev;

    sigemptyset(&mask);
    sigaddset(&mask, SIGCHLD);
    sigaddset(&mask, SIGINT);
    sigaddset(&mask, SIGTSTP);
    sigprocmask(SIG_BLOCK, &mask, &prev);

    // Parse command line
    parse_result = parseline(cmdline, &token);

    if (parse_result == PARSELINE_ERROR || parse_result == PARSELINE_EMPTY) {
        return;
    }

    // TODO: Implement commands here.
    if (!builtin_cmd(token)) {

        pid = fork();
        if (pid < 0) {

            printf("error\n");
            exit(0);

        } else if (pid == 0) { /* Child */

            setpgid(0,0);
            sigprocmask(SIG_SETMASK, &prev, NULL); // Unblock SIGCHLD

            if (execve(token.argv[0], token.argv, environ) < 0) {
                printf("%s: Command not found.\n", token.argv[0]);
                exit(0);   
            }

        } else {

            bg = (parse_result == PARSELINE_BG);
            if (!bg) { // Foreground

                jid_t jid = add_job(pid, FG, cmdline);
                while (fg_job() == jid) {
                    sigsuspend(&prev);
                }

            } else { // Background

                jid_t jid = add_job(pid, BG, cmdline);
                printf("[%d] (%d) %s\n", jid, pid, cmdline);
                
            }

        }
    }

    sigprocmask(SIG_SETMASK, &prev, NULL); // Restore old signals

}

/*****************
 * Signal handlers
 *****************/

/**
 * @brief Handles signals for sigchld. Kernel sends sigchld to the shell when a 
 * child process terminates.
 */
void sigchld_handler(int sig) {

    int olderrno = errno;
    sigset_t mask_all;
    sigset_t prev_all;
    pid_t pid;
    jid_t jid;
    int status;

    sigfillset(&mask_all);

    while ((pid = waitpid(-1, &status, WNOHANG | WUNTRACED | WCONTINUED)) > 0) {

        sigprocmask(SIG_BLOCK, &mask_all, &prev_all);

        if (WIFSIGNALED(status)) {

            jid = job_from_pid(pid);
            printf("Job [%d] (%d) terminated by signal %d\n", jid, pid, 
                                                            WTERMSIG(status));
            delete_job(jid);

        } else if (WIFSTOPPED(status)) {

            jid = job_from_pid(pid);
            printf("Job [%d] (%d) stopped by signal %d\n", jid, pid, 
                                                            WSTOPSIG(status));
            job_set_state(jid, ST);

        } else if (WIFEXITED(status)) {

            jid = job_from_pid(pid);
            delete_job(jid); // Delete job from job list

        } 

    }

    sigprocmask(SIG_SETMASK, &prev_all, NULL);

    if (errno != ECHILD) {
        strerror(errno);
    }

    errno = olderrno;

}

/**
 * @brief <What does sigint_handler do?>
 *
 * TODO: Delete this comment and replace it with your own.
 */
void sigint_handler(int sig) {

    int olderrno = errno;
    sigset_t mask_all;
    sigset_t prev_all;
    pid_t pid;
    jid_t jid;

    sigfillset(&mask_all);
    sigprocmask(SIG_BLOCK, &mask_all, &prev_all);

    jid = fg_job();
    if (jid > 0) {
        pid = job_get_pid(jid);
        kill(-pid, SIGINT);
    }

    sigprocmask(SIG_SETMASK, &prev_all, NULL);

    errno = olderrno;

}

/**
 * @brief <What does sigtstp_handler do?>
 *
 * TODO: Delete this comment and replace it with your own.
 */
void sigtstp_handler(int sig) {

    int olderrno = errno;
    sigset_t mask_all;
    sigset_t prev_all;
    pid_t pid;
    jid_t jid;

    sigfillset(&mask_all);
    sigprocmask(SIG_BLOCK, &mask_all, &prev_all);

    jid = fg_job();
    if (jid > 0) {
        pid = job_get_pid(jid);
        kill(-pid, SIGTSTP);
    }

    sigprocmask(SIG_SETMASK, &prev_all, NULL);

    errno = olderrno;

}

/**
 * @brief Attempt to clean up global resources when the program exits.
 *
 * In particular, the job list must be freed at this time, since it may
 * contain leftover buffers from existing or even deleted jobs.
 */
void cleanup(void) {
    // Signals handlers need to be removed before destroying the joblist
    Signal(SIGINT, SIG_DFL);  // Handles Ctrl-C
    Signal(SIGTSTP, SIG_DFL); // Handles Ctrl-Z
    Signal(SIGCHLD, SIG_DFL); // Handles terminated or stopped child

    destroy_job_list();
}

/**
 * @brief Determines whether or not command is built-in
*/
bool builtin_cmd(struct cmdline_tokens tok) {

    if (!strcmp(tok.argv[0], "quit")) {

        exit(0);

    } else if (!strcmp(tok.argv[0], "jobs")) {

        list_jobs(STDOUT_FILENO);
        return true;

    } else if (BUILTIN_FG) {

        return true;

    } else if (BUILTIN_BG) {

        return true;
        
    }

    return false;

}
