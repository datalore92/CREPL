#include "../include/repl.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <ctype.h>

// Define tokenization helpers
#define TOKEN_NUMBER 0
#define TOKEN_OPERATOR 1
#define TOKEN_VARIABLE 2
#define TOKEN_COMMAND 3
#define TOKEN_ERROR 4

typedef struct {
    int type;
    union {
        double number;
        char op;
        char var_name[MAX_VARIABLE_NAME];
        char command[MAX_VARIABLE_NAME];
    } value;
} Token;

// Forward declarations of helper functions
static double evaluate_expression(REPL* repl, const char* expr, bool* error);
static void tokenize(REPL* repl, const char* expr, Token* tokens, int* token_count, bool* error);
static double parse_expression(REPL* repl, Token* tokens, int* pos, int token_count, bool* error);
static double parse_term(REPL* repl, Token* tokens, int* pos, int token_count, bool* error);
static double parse_factor(REPL* repl, Token* tokens, int* pos, int token_count, bool* error);
static bool is_command(const char* input);
static bool handle_command(REPL* repl, const char* input);

// Built-in commands
static const char* HELP_CMD = "help";
static const char* CLEAR_CMD = "clear";
static const char* EXIT_CMD = "exit";
static const char* QUIT_CMD = "quit";
static const char* VARS_CMD = "vars";
static const char* VERSION_CMD = "version";

// Enhanced evaluator function
char* repl_evaluate(REPL* repl, const char* input) {
    static char result[MAX_INPUT_LENGTH];
    
    // Trim leading/trailing whitespace
    while (isspace(*input)) input++;
    
    // Handle empty input
    if (*input == '\0') {
        strcpy(result, "");
        return result;
    }
    
    // Check if input is a command
    if (is_command(input)) {
        if (handle_command(repl, input)) {
            // Command handled successfully
            return result;
        }
    }
    
    // Check if input is an assignment (var = expression)
    char var_name[MAX_VARIABLE_NAME] = {0};
    int expr_start = 0;
    if (sscanf(input, "%31[a-zA-Z0-9_] = %n", var_name, &expr_start) == 1 && expr_start != 0) {
        // This is a variable assignment
        bool error = false;
        double value = evaluate_expression(repl, input + expr_start, &error);
        
        if (!error) {
            repl_set_variable(repl, var_name, value);
            sprintf(result, "%s = %.6g", var_name, value);
        } else {
            sprintf(result, "Error evaluating expression: %s", input + expr_start);
        }
        return result;
    }
    
    // Otherwise, evaluate as an expression
    bool error = false;
    double value = evaluate_expression(repl, input, &error);
    
    if (error) {
        sprintf(result, "Error evaluating: %s", input);
    } else {
        // Check if result is close to an integer
        if (fabs(value - round(value)) < 1e-10) {
            sprintf(result, "%.0f", value);
        } else {
            sprintf(result, "%.6g", value);
        }
    }
    
    return result;
}

// Expression evaluation functions
static double evaluate_expression(REPL* repl, const char* expr, bool* error) {
    Token tokens[100]; // Assume max 100 tokens
    int token_count = 0;
    
    tokenize(repl, expr, tokens, &token_count, error);
    if (*error) return 0.0;
    
    int pos = 0;
    double result = parse_expression(repl, tokens, &pos, token_count, error);
    
    // Make sure all tokens were consumed
    if (pos != token_count && !*error) {
        *error = true;
    }
    
    return result;
}

