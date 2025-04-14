#ifndef REPL_UI_H
#define REPL_UI_H

#include "repl_core.h"

// UI rendering functions
void repl_render(REPL* repl);
void repl_render_text(REPL* repl, const char* text, int x, int y, SDL_Color color);
void repl_render_styled_text(REPL* repl, const char* text, int x, int y, SDL_Color color, bool is_bold);
void repl_render_background(REPL* repl);
void repl_render_title_bar(REPL* repl);
void repl_render_input_area(REPL* repl);
void repl_render_scrollbar(REPL* repl);

#endif // REPL_UI_H