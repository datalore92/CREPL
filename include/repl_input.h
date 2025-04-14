#ifndef REPL_INPUT_H
#define REPL_INPUT_H

#include "repl_core.h"

// Input handling functions
void repl_read(REPL* repl, SDL_Event* event);
void repl_clear_input(REPL* repl);

#endif // REPL_INPUT_H