static void tokenize(REPL* repl, const char* expr, Token* tokens, int* token_count, bool* error) {
    *token_count = 0;
    *error = false;
    
    while (*expr) {
        // Skip whitespace
        if (isspace(*expr)) {
            expr++;
            continue;
        }
        
        // Check for numbers
        if (isdigit(*expr) || *expr == '.') {
            char* end;
            double val = strtod(expr, &end);
            tokens[*token_count].type = TOKEN_NUMBER;
            tokens[*token_count].value.number = val;
            (*token_count)++;
            expr = end;
            continue;
        }
        
        // Check for operators
        if (*expr == '+' || *expr == '-' || *expr == '*' || *expr == '/' || 
            *expr == '^' || *expr == '(' || *expr == ')') {
            tokens[*token_count].type = TOKEN_OPERATOR;
            tokens[*token_count].value.op = *expr;
            (*token_count)++;
            expr++;
            continue;
        }
        
        // Check for variables/functions
        if (isalpha(*expr) || *expr == '_') {
            int i = 0;
            char name[MAX_VARIABLE_NAME] = {0};
            
            while ((isalnum(*expr) || *expr == '_') && i < MAX_VARIABLE_NAME - 1) {
                name[i++] = *expr++;
            }
            name[i] = '\0';
            
            // Check if it's a variable
            if (repl_is_variable(repl, name)) {
                tokens[*token_count].type = TOKEN_VARIABLE;
                strcpy(tokens[*token_count].value.var_name, name);
            } else {
                tokens[*token_count].type = TOKEN_ERROR;
                *error = true;
                sprintf(tokens[*token_count].value.var_name, "Unknown variable: %s", name);
            }
            
            (*token_count)++;
            continue;
        }
        
        // Unknown token
        *error = true;
        return;
    }
}

static double parse_expression(REPL* repl, Token* tokens, int* pos, int token_count, bool* error) {
    double left = parse_term(repl, tokens, pos, token_count, error);
    if (*error) return 0.0;
    
    while (*pos < token_count) {
        if (tokens[*pos].type != TOKEN_OPERATOR) break;
        
        char op = tokens[*pos].value.op;
        if (op != '+' && op != '-') break;
        
        (*pos)++;
        double right = parse_term(repl, tokens, pos, token_count, error);
        if (*error) return 0.0;
        
        if (op == '+') left += right;
        else left -= right;
    }
    
    return left;
}

static double parse_term(REPL* repl, Token* tokens, int* pos, int token_count, bool* error) {
    double left = parse_factor(repl, tokens, pos, token_count, error);
    if (*error) return 0.0;
    
    while (*pos < token_count) {
        if (tokens[*pos].type != TOKEN_OPERATOR) break;
        
        char op = tokens[*pos].value.op;
        if (op != '*' && op != '/') break;
        
        (*pos)++;
        double right = parse_factor(repl, tokens, pos, token_count, error);
        if (*error) return 0.0;
        
        if (op == '*') left *= right;
        else {
            if (right == 0.0) {
                *error = true;
                return 0.0;
            }
            left /= right;
        }
    }
    
    return left;
}

static double parse_factor(REPL* repl, Token* tokens, int* pos, int token_count, bool* error) {
    if (*pos >= token_count) {
        *error = true;
        return 0.0;
    }
    
    if (tokens[*pos].type == TOKEN_NUMBER) {
        return tokens[(*pos)++].value.number;
    }
    
    if (tokens[*pos].type == TOKEN_VARIABLE) {
        bool found;
        double value = repl_get_variable(repl, tokens[*pos].value.var_name, &found);
        if (!found) {
            *error = true;
            return 0.0;
        }
        (*pos)++;
        return value;
    }
    
    if (tokens[*pos].type == TOKEN_OPERATOR && tokens[*pos].value.op == '(') {
        (*pos)++; // Skip opening parenthesis
        double value = parse_expression(repl, tokens, pos, token_count, error);
        if (*error) return 0.0;
        
        if (*pos >= token_count || tokens[*pos].type != TOKEN_OPERATOR || tokens[*pos].value.op != ')') {
            *error = true;
            return 0.0;
        }
        
        (*pos)++; // Skip closing parenthesis
        return value;
    }
    
    // Unary operators
    if (tokens[*pos].type == TOKEN_OPERATOR) {
        if (tokens[*pos].value.op == '+') {
            (*pos)++;
            return parse_factor(repl, tokens, pos, token_count, error);
        }
        if (tokens[*pos].value.op == '-') {
            (*pos)++;
            return -parse_factor(repl, tokens, pos, token_count, error);
        }
    }
    
    *error = true;
    return 0.0;
}

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

