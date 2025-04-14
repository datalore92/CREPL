#include "../include/repl_ui.h"
#include <stdio.h>
#include <string.h>
#include <ctype.h>

void repl_render_text(REPL* repl, const char* text, int x, int y, SDL_Color color) {
    repl_render_styled_text(repl, text, x, y, color, false);
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
    sprintf(title, "C REPL v1.0 - A Read-Eval-Print Loop");
    
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