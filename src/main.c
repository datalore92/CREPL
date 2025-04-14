#include "../include/repl_core.h"
#include <stdio.h>

int main(int argc, char* argv[]) {
    // Initialize REPL with window title and size
    REPL* repl = repl_init("C REPL v2.0 - An Enhanced REPL", 800, 600);
    
    if (!repl) {
        fprintf(stderr, "Failed to initialize REPL\n");
        return 1;
    }
    
    // Run the REPL loop
    repl_loop(repl);
    
    // Clean up resources
    repl_cleanup(repl);
    
    return 0;
}