/* Execute.c
 * Authors: Liv Sutton and Chloe Tran
 * 
 * This file contains the implementation of command execution functions
 * for the Quash shell. It manages the execution of commands, handles
 * background and foreground jobs, and manages input/output redirection.
 * The file includes functions for running generic commands, built-in 
 * commands like `cd`, `pwd`, `echo`, and managing job status.
 */

#include "execute.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <string.h>  
#include <limits.h>  

#include "quash.h"
#include "deque.h"

#define READ_END 0
#define WRITE_END 1

// Define a queue for process IDs
IMPLEMENT_DEQUE_STRUCT(pid_queue, pid_t);
IMPLEMENT_DEQUE(pid_queue, pid_t);
pid_queue process_id_queue;

// Define the structure for a Job
struct Job {
    int job_id;         // Unique job identifier
    char* command;      // Command string associated with the job
    pid_queue process_ids; // Queue of process IDs for the job
    pid_t first_pid;    // First process ID of the job
} Job;

// Define a queue for jobs
IMPLEMENT_DEQUE_STRUCT(job_queue, struct Job);
IMPLEMENT_DEQUE(job_queue, struct Job);
job_queue job_list; // Global job queue
int job_count = 1;  // Job counter

bool is_initialized = false; // Flag to check initialization status
static int pipes[2][2]; // Pipe array for inter-process communication

/***************************************************************************
 * Interface Functions
 ***************************************************************************/
// Returns the current working directory.
char* get_current_directory(bool* should_free) {
    char* wd = NULL;
    wd = getcwd(NULL, 0); // Get the current working directory
    return wd;
}

// Returns the value of an environment variable
const char* lookup_env(const char* env_var) {
    return getenv(env_var);
}

// Check the status of background jobs
void check_jobs_bg_status() {
    // Loop through jobs in the job queue
    int total_jobs = length_job_queue(&job_list);
    for (int j = 0; j < total_jobs; j++) {
        struct Job current_job = pop_front_job_queue(&job_list);

        // Loop through process IDs for the current job
        int total_pids = length_pid_queue(&current_job.process_ids);
        pid_t front_pid = peek_front_pid_queue(&current_job.process_ids);
        for (int p = 0; p < total_pids; p++) {
            pid_t current_pid = pop_front_pid_queue(&current_job.process_ids);
            int status;
            // Check if the child process has finished
            if (waitpid(current_pid, &status, WNOHANG) == 0) {
                push_back_pid_queue(&current_job.process_ids, current_pid); // Still running
            }
        }

        // If there are no more PIDs, the job is complete
        if (is_empty_pid_queue(&current_job.process_ids)) {
            print_job_bg_complete(current_job.job_id, front_pid, current_job.command);
        } else {
            push_back_job_queue(&job_list, current_job); // Still running job
        }
    }
}

// Prints the job id number, the process id of the first process belonging to
// the Job, and the command string associated with this job
void print_job(int job_id, pid_t pid, const char* command) {
    printf("[%d]\t%8d\t%s\n", job_id, pid, command);
    fflush(stdout);
}

// Prints a start up message for background processes
void print_job_bg_start(int job_id, pid_t pid, const char* command) {
    printf("Background job started: ");
    print_job(job_id, pid, command);
}

// Prints a completion message followed by the print job
void print_job_bg_complete(int job_id, pid_t pid, const char* command) {
    printf("Completed: \t");
    print_job(job_id, pid, command);
}

/***************************************************************************
 * Functions to process commands
 ***************************************************************************/
