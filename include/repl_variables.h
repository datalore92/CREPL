#ifndef REPL_VARIABLES_H
#define REPL_VARIABLES_H

#include "repl_core.h"

// Variable management functions
void repl_set_variable(REPL* repl, const char* name, double value);
double repl_get_variable(REPL* repl, const char* name, bool* found);
bool repl_is_variable(REPL* repl, const char* name);
void repl_list_variables(REPL* repl, char* buffer, size_t buffer_size);

#endif // REPL_VARIABLES_H