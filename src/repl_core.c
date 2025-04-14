#include "../include/repl_core.h"
#include "../include/repl_input.h"
#include "../include/repl_ui.h"
#include "../include/repl_variables.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

REPL* repl_init(const char* title, int width, int height) {
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        fprintf(stderr, "SDL could not initialize! SDL_Error: %s\n", SDL_GetError());
        return NULL;
    }
    
    // Initialize SDL_ttf
    if (TTF_Init() < 0) {
        fprintf(stderr, "SDL_ttf could not initialize! TTF_Error: %s\n", TTF_GetError());
        SDL_Quit();
        return NULL;
    }
    
    REPL* repl = (REPL*)malloc(sizeof(REPL));
    if (!repl) return NULL;
    
    // Initialize SDL components
    repl->window = SDL_CreateWindow(title, 
                                   SDL_WINDOWPOS_UNDEFINED, 
                                   SDL_WINDOWPOS_UNDEFINED, 
                                   width, height, 
                                   SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);
    
    if (!repl->window) {
        fprintf(stderr, "Window could not be created! SDL_Error: %s\n", SDL_GetError());
        free(repl);
        TTF_Quit();
        SDL_Quit();
        return NULL;
    }
    
    // Set window icon (using SDL logo as a placeholder)
    // You would replace this with your own icon
    SDL_Surface* icon = SDL_LoadBMP("C:/Windows/Media/favicon.bmp");
    if (icon) {
        SDL_SetWindowIcon(repl->window, icon);
        SDL_FreeSurface(icon);
    }
    
    repl->renderer = SDL_CreateRenderer(repl->window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!repl->renderer) {
        fprintf(stderr, "Renderer could not be created! SDL_Error: %s\n", SDL_GetError());
        SDL_DestroyWindow(repl->window);
        free(repl);
        TTF_Quit();
        SDL_Quit();
        return NULL;
    }
    
    // Enable alpha blending for transparency
    SDL_SetRenderDrawBlendMode(repl->renderer, SDL_BLENDMODE_BLEND);
    
    // Initialize fonts
    repl->font_size = 16;
    repl->font = TTF_OpenFont("C:/Windows/Fonts/consola.ttf", repl->font_size);
    if (!repl->font) {
        // Try another common system font if Consolas isn't available
        repl->font = TTF_OpenFont("C:/Windows/Fonts/cour.ttf", repl->font_size);
    }
    
    if (!repl->font) {
        fprintf(stderr, "Failed to load font! TTF_Error: %s\n", TTF_GetError());
        SDL_DestroyRenderer(repl->renderer);
        SDL_DestroyWindow(repl->window);
        free(repl);
        TTF_Quit();
        SDL_Quit();
        return NULL;
    }
    
    // Initialize bold font for highlighting
    repl->bold_font = TTF_OpenFont("C:/Windows/Fonts/consolab.ttf", repl->font_size);
    if (!repl->bold_font) {
        // Try another common system bold font
        repl->bold_font = TTF_OpenFont("C:/Windows/Fonts/courbd.ttf", repl->font_size);
    }
    
    if (!repl->bold_font) {
        // If no bold font available, use the regular font
        repl->bold_font = repl->font;
    }
    
    // Initialize REPL state
    memset(repl->input_buffer, 0, sizeof(repl->input_buffer));
    memset(repl->output_buffer, 0, sizeof(repl->output_buffer));
    strcpy(repl->output_buffer, "Welcome to C REPL v2.0! Type 'help' for available commands.\n> ");
    
    repl->input_cursor = 0;
    repl->scroll_offset = 0;
    repl->max_scroll_lines = 0;
    repl->visible_lines = 0;
    
    // Initialize scrolling and display state
    repl->view_mode = VIEW_MODE_SCROLL;
    repl->show_scrollbar = true;
    repl->scrollbar_dragging = -1;
    repl->scroll_position = 0.0f;
    repl->last_output_line = 0;
    repl->auto_scroll = true;
    
    // Initialize history
    memset(repl->history, 0, sizeof(repl->history));
    repl->history_count = 0;
    repl->history_index = -1; // -1 means current input, not from history
    
    // Initialize variables
    memset(repl->variables, 0, sizeof(repl->variables));
    repl->variable_count = 0;
    
    // Set up some default variables
    repl_set_variable(repl, "pi", 3.14159265358979323846);
    repl_set_variable(repl, "e", 2.71828182845904523536);
    
    // Initialize colors - modern dark theme with higher contrast
    repl->bg_color = (SDL_Color){30, 30, 44, 255}; // Deep blue-gray background
    repl->text_color = (SDL_Color){220, 223, 228, 255}; // Light gray text
    repl->prompt_color = (SDL_Color){97, 175, 239, 255}; // Bright blue for prompts
    repl->result_color = (SDL_Color){152, 195, 121, 255}; // Green for results
    repl->error_color = (SDL_Color){224, 108, 117, 255}; // Soft red for errors
    repl->highlight_color = (SDL_Color){229, 192, 123, 255}; // Gold for highlights
    repl->input_bg_color = (SDL_Color){44, 44, 60, 200}; // Slightly lighter bg for input
    repl->output_bg_color = (SDL_Color){36, 36, 52, 150}; // Slightly darker bg for output
    repl->title_color = (SDL_Color){97, 175, 239, 255}; // Same as prompt color
    repl->scrollbar_color = (SDL_Color){97, 175, 239, 180}; // Semi-transparent blue
    repl->scrollbar_bg_color = (SDL_Color){50, 50, 65, 100}; // Semi-transparent dark
    
    // Initialize appearance settings
    repl->use_syntax_highlighting = true;
    repl->line_padding = 2;
    repl->margin = 10;
    repl->show_line_numbers = false;
    repl->show_title_bar = true;
    repl->scrollbar_width = 12;
    
    // Store window dimensions
    repl->window_width = width;
    repl->window_height = height;
    
    // Calculate number of visible lines
    repl_calculate_visible_lines(repl);
    
    // Initialize REPL flags
    repl->running = true;
    repl->eval_ready = false;
    
    // Set the background texture to NULL initially
    repl->background_texture = NULL;
    
    return repl;
}