// Run a program reachable by the path environment variable, relative path, or absolute path
void run_generic(GenericCommand cmd) {
    char* executable = cmd.args[0]; // First argument as executable
    char** args = cmd.args;          // Arguments array

    // Check if the command is an absolute or relative path
    if (executable[0] == '/' || executable[0] == '.') {
        execvp(executable, args); // Execute command directly
    } else {
        // Search for the command in the PATH
        char* path = getenv("PATH");
        char* path_copy = strdup(path);
        char* token = strtok(path_copy, ":");

        while (token != NULL) {
            char full_path[PATH_MAX];
            snprintf(full_path, sizeof(full_path), "%s/%s", token, executable);

            if (access(full_path, X_OK) == 0) { // Check if executable
                execvp(full_path, args);
                break;
            }

            token = strtok(NULL, ":");
        }

        free(path_copy); // Free duplicated path string
    }

    perror("ERROR: Failed to execute program"); // Print error if execution fails
    exit(EXIT_FAILURE);
}

// Print strings
void run_echo(EchoCommand cmd) {
    char** strings = cmd.args; // Get the arguments for echo
    for (; *strings != NULL; ++strings) {
        printf("%s ", *strings); // Print each string
    }
    printf("\n");
    fflush(stdout); // Flush the buffer before returning
}

// Sets an environment variable
void run_export(ExportCommand cmd) {
    const char* env_var = cmd.env_var; // Get environment variable name
    const char* value = cmd.val;        // Get environment variable value
    setenv(env_var, value, 1);          // Set the environment variable
}

// Changes the current working directory
void run_cd(CDCommand cmd) {
    const char* directory = cmd.dir; // Get the directory to change to

    // Check if the directory is valid
    if (directory == NULL) {
        perror("ERROR: Failed to resolve path");
        return;
    }

    char resolved_path[102]; // Buffer for resolved path
    realpath(directory, resolved_path); // Resolve the absolute path
    chdir(resolved_path); // Change directory
    char cwd[1024];
    setenv("OLD_PWD", getcwd(cwd, sizeof(cwd)), 1); // Save old working directory
    setenv("PWD", directory, 1); // Set new working directory
}

// Sends a signal to all processes contained in a job
void run_kill(KillCommand cmd) {
    int signal = cmd.sig; // Signal to send
    int job_id = cmd.job; // Job identifier

    struct Job current_job;

    // Find the job in the job list
    for (int j = 0; j < length_job_queue(&job_list); j++) {
        current_job = pop_front_job_queue(&job_list);
        if (current_job.job_id == job_id) {
            // Kill all processes in the job
            pid_queue current_process_ids = current_job.process_ids;
            while (length_pid_queue(&current_process_ids) != 0) {
                pid_t current_pid = pop_front_pid_queue(&current_process_ids);
                kill(current_pid, signal);
            }
        }
        push_back_job_queue(&job_list, current_job); // Add job back to list
    }
}

// Prints the current working directory to stdout
void run_pwd() {
    char cwd[1024];
    printf("%s\n", getcwd(cwd, sizeof(cwd))); // Print current working directory
    fflush(stdout); // Flush the buffer before returning
}

// Prints all background jobs currently in the job list to stdout
void run_jobs() {
    int total_jobs = length_job_queue(&job_list);
    for (int j = 0; j < total_jobs; j++) {
        struct Job current_job = pop_front_job_queue(&job_list);
        print_job(current_job.job_id, current_job.first_pid, current_job.command); // Print job details
        push_back_job_queue(&job_list, current_job); // Add job back to list
    }
    fflush(stdout); // Flush the buffer before returning
}

/***************************************************************************
 * Functions for command resolution and process setup
 ***************************************************************************/
// Dispatch function for commands to run in child processes
void child_run_command(Command cmd) {
    CommandType type = get_command_type(cmd); // Get command type

    switch (type) {
        case GENERIC:
            run_generic(cmd.generic);
            break;

        case ECHO:
            run_echo(cmd.echo);
            break;

        case PWD:
            run_pwd();
            break;

        case JOBS:
            run_jobs();
            break;

        case EXPORT:
        case CD:
        case KILL:
        case EXIT:
        case EOC:
            break;

        default:
            fprintf(stderr, "Unknown command type: %d\n", type);
    }
}

