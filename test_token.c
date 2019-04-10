#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "tokenizer.h"

int detect_redirection(struct tokens *tokens) {
	for (int i = 0; i < tokens_get_length(tokens); i++) {
		/* redirect input detected */
		if (!strcmp(tokens_get_token(tokens, i), "<")) {
			if (i != tokens_get_length(tokens) - 2) {
				return -2;
			} else {
				return 0;
			}
			/* redirect output detected */
		} else if (!strcmp(tokens_get_token(tokens, i), ">")) {
			/* check if there is a target or source for redirection */
			if (i != tokens_get_length(tokens) - 2) {
				printf("Arguments are not enough...");
				return -2;
			} else {
				printf("output redirection has been detected.");
				/* open target file for redirection */
				// fd = open(tokens_get_token(tokens, i + 1), O_WRONLY | O_CREAT);
				// if (fd < 0) break;
				// if (dup2(fd, 1) == -1) break;
				return 1;
			}
		}
	}
	return -1;
}

int bg_proc_detect(struct tokens *tokens) {
	if(!strcmp(tokens_get_token(tokens, tokens_get_length(tokens) - 1), "&"))
		return 0;
	else
		return -1;
}

int main() {
	char line[200];
	while(fgets(line,199, stdin)) {
		struct tokens *tokens = tokenize(line);
		// printf("%d\n", detect_redirection(tokens));	
		// char **ele = get_tokens(tokens);
		// ele[2] = NULL;
		// for (int i = 0; i < 3; i++) {
		// 	printf("%p\n", ele[i]);;
		// }
		printf("%d\n", bg_proc_detect(tokens));
		tokens_destroy(tokens);
	}
}
