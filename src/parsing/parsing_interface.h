#ifndef SRC_PARSING_PARSING_INTERFACE_H
#define SRC_PARSING_PARSING_INTERFACE_H

#include <stdbool.h>

#include "command.h"
#include "deque.h"
#include "quash.h"


typedef struct Redirect {
  char* in;    /**< File name for redirect in. */
  char* out;   /**< File name for redirect out. */
  bool append; /**< Flag indicating that the redirect out should actually append
                * to the end of a file rather than truncating it */
} Redirect;


IMPLEMENT_DEQUE_STRUCT(CmdStrs, char*);

IMPLEMENT_DEQUE_STRUCT(Cmds, CommandHolder);

PROTOTYPE_DEQUE(CmdStrs, char*);
PROTOTYPE_DEQUE(Cmds, CommandHolder);

Redirect mk_redirect(char* in, char* out, bool append);


char* interpret_complex_string_token(const char* str);

CommandHolder* parse(QuashState* state);

void destroy_parser();

#endif