void repl_cleanup(REPL* repl) {
    if (repl->font) TTF_CloseFont(repl->font);
    if (repl->renderer) SDL_DestroyRenderer(repl->renderer);
    if (repl->window) SDL_DestroyWindow(repl->window);
    free(repl);
    TTF_Quit();
    SDL_Quit();
}

void repl_print(REPL* repl, const char* result, bool is_error) {
    // Append input to output buffer
    strcat(repl->output_buffer, repl->input_buffer);
    strcat(repl->output_buffer, "\n");
    
    // Append result to output buffer
    if (strlen(result) > 0) {
        if (is_error) {
            strcat(repl->output_buffer, "Error: ");
        }
        strcat(repl->output_buffer, result);
        strcat(repl->output_buffer, "\n");
    }
    
    strcat(repl->output_buffer, "> ");
    
    // Reset input buffer and evaluation flag
    repl_clear_input(repl);
    repl->eval_ready = false;
    
    // Count total lines in output buffer
    int total_lines = repl_count_output_lines(repl);
    
    // Always re-enable auto-scroll when a new command is entered
    repl->auto_scroll = true;
    
    // Always scroll to show the most recent command
    // Calculate the optimal scroll position to show the most recent history
    int optimal_scroll = total_lines - repl->visible_lines + 1;
    if (optimal_scroll < 0) optimal_scroll = 0;
    
    // Set scroll position to show most recent commands
    repl->scroll_offset = optimal_scroll;
    
    // Update normalized scroll position for scrollbar
    if (total_lines > repl->visible_lines) {
        repl->scroll_position = (float)repl->scroll_offset / (total_lines - repl->visible_lines);
    } else {
        repl->scroll_position = 0.0f;
    }
}

// Calculate the number of visible lines that can fit in the output area
void repl_calculate_visible_lines(REPL* repl) {
    // Calculate line height
    int line_height = TTF_FontHeight(repl->font) + 2 * repl->line_padding;
    
    // Calculate available height for output area
    int title_offset = repl->show_title_bar ? 30 : 0;
    int input_area_height = line_height + 2 * repl->margin;
    int output_height = repl->window_height - title_offset - input_area_height - 2 * repl->margin;
    
    // Calculate number of visible lines
    repl->visible_lines = output_height / line_height;
    
    // Make sure it's at least 1 line
    if (repl->visible_lines < 1) repl->visible_lines = 1;
}

// Count the total number of lines in the output buffer
int repl_count_output_lines(REPL* repl) {
    int total_lines = 0;
    
    for (int i = 0; i < strlen(repl->output_buffer); i++) {
        if (repl->output_buffer[i] == '\n') {
            total_lines++;
        }
    }
    
    return total_lines;
}

void repl_scroll(REPL* repl, int lines) {
    // Count total lines in output buffer
    int total_lines = repl_count_output_lines(repl);
    
    // Update scroll offset
    repl->scroll_offset += lines;
    
    // Clamp scroll offset
    if (repl->scroll_offset < 0) {
        repl->scroll_offset = 0;
    } else if (repl->scroll_offset > total_lines) {
        repl->scroll_offset = total_lines;
    }
}

