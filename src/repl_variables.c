#include "../include/repl_variables.h"
#include <string.h>
#include <stdio.h>

void repl_set_variable(REPL* repl, const char* name, double value) {
    // Check if variable already exists
    for (int i = 0; i < repl->variable_count; i++) {
        if (strcmp(repl->variables[i].name, name) == 0) {
            repl->variables[i].value = value;
            return;
        }
    }
    
    // Add new variable if space available
    if (repl->variable_count < MAX_VARIABLES) {
        strncpy(repl->variables[repl->variable_count].name, name, MAX_VARIABLE_NAME - 1);
        repl->variables[repl->variable_count].name[MAX_VARIABLE_NAME - 1] = '\0';
        repl->variables[repl->variable_count].value = value;
        repl->variable_count++;
    }
}

double repl_get_variable(REPL* repl, const char* name, bool* found) {
    for (int i = 0; i < repl->variable_count; i++) {
        if (strcmp(repl->variables[i].name, name) == 0) {
            if (found) *found = true;
            return repl->variables[i].value;
        }
    }
    
    if (found) *found = false;
    return 0.0;
}

bool repl_is_variable(REPL* repl, const char* name) {
    for (int i = 0; i < repl->variable_count; i++) {
        if (strcmp(repl->variables[i].name, name) == 0) {
            return true;
        }
    }
    return false;
}

void repl_list_variables(REPL* repl, char* buffer, size_t buffer_size) {
    if (!buffer || buffer_size == 0) return;
    
    // Initialize buffer with header
    snprintf(buffer, buffer_size, "Variables:\n");
    size_t offset = strlen(buffer);
    
    // Add each variable to the buffer
    for (int i = 0; i < repl->variable_count; i++) {
        // Check if we have enough room left in the buffer
        size_t remaining = buffer_size - offset;
        if (remaining < 50) break; // Ensure enough space for one more entry plus truncation message
        
        // Format this variable and add to buffer
        int written = snprintf(buffer + offset, remaining, "  %s = %.6g\n", 
                             repl->variables[i].name, repl->variables[i].value);
        
        if (written < 0 || (size_t)written >= remaining) {
            // Buffer is full, add truncation message
            snprintf(buffer + offset, remaining, "...(truncated)");
            break;
        }
        
        offset += written;
    }
    
    // Ensure null termination
    buffer[buffer_size - 1] = '\0';
}

void repl_clear_variables(REPL* repl) {
    // Reset variable count
    repl->variable_count = 0;
    
    // Set up default variables again
    repl_set_variable(repl, "pi", 3.14159265358979323846);
    repl_set_variable(repl, "e", 2.71828182845904523536);
}