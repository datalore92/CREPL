#ifndef REPL_HISTORY_H
#define REPL_HISTORY_H

#include "repl_core.h"

// History management functions
void repl_add_to_history(REPL* repl, const char* input);
void repl_navigate_history(REPL* repl, int direction);

#endif // REPL_HISTORY_H