// Set the scroll position directly (0.0 = top, 1.0 = bottom)
void repl_set_scroll_position(REPL* repl, float position) {
    int total_lines = repl_count_output_lines(repl);
    int max_offset = total_lines - repl->visible_lines + 1;
    
    if (max_offset < 0) max_offset = 0;
    
    // Calculate and set the new scroll offset
    repl->scroll_offset = (int)(position * max_offset);
    
    // Clamp to valid range
    if (repl->scroll_offset < 0) {
        repl->scroll_offset = 0;
    } else if (repl->scroll_offset > max_offset) {
        repl->scroll_offset = max_offset;
    }
    
    // Update the scroll position
    if (max_offset > 0) {
        repl->scroll_position = (float)repl->scroll_offset / max_offset;
    } else {
        repl->scroll_position = 0.0f;
    }
}

// Toggle between different view modes
void repl_toggle_view_mode(REPL* repl) {
    // Cycle through view modes
    switch (repl->view_mode) {
        case VIEW_MODE_SCROLL:
            repl->view_mode = VIEW_MODE_FIXED;
            repl->auto_scroll = false;
            break;
        case VIEW_MODE_FIXED:
            repl->view_mode = VIEW_MODE_PAGED;
            repl->auto_scroll = false;
            break;
        case VIEW_MODE_PAGED:
            repl->view_mode = VIEW_MODE_SCROLL;
            repl->auto_scroll = true;
            // Scroll to bottom in scroll mode
            repl_scroll(repl, INT_MAX);
            break;
    }
}

void repl_handle_resize(REPL* repl, int width, int height) {
    // Store new dimensions
    repl->window_width = width;
    repl->window_height = height;
    
    // Recalculate visible lines
    repl_calculate_visible_lines(repl);
    
    // If auto-scroll is enabled, adjust scroll to show most recent content
    if (repl->auto_scroll) {
        int total_lines = repl_count_output_lines(repl);
        int max_offset = total_lines - repl->visible_lines + 1;
        if (max_offset < 0) max_offset = 0;
        
        // Set scroll position to show most recent history
        repl->scroll_offset = max_offset;
        
        // Update normalized scroll position for scrollbar
        if (total_lines > repl->visible_lines) {
            repl->scroll_position = (float)repl->scroll_offset / (total_lines - repl->visible_lines);
        } else {
            repl->scroll_position = 0.0f;
        }
    }
}

void repl_show_help(REPL* repl) {
    static char help_text[] = 
        "C REPL v2.0 - Help\n"
        "Commands:\n"
        "  help      - Show this help message\n"
        "  clear     - Clear the console\n"
        "  vars      - Display all defined variables\n"
        "  version   - Display version information\n"
        "  exit/quit - Exit the REPL\n"
        "\n"
        "Expressions:\n"
        "  Arithmetic: 5 + 3, 10 * (3 + 2), etc.\n"
        "  Variables: x = 5, pi, e (predefined)\n"
        "\n"
        "Keyboard Shortcuts:\n"
        "  Up/Down        - Navigate command history\n"
        "  Left/Right     - Move cursor\n"
        "  Home/End       - Jump to start/end of line\n"
        "  Alt+Home/End   - Jump to top/bottom of output\n"
        "  PageUp/PageDown - Scroll output by pages\n"
        "  Alt+V          - Toggle view mode (scroll/fixed/paged)\n"
        "  Alt+S          - Toggle auto-scroll\n"
        "  Escape         - Clear current input\n"
        "\n"
        "Mouse Controls:\n"
        "  Mouse wheel    - Scroll output\n"
        "  Click & drag scrollbar - Navigate history\n";
    
    repl_print(repl, help_text, false);
}

void repl_loop(REPL* repl) {
    SDL_Event event;
    
    while (repl->running) {
        // Handle events
        while (SDL_PollEvent(&event) != 0) {
            if (event.type == SDL_QUIT) {
                repl->running = false;
            } else {
                repl_read(repl, &event);
            }
        }
        
        // Evaluate input if ready
        if (repl->eval_ready) {
            char* result = repl_evaluate(repl, repl->input_buffer);
            
            // Check if it's an error message
            bool is_error = (strncmp(result, "Error", 5) == 0);
            
            // Print result
            repl_print(repl, result, is_error);
        }
        
        // Render REPL
        repl_render(repl);
        
        // Small delay to prevent CPU hogging
        SDL_Delay(10);
    }
}