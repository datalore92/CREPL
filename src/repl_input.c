#include "../include/repl_input.h"
#include "../include/repl_history.h"
#include <string.h>
#include <limits.h>
#include <ctype.h>

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
                // Check if mouse is over the output area (for scrolling content)
                int title_offset = repl->show_title_bar ? 30 : 0;
                int line_height = TTF_FontHeight(repl->font) + 2 * repl->line_padding;
                int input_area_y = repl->window_height - line_height - 2 * repl->line_padding - repl->margin;
                int mouse_x, mouse_y;
                
                // Get current mouse position
                SDL_GetMouseState(&mouse_x, &mouse_y);
                
                // If mouse is over the input area or near it, use wheel for history navigation
                if (mouse_y >= input_area_y - 20) {
                    // Use mouse wheel for history navigation
                    // Invert wheel direction for history navigation
                    if (event->wheel.y > 0) {
                        // Scroll up = older commands (positive direction)
                        repl_navigate_history(repl, 1);
                    } else {
                        // Scroll down = newer commands (negative direction)
                        repl_navigate_history(repl, -1);
                    }
                } else {
                    // Otherwise, use wheel for scrolling output content
                    int total_lines = repl_count_output_lines(repl);
                    
                    // Calculate how much to scroll based on total lines
                    float scroll_amount = 0.05f;  // Percentage of total scrollable content
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

void repl_clear_input(REPL* repl) {
    memset(repl->input_buffer, 0, sizeof(repl->input_buffer));
    repl->input_cursor = 0;
    repl->history_index = -1;
}