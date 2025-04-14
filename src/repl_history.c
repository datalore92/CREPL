#include "../include/repl_history.h"
#include <string.h>

void repl_add_to_history(REPL* repl, const char* input) {
    // Don't add empty commands or duplicates of the most recent command
    if (!input || !input[0]) return;
    if (repl->history_count > 0 && strcmp(repl->history[0], input) == 0) return;
    
    // Shift history entries to make room for new one
    for (int i = MAX_HISTORY_ENTRIES - 1; i > 0; i--) {
        strcpy(repl->history[i], repl->history[i - 1]);
    }
    
    // Add new entry at the beginning
    strncpy(repl->history[0], input, MAX_INPUT_LENGTH - 1);
    repl->history[0][MAX_INPUT_LENGTH - 1] = '\0';
    
    if (repl->history_count < MAX_HISTORY_ENTRIES) {
        repl->history_count++;
    }
    
    // Reset history navigation index when adding new command
    repl->history_index = -1;
}

void repl_navigate_history(REPL* repl, int direction) {
    // Save current input if we're just starting navigation
    static char saved_input[MAX_INPUT_LENGTH] = {0};
    
    if (repl->history_count == 0) {
        return;  // No history to navigate
    }
    
    if (repl->history_index == -1 && direction > 0) {
        // Starting to navigate up from current input, save the current input
        strncpy(saved_input, repl->input_buffer, MAX_INPUT_LENGTH - 1);
        saved_input[MAX_INPUT_LENGTH - 1] = '\0';
    }
    
    // Calculate new history index
    int new_index = repl->history_index + direction;
    
    if (new_index >= repl->history_count) {
        new_index = repl->history_count - 1;  // Clamp to oldest entry
    } else if (new_index < -1) {
        new_index = -1;  // Clamp to current input
    }
    
    // Update input buffer with history entry or saved input
    if (new_index == -1) {
        // When returning to the current input position, restore the saved input
        strncpy(repl->input_buffer, saved_input, MAX_INPUT_LENGTH - 1);
        repl->input_buffer[MAX_INPUT_LENGTH - 1] = '\0';
    } else {
        strncpy(repl->input_buffer, repl->history[new_index], MAX_INPUT_LENGTH - 1);
        repl->input_buffer[MAX_INPUT_LENGTH - 1] = '\0';
    }
    
    // Update cursor and history index
    repl->input_cursor = strlen(repl->input_buffer);
    repl->history_index = new_index;
}