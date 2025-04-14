#ifndef REPL_CORE_H
#define REPL_CORE_H

#include <SDL.h>
#include <SDL_ttf.h>
#include <stdbool.h>

/* Core definitions for the REPL */
#define MAX_INPUT_LENGTH 1024
#define MAX_OUTPUT_LENGTH 32768  // Increased buffer size for more history
#define MAX_HISTORY_ENTRIES 500  // Increased history entries
#define MAX_VARIABLES 100
#define MAX_VARIABLE_NAME 32
#define MAX_VISIBLE_LINES 100    // Maximum number of lines to show at once

// Enumeration for view modes
typedef enum {
    VIEW_MODE_SCROLL,    // Normal scrolling view
    VIEW_MODE_FIXED,     // Fixed view (output stays in place)
    VIEW_MODE_PAGED      // Paged view (like 'less' or 'more')
} ViewMode;

// Variable structure for storing variables
typedef struct {
    char name[MAX_VARIABLE_NAME];
    double value;
} Variable;

typedef struct {
    // SDL components
    SDL_Window* window;
    SDL_Renderer* renderer;
    TTF_Font* font;
    TTF_Font* bold_font;
    SDL_Texture* background_texture;
    
    // REPL state
    char input_buffer[MAX_INPUT_LENGTH];
    char output_buffer[MAX_OUTPUT_LENGTH];
    int input_cursor;
    int scroll_offset;
    int max_scroll_lines;        // Maximum lines that can be scrolled
    int visible_lines;           // Number of lines visible in the output area
    
    // Display state
    ViewMode view_mode;          // Current view mode
    bool show_scrollbar;         // Whether to show the scrollbar
    int scrollbar_dragging;      // -1 if not dragging, otherwise the y-offset when drag started
    float scroll_position;       // Position in the document (0.0 to 1.0)
    int last_output_line;        // Index of the last line added to output
    
    // History support
    char history[MAX_HISTORY_ENTRIES][MAX_INPUT_LENGTH];
    int history_count;
    int history_index; // -1 means current input, not from history
    
    // Variables support
    Variable variables[MAX_VARIABLES];
    int variable_count;
    
    // Font settings
    int font_size;
    SDL_Color text_color;
    SDL_Color bg_color;
    SDL_Color prompt_color;
    SDL_Color result_color;
    SDL_Color error_color;
    SDL_Color highlight_color;
    SDL_Color input_bg_color;
    SDL_Color output_bg_color;
    SDL_Color title_color;
    SDL_Color scrollbar_color;   // Color for the scrollbar
    SDL_Color scrollbar_bg_color; // Background color for scrollbar track
    
    // Appearance settings
    bool use_syntax_highlighting;
    int line_padding;
    int margin;
    bool show_line_numbers;
    bool show_title_bar;
    int scrollbar_width;         // Width of the scrollbar
    
    // Window dimensions
    int window_width;
    int window_height;
    
    // REPL flags
    bool running;
    bool eval_ready;
    bool auto_scroll;            // Whether to automatically scroll to bottom on new output
} REPL;

// REPL initialization and cleanup
REPL* repl_init(const char* title, int width, int height);
void repl_cleanup(REPL* repl);

// Core REPL functions
char* repl_evaluate(REPL* repl, const char* input);
void repl_print(REPL* repl, const char* result, bool is_error);
void repl_loop(REPL* repl);

// Helper functions
void repl_scroll(REPL* repl, int lines);
void repl_set_scroll_position(REPL* repl, float position);
int repl_count_output_lines(REPL* repl);
void repl_toggle_view_mode(REPL* repl);
void repl_show_help(REPL* repl);
void repl_calculate_visible_lines(REPL* repl);
void repl_handle_resize(REPL* repl, int width, int height);

#endif // REPL_CORE_H