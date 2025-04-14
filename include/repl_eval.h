#ifndef REPL_EVAL_H
#define REPL_EVAL_H

#include "repl_core.h"

// Expression evaluation functions
bool is_command(const char* input);
bool handle_command(REPL* repl, const char* input);
double evaluate_expression(REPL* repl, const char* expr, bool* error);

#endif // REPL_EVAL_H