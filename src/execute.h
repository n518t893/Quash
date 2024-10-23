
#ifndef SRC_EXECUTE_H
#define SRC_EXECUTE_H

#include <stdbool.h>
#include <unistd.h>

#include "command.h"

const char* lookup_env(const char* env_var);

void write_env(const char* env_var, const char* val);

char* get_current_directory(bool* should_free);

void check_jobs_bg_status();

void print_job(int job_id, pid_t pid, const char* cmd);

void print_job_bg_start(int job_id, pid_t pid, const char* cmd);


void print_job_bg_complete(int job_id, pid_t pid, const char* cmd);


void run_generic(GenericCommand cmd);


void run_echo(EchoCommand cmd);


void run_export(ExportCommand cmd);


void run_cd(CDCommand cmd);

void run_kill(KillCommand cmd);


void run_pwd();


void run_jobs();


void run_script(CommandHolder* holders);

#endif
