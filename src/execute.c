/*
 * @file execute.c
 * @author julia drahozal 2441134
 *
 * @brief Implements interface functions between Quash and the environment and
 * functions that interpret an execute commands.
 *
 * @note As you add things to this file you may want to change the method signature
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

#define READ 0
#define WRITE 1

// pid queue
IMPLEMENT_DEQUE_STRUCT (pid_queue, pid_t);
IMPLEMENT_DEQUE (pid_queue, pid_t);
pid_queue pq;

// job struct
struct Job {
  int job_id;
  char* cmd;
  pid_queue pq;
  pid_t pid;
}Job;

// job queue
IMPLEMENT_DEQUE_STRUCT (job_queue, struct Job);
IMPLEMENT_DEQUE (job_queue, struct Job);
job_queue jq;
int job_num = 1;

bool init = 0;
static int pipes[2][2];

/***************************************************************************
 * Interface Functions
 ***************************************************************************/

// Return a string containing the current working directory.
char* get_current_directory(bool* should_free) {
	char* wd = NULL;
	wd = getcwd (NULL, 0);
	return wd;
}

// Returns the value of an environment variable env_var
const char* lookup_env(const char* env_var) {
  return getenv (env_var);
}

// Check the status of background jobs
void check_jobs_bg_status() {

	// loop through jobs
  int num_jobs = length_job_queue (&jq);
  for (int j = 0; j < num_jobs; j++) {

    struct Job curr_job = pop_front_job_queue (&jq);

		// loop through pids
		int num_pids = length_pid_queue (&curr_job.pq);
		pid_t front = peek_front_pid_queue (&curr_job.pq);
    for (int p = 0; p < num_pids; p++) {

      pid_t curr_pid = pop_front_pid_queue (&curr_job.pq);
			int status;
      if (waitpid (curr_pid, &status, WNOHANG) == 0) {
        push_back_pid_queue (&curr_job.pq, curr_pid);
      }
    }

    if (is_empty_pid_queue (&curr_job.pq)) {
      print_job_bg_complete (curr_job.job_id, front, curr_job.cmd);
    }
    else {
      push_back_job_queue (&jq, curr_job);
    }
  }
}

// Prints the job id number, the process id of the first process belonging to
// the Job, and the command string associated with this job
void print_job(int job_id, pid_t pid, const char* cmd) {
  printf("[%d]\t%8d\t%s\n", job_id, pid, cmd);
  fflush(stdout);
}

// Prints a start up message for background processes
void print_job_bg_start(int job_id, pid_t pid, const char* cmd) {
  printf("Background job started: ");
  print_job(job_id, pid, cmd);
}

// Prints a completion message followed by the print job
void print_job_bg_complete(int job_id, pid_t pid, const char* cmd) {
  printf("Completed: \t");
  print_job(job_id, pid, cmd);
}

/***************************************************************************
 * Functions to process commands
 ***************************************************************************/

// Run a program reachable by the path environment variable, relative path, or
// absolute path
void run_generic(GenericCommand cmd) {
    char* exec = cmd.args[0];
    char** args = cmd.args;

    // Check if the command is an absolute or relative path
    if (exec[0] == '/' || exec[0] == '.') {
        execvp(exec, args);
    } else {
        // Search for the command in the PATH
        char* path = getenv("PATH");
        char* path_copy = strdup(path);
        char* token = strtok(path_copy, ":");

        while (token != NULL) {
            char full_path[PATH_MAX];
            snprintf(full_path, sizeof(full_path), "%s/%s", token, exec);

            if (access(full_path, X_OK) == 0) {
                execvp(full_path, args);
                break;
            }

            token = strtok(NULL, ":");
        }

        free(path_copy);
    }

    perror("ERROR: Failed to execute program");
    exit(EXIT_FAILURE);
}

// Print strings
void run_echo(EchoCommand cmd) {
  char** str = cmd.args;
  for (; *str != NULL; ++str){
     printf ("%s ", *str);
  }
  printf ("\n");

	// Flush the buffer before returning
  fflush(stdout);
}

// Sets an environment variable
void run_export(ExportCommand cmd) {
	const char* env_var = cmd.env_var;
  const char* val = cmd.val;
  setenv (env_var, val, 1);
}

// Changes the current working directory
void run_cd(CDCommand cmd) {
  const char* dir = cmd.dir;

  // Check if the directory is valid
  if (dir == NULL) {
    perror("ERROR: Failed to resolve path");
    return;
  }

  char path[102];
  realpath (dir, path);
  chdir (path);
  char cwd[1024];
  setenv ("OLD_PWD", getcwd (cwd, sizeof (cwd)), 1);
  setenv ("PWD", dir, 1);
}

// Sends a signal to all processes contained in a job
void run_kill(KillCommand cmd) {
  int signal = cmd.sig;
  int job_id = cmd.job;

  struct Job curr_job;

	// find job
  for (int j = 0; j < length_job_queue (&jq); j++) {

    curr_job = pop_front_job_queue (&jq);
    if (curr_job.job_id == job_id) {

			// kill all processes
			pid_queue curr_pq = curr_job.pq;
			while (length_pid_queue (&curr_pq) != 0) {
		    pid_t curr_pid = pop_front_pid_queue (&curr_pq);
		    kill (curr_pid, signal);
		  }
    }
    push_back_job_queue (&jq, curr_job);
  }
}

