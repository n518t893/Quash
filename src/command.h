#ifndef SRC_COMMAND_H
#define SRC_COMMAND_H

#include <stdbool.h>

#define REDIRECT_IN     (0x01)
#define REDIRECT_OUT    (0x04)
#define REDIRECT_APPEND (0x08)
#define PIPE_IN         (0x10)
#define PIPE_OUT        (0x20)
#define BACKGROUND      (0x40)


typedef enum CommandType {
  EOC = 0, 
  GENERIC,
  ECHO,
  EXPORT,
  KILL,
  CD,
  PWD,
  JOBS,
  EXIT
} CommandType;


typedef struct SimpleCommand {
  CommandType type; 
} SimpleCommand;

typedef struct GenericCommand {
  CommandType type; 
  char** args;      
} GenericCommand;

typedef GenericCommand EchoCommand;


typedef struct ExportCommand {
  CommandType type; 
  char* env_var;    
  char* val;        
} ExportCommand;

typedef struct CDCommand {
  CommandType type; 
  char* dir;        
} CDCommand;

typedef struct KillCommand {
  CommandType type; 
  int sig;          
  int job;          
  char* sig_str;    
  char* job_str;    
} KillCommand;

typedef SimpleCommand PWDCommand;

typedef SimpleCommand JobsCommand;

typedef SimpleCommand ExitCommand;

typedef SimpleCommand EOCCommand;

typedef union Command {
  SimpleCommand simple;   
  GenericCommand generic; 
  EchoCommand echo;       
  ExportCommand export;   
  CDCommand cd;           
  KillCommand kill;       
  PWDCommand pwd;         
  JobsCommand jobs;       
  ExitCommand exit;       
  EOCCommand eoc;         
} Command;

typedef struct CommandHolder {
  char* redirect_in;  
  char* redirect_out; 
  char flags;         
  Command cmd;        
} CommandHolder;

CommandHolder mk_command_holder(char* redirect_in, char* redirect_out, char flags, Command cmd);

Command mk_generic_command(char** args);

Command mk_echo_command(char** args);

Command mk_export_command(char* env_var, char* val);

Command mk_cd_command(char* dir);

Command mk_kill_command(char* sig, char* job);

Command mk_pwd_command();

Command mk_jobs_command();

Command mk_exit_command();

Command mk_eoc();

CommandType get_command_type(Command cmd);


CommandType get_command_holder_type(CommandHolder holder);

void debug_print_script(const CommandHolder* holders);

#endif
