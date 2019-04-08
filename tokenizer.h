#pragma once

/* A struct that represents a list of words. */
struct tokens;

/* Turn a string into a list of words. */
struct tokens *tokenize(const char *line);

/* How many words are there? */
size_t tokens_get_length(struct tokens *tokens);

/* Get me the Nth word (zero-indexed) */
char *tokens_get_token(struct tokens *tokens, size_t n);

/* shorten the token when redirecting */
void shorten_token(struct tokens *tokens);

/* print tokens array */
void print_tokens_array(char **tokens, size_t n);

/* print tokens */
void print_tokens(struct tokens *tokens);

/* get string array */
char **get_tokens(struct tokens *tokens);

/* Free the memory */
void tokens_destroy(struct tokens *tokens);