// Dispatch function for commands to run in the parent process (quash)
void parent_run_command(Command cmd) {
    CommandType type = get_command_type(cmd); // Get command type

    switch (type) {
        case EXPORT:
            run_export(cmd.export);
            break;

        case CD:
            run_cd(cmd.cd);
            break;

        case KILL:
            run_kill(cmd.kill);
            break;

        case GENERIC:
        case ECHO:
        case PWD:
        case JOBS:
        case EXIT:
        case EOC:
            break;

        default:
            fprintf(stderr, "Unknown command type: %d\n", type);
    }
}

// Creates a new process for the given command in the CommandHolder, setting up redirects and pipes
void create_process(CommandHolder holder, int index) {
    // Read flags from the parser
    bool pipe_in = holder.flags & PIPE_IN;
    bool pipe_out = holder.flags & PIPE_OUT;
    bool redirect_in = holder.flags & REDIRECT_IN;
    bool redirect_out = holder.flags & REDIRECT_OUT;
    bool redirect_append = holder.flags & REDIRECT_APPEND;

    int write_end = index % 2; // Determine write end for pipe
    int read_end = (index - 1) % 2; // Determine read end for pipe

    if (pipe_out) {
        pipe(pipes[write_end]); // Create pipe for output
    }
    pid_t pid = fork(); // Create new process

    push_back_pid_queue(&process_id_queue, pid); // Add PID to queue
    if (pid == 0) {
        // Child process
        if (pipe_in) {
            dup2(pipes[read_end][READ_END], STDIN_FILENO); // Redirect input from pipe
            close(pipes[read_end][READ_END]);
        }
        if (pipe_out) {
            dup2(pipes[write_end][WRITE_END], STDOUT_FILENO); // Redirect output to pipe
            close(pipes[write_end][WRITE_END]);
        }
        if (redirect_in) {
            FILE* f = fopen(holder.redirect_in, "r"); // Open input redirection file
            dup2(fileno(f), STDIN_FILENO); // Redirect input
        }
        if (redirect_out) {
            FILE* f = fopen(holder.redirect_out, redirect_append ? "a" : "w"); // Open output redirection file
            dup2(fileno(f), STDOUT_FILENO); // Redirect output
        }

        child_run_command(holder.cmd); // Execute command
        exit(0); // Exit child process
    } else if (pipe_out) {
        close(pipes[write_end][WRITE_END]); // Close write end of pipe in parent
    }

    parent_run_command(holder.cmd); // Execute command in parent process
}

// Run a list of commands
void run_script(CommandHolder* holders) {
    if (!is_initialized) {
        job_list = new_job_queue(1); // Initialize job queue
        is_initialized = true; // Set initialization flag
    }
    process_id_queue = new_pid_queue(1); // Initialize process ID queue

    if (holders == NULL)
        return; // Return if no commands to run

    check_jobs_bg_status(); // Check background jobs status

    // Check if the first command is EXIT and the second is EOC
    if (get_command_holder_type(holders[0]) == EXIT &&
        get_command_holder_type(holders[1]) == EOC) {
        end_main_loop(); // Exit the main loop
        return;
    }

    CommandType type;

    // Run all commands in the `holders` array
    for (int i = 0; (type = get_command_holder_type(holders[i])) != EOC; ++i) {
        create_process(holders[i], i); // Create a new process for each command
    }

    // If the job is not a background job, wait for all child processes to finish
    if (!(holders[0].flags & BACKGROUND)) {
        while (!is_empty_pid_queue(&process_id_queue)) {
            pid_t curr_pid = pop_front_pid_queue(&process_id_queue);
            int status;
            waitpid(curr_pid, &status, 0); // Wait for child to finish
        }
        destroy_pid_queue(&process_id_queue); // Clean up PID queue
    } else { // If it's a background job
        struct Job current_job;
        current_job.job_id = job_count++;
        current_job.process_ids = process_id_queue;
        current_job.command = get_command_string();
        current_job.first_pid = peek_back_pid_queue(&process_id_queue); // First PID of the job
        push_back_job_queue(&job_list, current_job); // Add job to the job list
        print_job_bg_start(current_job.job_id, current_job.first_pid, current_job.command); // Print start message
    }
}
