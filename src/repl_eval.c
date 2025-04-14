#include "../include/repl_eval.h"
#include "../include/repl_variables.h"
#include "../include/repl_ui.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>

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

// Built-in commands
static const char* HELP_CMD = "help";
static const char* CLEAR_CMD = "clear";
static const char* EXIT_CMD = "exit";
static const char* QUIT_CMD = "quit";
static const char* VARS_CMD = "vars";
static const char* VERSION_CMD = "version";

// Forward declarations of helper functions - make these local to the module
static void tokenize(REPL* repl, const char* expr, Token* tokens, int* token_count, bool* error);
static double parse_expression(REPL* repl, Token* tokens, int* pos, int token_count, bool* error);
static double parse_term(REPL* repl, Token* tokens, int* pos, int token_count, bool* error);
static double parse_factor(REPL* repl, Token* tokens, int* pos, int token_count, bool* error);

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
double evaluate_expression(REPL* repl, const char* expr, bool* error) {
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

bool is_command(const char* input) {
    // Skip leading whitespace
    while (isspace(*input)) input++;
    
    // Check if the input contains any whitespace or operators
    for (const char* c = input; *c; c++) {
        if (isspace(*c) || *c == '=' || *c == '+' || *c == '-' || *c == '*' || *c == '/') {
            return false;
        }
    }
    
    // Check against known commands
    return (strcmp(input, HELP_CMD) == 0 ||
            strcmp(input, CLEAR_CMD) == 0 ||
            strcmp(input, EXIT_CMD) == 0 ||
            strcmp(input, QUIT_CMD) == 0 ||
            strcmp(input, VARS_CMD) == 0 ||
            strcmp(input, VERSION_CMD) == 0);
}

bool handle_command(REPL* repl, const char* input) {
    static char result_buffer[MAX_OUTPUT_LENGTH];
    
    // Skip leading whitespace
    while (isspace(*input)) input++;
    
    if (strcmp(input, HELP_CMD) == 0) {
        repl_show_help(repl);
        return true;
    }
    else if (strcmp(input, CLEAR_CMD) == 0) {
        // Clear output buffer except for a new prompt
        memset(repl->output_buffer, 0, sizeof(repl->output_buffer));
        strcpy(repl->output_buffer, "> ");
        return true;
    }
    else if (strcmp(input, EXIT_CMD) == 0 || strcmp(input, QUIT_CMD) == 0) {
        repl->running = false;
        strcpy(result_buffer, "Exiting...");
        repl_print(repl, result_buffer, false);
        return true;
    }
    else if (strcmp(input, VARS_CMD) == 0) {
        repl_list_variables(repl, result_buffer, sizeof(result_buffer));
        repl_print(repl, result_buffer, false);
        return true;
    }
    else if (strcmp(input, VERSION_CMD) == 0) {
        strcpy(result_buffer, "C REPL v2.0 - A simple expression evaluator");
        repl_print(repl, result_buffer, false);
        return true;
    }
    
    return false;
}