void repl_read(REPL* repl, SDL_Event* event) {
    static bool alt_pressed = false;
    
    switch (event->type) {
        case SDL_KEYDOWN:
            // Handle Alt key for special commands
            if (event->key.keysym.sym == SDLK_LALT || event->key.keysym.sym == SDLK_RALT) {
                alt_pressed = true;
            }
            // Alt+V toggles view mode
            else if (alt_pressed && event->key.keysym.sym == SDLK_v) {
                repl_toggle_view_mode(repl);
            }
            // Alt+S toggles auto-scroll
            else if (alt_pressed && event->key.keysym.sym == SDLK_s) {
                repl->auto_scroll = !repl->auto_scroll;
                if (repl->auto_scroll) {
                    repl_scroll(repl, INT_MAX); // Scroll to bottom
                }
            }
            // Home key jumps to top of output when Alt is pressed
            else if (alt_pressed && event->key.keysym.sym == SDLK_HOME) {
                repl_set_scroll_position(repl, 0.0f); // Scroll to top
            }
            // End key jumps to bottom of output when Alt is pressed
            else if (alt_pressed && event->key.keysym.sym == SDLK_END) {
                repl_set_scroll_position(repl, 1.0f); // Scroll to bottom
            }
            else if (event->key.keysym.sym == SDLK_RETURN) {
                // Process Enter key - evaluate input
                if (repl->input_cursor > 0) {
                    repl->input_buffer[repl->input_cursor] = '\0';
                    repl->eval_ready = true;
                    
                    // Add to history if not empty
                    if (repl->input_buffer[0] != '\0') {
                        repl_add_to_history(repl, repl->input_buffer);
                    }
                    
                    repl->history_index = -1;  // Reset history navigation
                }
            } 
            else if (event->key.keysym.sym == SDLK_BACKSPACE && repl->input_cursor > 0) {
                // Process Backspace key
                repl->input_cursor--;
                repl->input_buffer[repl->input_cursor] = '\0';
            }
            else if (event->key.keysym.sym == SDLK_DELETE && repl->input_cursor < strlen(repl->input_buffer)) {
                // Delete key removes character at cursor
                memmove(repl->input_buffer + repl->input_cursor, 
                        repl->input_buffer + repl->input_cursor + 1, 
                        strlen(repl->input_buffer) - repl->input_cursor);
            }
            else if (event->key.keysym.sym == SDLK_LEFT && repl->input_cursor > 0) {
                // Move cursor left
                repl->input_cursor--;
            }
            else if (event->key.keysym.sym == SDLK_RIGHT && repl->input_cursor < strlen(repl->input_buffer)) {
                // Move cursor right
                repl->input_cursor++;
            }
            else if (event->key.keysym.sym == SDLK_HOME && !alt_pressed) {
                // Move cursor to start of line
                repl->input_cursor = 0;
            }
            else if (event->key.keysym.sym == SDLK_END && !alt_pressed) {
                // Move cursor to end of line
                repl->input_cursor = strlen(repl->input_buffer);
            }
            else if (event->key.keysym.sym == SDLK_UP) {
                // Navigate history up
                repl_navigate_history(repl, 1);
            }
            else if (event->key.keysym.sym == SDLK_DOWN) {
                // Navigate history down
                repl_navigate_history(repl, -1);
            }
            else if (event->key.keysym.sym == SDLK_PAGEUP) {
                // Scroll up - more lines at once
                repl_scroll(repl, -10);
            }
            else if (event->key.keysym.sym == SDLK_PAGEDOWN) {
                // Scroll down - more lines at once
                repl_scroll(repl, 10);
            }
            else if (event->key.keysym.sym == SDLK_ESCAPE) {
                // Clear current input
                repl_clear_input(repl);
            }
            else if ((event->key.keysym.sym >= SDLK_SPACE && event->key.keysym.sym <= SDLK_z) ||
                     event->key.keysym.sym == SDLK_SPACE ||
                     event->key.keysym.sym == SDLK_PERIOD ||
                     event->key.keysym.sym == SDLK_COMMA ||
                     event->key.keysym.sym == SDLK_SLASH ||
                     event->key.keysym.sym == SDLK_BACKSLASH ||
                     event->key.keysym.sym == SDLK_MINUS ||
                     event->key.keysym.sym == SDLK_EQUALS ||
                     event->key.keysym.sym == SDLK_LEFTBRACKET ||
                     event->key.keysym.sym == SDLK_RIGHTBRACKET ||
                     event->key.keysym.sym == SDLK_SEMICOLON ||
                     event->key.keysym.sym == SDLK_QUOTE ||
                     event->key.keysym.sym == SDLK_TAB) {
                
                char input_char;
                
                // Map tab to space
                if (event->key.keysym.sym == SDLK_TAB) {
                    input_char = ' ';
                } else {
                    // Get character from keyboard input
                    input_char = (char)event->key.keysym.sym;
                }
                
                // Handle shift key for special characters
                SDL_Keymod mod = SDL_GetModState();
                if (mod & KMOD_SHIFT) {
                    switch (input_char) {
                        case '1': input_char = '!'; break;
                        case '2': input_char = '@'; break;
                        case '3': input_char = '#'; break;
                        case '4': input_char = '$'; break;
                        case '5': input_char = '%'; break;
                        case '6': input_char = '^'; break;
                        case '7': input_char = '&'; break;
                        case '8': input_char = '*'; break;
                        case '9': input_char = '('; break;
                        case '0': input_char = ')'; break;
                        case '-': input_char = '_'; break;
                        case '=': input_char = '+'; break;
                        case '[': input_char = '{'; break;
                        case ']': input_char = '}'; break;
                        case ';': input_char = ':'; break;
                        case '\'': input_char = '"'; break;
                        case ',': input_char = '<'; break;
                        case '.': input_char = '>'; break;
                        case '/': input_char = '?'; break;
                        case '\\': input_char = '|'; break;
                        default:
                            // Convert lowercase to uppercase if shift is pressed
                            if (input_char >= 'a' && input_char <= 'z') input_char = input_char - 32;
                            break;
                    }
                }
                
                if (repl->input_cursor < MAX_INPUT_LENGTH - 1) {
                    // Insert character at cursor position
                    if (repl->input_cursor < strlen(repl->input_buffer)) {
                        // Make room for insertion
                        memmove(repl->input_buffer + repl->input_cursor + 1, 
                                repl->input_buffer + repl->input_cursor, 
                                strlen(repl->input_buffer) - repl->input_cursor + 1);
                    }
                    repl->input_buffer[repl->input_cursor] = input_char;
                    repl->input_cursor++;
                }
            }
            break;
            
        case SDL_KEYUP:
            if (event->key.keysym.sym == SDLK_LALT || event->key.keysym.sym == SDLK_RALT) {
                alt_pressed = false;
            }
            break;
        
        case SDL_MOUSEWHEEL:
            if (event->wheel.y != 0) {
                int total_lines = repl_count_output_lines(repl);
                
                // Calculate how much to scroll based on total lines
                float scroll_amount = 0.05f;  // Percentage of total scrollable content to scroll
                float scroll_delta;
                
                // Invert direction so scrolling down moves content up
                if (event->wheel.y > 0) {
                    // Scroll up (negative delta moves view toward top of content)
                    scroll_delta = -scroll_amount;
                } else {
                    // Scroll down (positive delta moves view toward bottom of content)
                    scroll_delta = scroll_amount;
                }
                
                // Get the new position by adding the delta to the current position
                float new_position = repl->scroll_position + scroll_delta;
                
                // Clamp the new position
                if (new_position < 0.0f) new_position = 0.0f;
                if (new_position > 1.0f) new_position = 1.0f;
                
                // Set the new scroll position - this updates both the position and offset
                repl_set_scroll_position(repl, new_position);
                
                // Disable auto-scroll when manually scrolling
                repl->auto_scroll = false;
            }
            break;
            
        case SDL_MOUSEBUTTONDOWN:
            if (event->button.button == SDL_BUTTON_LEFT) {
                // Check if click is on scrollbar
                int title_offset = repl->show_title_bar ? 30 : 0;
                int line_height = TTF_FontHeight(repl->font) + 2 * repl->line_padding;
                int input_area_height = line_height + 2 * repl->line_padding;
                int scrollbar_height = repl->window_height - title_offset - input_area_height - 4 * repl->margin;
                int scrollbar_x = repl->window_width - repl->scrollbar_width - repl->margin;
                int scrollbar_y = title_offset + 2 * repl->margin;
                
                // Calculate the thumb size and position
                int total_lines = repl_count_output_lines(repl);
                float visible_portion = (float)repl->visible_lines / (total_lines > 0 ? total_lines : 1);
                if (visible_portion > 1.0f) visible_portion = 1.0f;
                
                int thumb_height = (int)(scrollbar_height * visible_portion);
                if (thumb_height < 20) thumb_height = 20; // Minimum thumb size
                
                int thumb_y = scrollbar_y + (int)(repl->scroll_position * (scrollbar_height - thumb_height));
                
                // Check if click is on the scrollbar track
                if (event->button.x >= scrollbar_x && 
                    event->button.x <= scrollbar_x + repl->scrollbar_width &&
                    event->button.y >= scrollbar_y && 
                    event->button.y <= scrollbar_y + scrollbar_height) {
                    
                    if (event->button.y >= thumb_y && event->button.y <= thumb_y + thumb_height) {
                        // Click is on the thumb - start dragging
                        repl->scrollbar_dragging = event->button.y - thumb_y;
                    } else {
                        // Click is on the track - jump to that position
                        float relative_pos = (float)(event->button.y - scrollbar_y) / scrollbar_height;
                        repl_set_scroll_position(repl, relative_pos);
                    }
                    
                    // Disable auto-scroll when manually scrolling
                    repl->auto_scroll = false;
                }
            }
            break;
            
        case SDL_MOUSEBUTTONUP:
            if (event->button.button == SDL_BUTTON_LEFT && repl->scrollbar_dragging >= 0) {
                // Stop scrollbar dragging
                repl->scrollbar_dragging = -1;
            }
            break;
            
        case SDL_MOUSEMOTION:
            if (repl->scrollbar_dragging >= 0) {
                // Continue scrollbar dragging
                int title_offset = repl->show_title_bar ? 30 : 0;
                int line_height = TTF_FontHeight(repl->font) + 2 * repl->line_padding;
                int input_area_height = line_height + 2 * repl->line_padding;
                int scrollbar_height = repl->window_height - title_offset - input_area_height - 4 * repl->margin;
                int scrollbar_y = title_offset + 2 * repl->margin;
                
                // Calculate thumb size again
                int total_lines = repl_count_output_lines(repl);
                float visible_portion = (float)repl->visible_lines / (total_lines > 0 ? total_lines : 1);
                if (visible_portion > 1.0f) visible_portion = 1.0f;
                
                int thumb_height = (int)(scrollbar_height * visible_portion);
                if (thumb_height < 20) thumb_height = 20;
                
                // Calculate new position based on mouse position
                float relative_pos = (float)(event->motion.y - repl->scrollbar_dragging - scrollbar_y) / (scrollbar_height - thumb_height);
                
                // Clamp position between 0 and 1
                if (relative_pos < 0.0f) relative_pos = 0.0f;
                if (relative_pos > 1.0f) relative_pos = 1.0f;
                
                repl_set_scroll_position(repl, relative_pos);
            }
            break;
            
        case SDL_WINDOWEVENT:
            if (event->window.event == SDL_WINDOWEVENT_RESIZED) {
                repl_handle_resize(repl, event->window.data1, event->window.data2);
                
                // Recalculate visible lines after resize
                repl_calculate_visible_lines(repl);
            }
            break;
    }
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

void repl_render_styled_text(REPL* repl, const char* text, int x, int y, SDL_Color color, bool is_bold) {
    if (!text || !text[0]) return;
    TTF_Font* font_to_use = is_bold ? repl->bold_font : repl->font;
    
    if (!font_to_use) return;
    
    SDL_Surface* surface = TTF_RenderText_Blended(font_to_use, text, color);
    if (!surface) return;
    
    SDL_Texture* texture = SDL_CreateTextureFromSurface(repl->renderer, surface);
    if (!texture) {
        SDL_FreeSurface(surface);
        return;
    }
    
    SDL_Rect dest = {x, y, surface->w, surface->h};
    SDL_RenderCopy(repl->renderer, texture, NULL, &dest);
    
    SDL_FreeSurface(surface);
    SDL_DestroyTexture(texture);
}

void repl_render_background(REPL* repl) {
    // Set the background color
    SDL_SetRenderDrawColor(
        repl->renderer,
        repl->bg_color.r,
        repl->bg_color.g,
        repl->bg_color.b,
        repl->bg_color.a
    );
    SDL_RenderClear(repl->renderer);
    
    // Create a subtle gradient effect
    for (int y = 0; y < repl->window_height; y += 3) {
        // Gradually shift color slightly
        int intensity = 5 + (y * 10 / repl->window_height);
        SDL_SetRenderDrawColor(
            repl->renderer,
            repl->bg_color.r + intensity / 2,
            repl->bg_color.g + intensity / 3,
            repl->bg_color.b + intensity,
            200
        );
        SDL_RenderDrawLine(repl->renderer, 0, y, repl->window_width, y);
    }
    
    // Render output area background (semi-transparent panel)
    int output_height = repl->window_height - 60; // Leave space for input area
    SDL_Rect output_rect = {
        repl->margin, 
        repl->margin + (repl->show_title_bar ? 30 : 0), 
        repl->window_width - 2 * repl->margin,
        output_height - 2 * repl->margin
    };
    
    SDL_SetRenderDrawColor(
        repl->renderer,
        repl->output_bg_color.r,
        repl->output_bg_color.g,
        repl->output_bg_color.b,
        repl->output_bg_color.a
    );
    SDL_RenderFillRect(repl->renderer, &output_rect);
    
    // Draw a subtle border around the output area
    SDL_SetRenderDrawColor(
        repl->renderer,
        repl->prompt_color.r,
        repl->prompt_color.g,
        repl->prompt_color.b,
        100
    );
    SDL_RenderDrawRect(repl->renderer, &output_rect);
}

void repl_render_title_bar(REPL* repl) {
    if (!repl->show_title_bar) return;
    
    // Render title bar background
    SDL_Rect title_rect = {0, 0, repl->window_width, 30};
    
    // Gradient title bar
    SDL_SetRenderDrawColor(
        repl->renderer,
        repl->bg_color.r + 10,
        repl->bg_color.g + 10,
        repl->bg_color.b + 20,
        255
    );
    SDL_RenderFillRect(repl->renderer, &title_rect);
    
    // Title text
    char title[100];
    sprintf(title, "C REPL v2.0 - A Read-Eval-Print Loop");
    
    repl_render_styled_text(
        repl,
        title,
        repl->margin + 5,
        8,
        repl->title_color,
        true
    );
    
    // Draw a separator line below the title
    SDL_SetRenderDrawColor(
        repl->renderer,
        repl->prompt_color.r,
        repl->prompt_color.g,
        repl->prompt_color.b,
        150
    );
    SDL_RenderDrawLine(
        repl->renderer,
        0, 30,
        repl->window_width, 30
    );
}

void repl_render_input_area(REPL* repl) {
    // Calculate position for input area
    int line_height = TTF_FontHeight(repl->font) + 2 * repl->line_padding;
    int input_y = repl->window_height - line_height - 2 * repl->margin;
    
    // Render input background
    SDL_Rect input_rect = {
        repl->margin,
        input_y - repl->line_padding,
        repl->window_width - 2 * repl->margin,
        line_height + 2 * repl->line_padding
    };
    
    // Input box with slightly different color than output area
    SDL_SetRenderDrawColor(
        repl->renderer,
        repl->input_bg_color.r,
        repl->input_bg_color.g,
        repl->input_bg_color.b,
        repl->input_bg_color.a
    );
    SDL_RenderFillRect(repl->renderer, &input_rect);
    
    // Input box border with prompt color
    SDL_SetRenderDrawColor(
        repl->renderer,
        repl->prompt_color.r,
        repl->prompt_color.g,
        repl->prompt_color.b,
        180
    );
    SDL_RenderDrawRect(repl->renderer, &input_rect);
    
    // Render current input with prompt
    char current_input[MAX_INPUT_LENGTH + 3]; // +3 for "> " and null terminator
    sprintf(current_input, "> %s", repl->input_buffer);
    
    repl_render_styled_text(
        repl,
        current_input,
        repl->margin + 5,
        input_y,
        repl->prompt_color,
        false
    );
    
    // Calculate cursor position (account for monospaced font)
    int char_width = TTF_FontHeight(repl->font) / 2; // Approximation for monospaced font
    
    // Render cursor
    SDL_Rect cursor = {
        repl->margin + 5 + (repl->input_cursor + 2) * char_width,
        input_y,
        2,
        line_height
    };
    
    // Make cursor blink
    int ticks = SDL_GetTicks();
    if ((ticks / 500) % 2 == 0) {
        SDL_SetRenderDrawColor(
            repl->renderer,
            repl->prompt_color.r,
            repl->prompt_color.g,
            repl->prompt_color.b,
            255
        );
        SDL_RenderFillRect(repl->renderer, &cursor);
    }
}

void repl_render(REPL* repl) {
    // Draw background
    repl_render_background(repl);
    
    // Draw title bar if enabled
    repl_render_title_bar(repl);
    
    // Calculate starting position for text rendering
    int title_offset = repl->show_title_bar ? 30 : 0;
    int x_start = repl->margin + 10;
    int y_start = repl->margin + title_offset + 5;
    
    // Calculate line height
    int line_height = TTF_FontHeight(repl->font) + 2 * repl->line_padding;
    
    // Render output buffer (with line wrapping and colors)
    char buffer[MAX_OUTPUT_LENGTH];
    strncpy(buffer, repl->output_buffer, sizeof(buffer) - 1);
    buffer[sizeof(buffer) - 1] = '\0';
    
    int y = y_start - (repl->scroll_offset * line_height);
    
    char* line = strtok(buffer, "\n");
    int line_count = 0;
    
    while (line) {
        // Skip lines that are scrolled out of view
        if (y + line_height > title_offset) {
            // Determine text color and style based on content
            SDL_Color color = repl->text_color;
            bool is_bold = false;
            
            if (strncmp(line, "> ", 2) == 0) {
                // Prompt line
                color = repl->prompt_color;
                is_bold = false;
            } 
            else if (strncmp(line, "Error: ", 7) == 0) {
                // Error line
                color = repl->error_color;
                is_bold = true;
            }
            else if (line_count > 0 && 
                     (isdigit(line[0]) || line[0] == '-' || line[0] == '+' || 
                     strstr(line, "=") != NULL)) {
                // Result line
                color = repl->result_color;
                is_bold = true;
            }
            
            // Apply syntax highlighting if enabled
            if (repl->use_syntax_highlighting && strncmp(line, "> ", 2) == 0) {
                // Here we would parse and highlight the input line
                // For simplicity, just render the whole line with prompt color
                repl_render_styled_text(repl, line, x_start, y, color, is_bold);
            } else {
                // Regular rendering
                repl_render_styled_text(repl, line, x_start, y, color, is_bold);
            }
        }
        
        y += line_height;
        line = strtok(NULL, "\n");
        line_count++;
        
        // Stop rendering if we're already below the window
        if (y > repl->window_height - 2 * repl->margin - line_height) {
            break;
        }
    }
    
    // Draw the input area at the bottom
    repl_render_input_area(repl);
    
    // Render the scrollbar
    repl_render_scrollbar(repl);
    
    // Present the rendered frame
    SDL_RenderPresent(repl->renderer);
}

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
}

