#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "tokenizer.h"

int main() {
	char line[20] = "ls -all -t";
	struct tokens *tokens = tokenize(line);
	char **ele = get_tokens(tokens);
	ele[2] = NULL;
	for (int i = 0; i < 3; i++) {
		printf("%p\n", ele[i]);;
	}
	tokens_destroy(tokens);
}