// Prints the current working directory to stdout
void run_pwd() {
	char cwd[1024];
  printf ("%s\n", getcwd (cwd, sizeof (cwd)));

  // Flush the buffer before returning
  fflush(stdout);
}

// Prints all background jobs currently in the job list to stdout
void run_jobs() {

	int num_jobs = length_job_queue (&jq);
	for (int j = 0; j < num_jobs; j++)
	{
		struct Job curr_job = pop_front_job_queue (&jq);
		print_job (curr_job.job_id, curr_job.pid, curr_job.cmd);
		push_back_job_queue (&jq, curr_job);
	}

  // Flush the buffer before returning
  fflush(stdout);
}

/***************************************************************************
 * Functions for command resolution and process setup
 ***************************************************************************/

/**
 * @brief A dispatch function to resolve the correct @a Command variant
 * function for child processes.
 *
 * This version of the function is tailored to commands that should be run in
 * the child process of a fork.
 *
 * @param cmd The Command to try to run
 *
 * @sa Command
 */
void child_run_command(Command cmd) {
 CommandType type = get_command_type(cmd);

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


/**
 * @brief A dispatch function to resolve the correct @a Command variant
 * function for the quash process.
 *
 * This version of the function is tailored to commands that should be run in
 * the parent process (quash).
 *
 * @param cmd The Command to try to run
 *
 * @sa Command
 */
void parent_run_command(Command cmd) {
  CommandType type = get_command_type(cmd);

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

/**
 * @brief Creates one new process centered around the @a Command in the @a
 * CommandHolder setting up redirects and pipes where needed
 *
 * @note Processes are not the same as jobs. A single job can have multiple
 * processes running under it. This function creates a process that is part of a
 * larger job.
 *
 * @note Not all commands should be run in the child process. A few need to
 * change the quash process in some way
 *
 * @param holder The CommandHolder to try to run
 *
 * @sa Command CommandHolder
 */
void create_process(CommandHolder holder, int i) {
	// Read the flags field from the parser
  bool p_in  = holder.flags & PIPE_IN;
  bool p_out = holder.flags & PIPE_OUT;
  bool r_in  = holder.flags & REDIRECT_IN;
  bool r_out = holder.flags & REDIRECT_OUT;
  bool r_app = holder.flags & REDIRECT_APPEND;

	int write_end = i % 2;
	int read_end = (i - 1) % 2;

  if (p_out){
		pipe (pipes[write_end]);
	}
  pid_t pid = fork();

  push_back_pid_queue(&pq, pid);
  if (pid == 0) {

		if (p_in){
			dup2 (pipes[read_end][READ], STDIN_FILENO);
			close (pipes[read_end][READ]);
		}
		if (p_out){
			dup2 (pipes[write_end][WRITE], STDOUT_FILENO);
			close (pipes[write_end][WRITE]);
		}
	  if (r_in){
		  FILE* f = fopen (holder.redirect_in, "r");
		  dup2 (fileno (f), STDIN_FILENO);
	  }
	  if (r_out){
		  if (r_app){
			  FILE* f = fopen (holder.redirect_out, "a");
			  dup2 (fileno (f), STDOUT_FILENO);
		  } else{
			  FILE* f = fopen (holder.redirect_out, "w");
			  dup2 (fileno (f), STDOUT_FILENO);
		  }
	  }

    child_run_command (holder.cmd);
    exit(0);
  }else if (p_out) {
      close (pipes[write_end][WRITE]);
    }

    parent_run_command(holder.cmd);
}

// Run a list of commands
void run_script(CommandHolder* holders) {

  if (init == 0){
		jq = new_job_queue (1);
    init = 1;
  }
  pq = new_pid_queue(1);

  if (holders == NULL)
    return;

  check_jobs_bg_status();

  if (get_command_holder_type(holders[0]) == EXIT &&
      get_command_holder_type(holders[1]) == EOC) {
    end_main_loop();
    return;
  }

  CommandType type;

  // Run all commands in the `holder` array
  for (int i = 0; (type = get_command_holder_type(holders[i]) ) != EOC; ++i){
    create_process(holders[i], i );
  }

	// not a background job
  if (!(holders[0].flags & BACKGROUND)) {

    while (!is_empty_pid_queue (&pq)) {
      pid_t curr_pid = pop_front_pid_queue (&pq);
      int status;
      waitpid (curr_pid, &status, 0);
    }
    destroy_pid_queue (&pq);
  }

	// background job
  else {

    struct Job curr_job;
    curr_job.job_id = job_num;
    job_num = job_num + 1;
    curr_job.pq = pq;
    curr_job.cmd = get_command_string ();
    curr_job.pid = peek_back_pid_queue (&pq);
    push_back_job_queue (&jq, curr_job);
    print_job_bg_start (curr_job.job_id, curr_job.pid, curr_job.cmd);
  }
}