void repl_navigate_history(REPL* repl, int direction) {
    // Save current input if we're just starting navigation
    static char saved_input[MAX_INPUT_LENGTH] = {0};
    
    if (repl->history_count == 0) {
        return;  // No history to navigate
    }
    
    if (repl->history_index == -1 && direction > 0) {
        // Starting to navigate up from current input
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

void repl_clear_input(REPL* repl) {
    memset(repl->input_buffer, 0, sizeof(repl->input_buffer));
    repl->input_cursor = 0;
    repl->history_index = -1;
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

// Render the scrollbar
void repl_render_scrollbar(REPL* repl) {
    if (!repl->show_scrollbar) return;
    
    // Calculate total and visible lines
    int total_lines = repl_count_output_lines(repl);
    
    // Only show scrollbar if we have more content than can be displayed
    if (total_lines <= repl->visible_lines) return;
    
    // Calculate scrollbar dimensions and positions
    int title_offset = repl->show_title_bar ? 30 : 0;
    int line_height = TTF_FontHeight(repl->font) + 2 * repl->line_padding;
    int input_area_height = line_height + 2 * repl->line_padding;
    
    int scrollbar_height = repl->window_height - title_offset - input_area_height - 4 * repl->margin;
    int scrollbar_x = repl->window_width - repl->scrollbar_width - repl->margin;
    int scrollbar_y = title_offset + 2 * repl->margin;
    
    // Draw scrollbar track
    SDL_Rect track_rect = {
        scrollbar_x,
        scrollbar_y,
        repl->scrollbar_width,
        scrollbar_height
    };
    
    SDL_SetRenderDrawColor(
        repl->renderer,
        repl->scrollbar_bg_color.r,
        repl->scrollbar_bg_color.g,
        repl->scrollbar_bg_color.b,
        repl->scrollbar_bg_color.a
    );
    SDL_RenderFillRect(repl->renderer, &track_rect);
    
    // Calculate thumb dimensions based on visible content percentage
    float visible_portion = (float)repl->visible_lines / total_lines;
    if (visible_portion > 1.0f) visible_portion = 1.0f;
    
    int thumb_height = (int)(scrollbar_height * visible_portion);
    if (thumb_height < 20) thumb_height = 20; // Minimum thumb size
    
    // Calculate thumb position based on scroll position
    int max_thumb_y = scrollbar_y + scrollbar_height - thumb_height;
    int thumb_y = scrollbar_y + (int)(repl->scroll_position * (scrollbar_height - thumb_height));
    
    // Clamp thumb position
    if (thumb_y < scrollbar_y) thumb_y = scrollbar_y;
    if (thumb_y > max_thumb_y) thumb_y = max_thumb_y;
    
    // Draw scrollbar thumb
    SDL_Rect thumb_rect = {
        scrollbar_x,
        thumb_y,
        repl->scrollbar_width,
        thumb_height
    };
    
    SDL_SetRenderDrawColor(
        repl->renderer,
        repl->scrollbar_color.r,
        repl->scrollbar_color.g,
        repl->scrollbar_color.b,
        repl->scrollbar_color.a
    );
    SDL_RenderFillRect(repl->renderer, &thumb_rect);
    
    // Draw a border around the thumb
    SDL_SetRenderDrawColor(
        repl->renderer,
        repl->scrollbar_color.r,
        repl->scrollbar_color.g,
        repl->scrollbar_color.b,
        255
    );
    SDL_RenderDrawRect(repl->renderer, &thumb_rect);
}

static bool is_command(const char* input) {
    // Check if input starts with a known command
    while (*input == ' ') input++; // Skip leading spaces
    
    return (strcmp(input, HELP_CMD) == 0 || 
            strcmp(input, CLEAR_CMD) == 0 || 
            strcmp(input, EXIT_CMD) == 0 || 
            strcmp(input, QUIT_CMD) == 0 ||
            strcmp(input, VARS_CMD) == 0 ||
            strcmp(input, VERSION_CMD) == 0);
}

static bool handle_command(REPL* repl, const char* input) {
    static char result[MAX_INPUT_LENGTH];
    result[0] = '\0';
    
    while (*input == ' ') input++; // Skip leading spaces
    
    if (strcmp(input, HELP_CMD) == 0) {
        repl_show_help(repl);
        return true;
    }
    else if (strcmp(input, CLEAR_CMD) == 0) {
        strcpy(repl->output_buffer, "REPL cleared.\n> ");
        return true;
    }
    else if (strcmp(input, EXIT_CMD) == 0 || strcmp(input, QUIT_CMD) == 0) {
        strcpy(result, "Exiting REPL...");
        repl_print(repl, result, false);
        repl->running = false;
        return true;
    }
    else if (strcmp(input, VARS_CMD) == 0) {
        if (repl->variable_count == 0) {
            strcpy(result, "No variables defined.");
            repl_print(repl, result, false);
            return true;
        }
        
        strcpy(result, "Variables:");
        for (int i = 0; i < repl->variable_count; i++) {
            char var_str[100];
            sprintf(var_str, "\n  %s = %.6g", repl->variables[i].name, repl->variables[i].value);
            strcat(result, var_str);
        }
        
        repl_print(repl, result, false);
        return true;
    }
    else if (strcmp(input, VERSION_CMD) == 0) {
        strcpy(result, "C REPL v2.0 - A Read-Eval-Print Loop for C");
        repl_print(repl, result, false);
        return true;
    }
    
    return false;
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