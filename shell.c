#include <ctype.h>
#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <signal.h>
#include <sys/wait.h>
#include <termios.h>
#include <unistd.h>
#include <fcntl.h>

#include "tokenizer.h"

extern char **environ;

/* Convenience macro to silence compiler warnings about unused function parameters. */
#define unused __attribute__((unused))

/* Whether the shell is connected to an actual terminal or not. */
bool shell_is_interactive;

/* File descriptor for the shell input */
int shell_terminal;

/* Terminal mode settings for the shell */
struct termios shell_tmodes;

/* Process group id for the shell */
pid_t shell_pgid;

/* helper function: match string start with */
_Bool startsWith(const char *pre, const char *str) {
	size_t lenpre = strlen(pre),
		   lenstr = strlen(str);
	return lenstr < lenpre ? 0 : strncmp(pre, str, lenpre) == 0;
}

int cmd_exit(struct tokens *tokens);
int cmd_help(struct tokens *tokens);
int cmd_pwd(struct tokens *tokens);
int cmd_cd(struct tokens *tokens);
/* Built-in command functions take token array (see parse.h) and return int */
typedef int cmd_fun_t(struct tokens *tokens);

/* Built-in command struct and lookup table */
typedef struct fun_desc {
	cmd_fun_t *fun;
	char *cmd;
	char *doc;
} fun_desc_t;

fun_desc_t cmd_table[] = {
	{cmd_help, "?", "show this help menu"},
	{cmd_exit, "exit", "exit the command shell"},
	{cmd_pwd, "pwd", "print working directory"},
	{cmd_cd, "cd", "change directory"}
};

/* Prints a helpful description for the given command */
int cmd_help(unused struct tokens *tokens) {
	for (unsigned int i = 0; i < sizeof(cmd_table) / sizeof(fun_desc_t); i++)
		printf("%s - %s\n", cmd_table[i].cmd, cmd_table[i].doc);
	return 1;
}

/* Exits this shell */
int cmd_exit(unused struct tokens *tokens) {
	exit(0);
}

/* print working directory */
int cmd_pwd(unused struct tokens *tokens) {
	char *buf = malloc(50 * sizeof(char));
	getcwd(buf, 50);
	printf("%s\n", buf);
	free(buf);
	return 1;
}

/* change directory */
int cmd_cd(struct tokens *tokens) {
	if (chdir(tokens_get_token(tokens, 1))) {
		printf("No such file exist.\n");
		return 0;
	}
	return 1;
}

/* Looks up the built-in command, if it exists. */
int lookup(char cmd[]) {
	for (unsigned int i = 0; i < sizeof(cmd_table) / sizeof(fun_desc_t); i++)
		if (cmd && (strcmp(cmd_table[i].cmd, cmd) == 0))
			return i;
	return -1;
}

/* detect redirection in the input        */
/* output -1 meaning without redirection  */
/* output -2 meaning invalide redirection */
/* output  0 meaning input redirection    */
/* output  1 meaning output redirection   */
int detect_redirection(struct tokens *tokens, int fd) {
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

/* Intialization procedures for this shell */
void init_shell() {
	/* Our shell is connected to standard input. */
	shell_terminal = STDIN_FILENO;

	/* Check if we are running interactively */
	shell_is_interactive = isatty(shell_terminal);

	if (shell_is_interactive) {
		/* If the shell is not currently in the foreground, we must pause the shell until it becomes a
		 * foreground process. We use SIGTTIN to pause the shell. When the shell gets moved to the
		 * foreground, we'll receive a SIGCONT. */
		while (tcgetpgrp(shell_terminal) != (shell_pgid = getpgrp()))
			kill(-shell_pgid, SIGTTIN);

		/* Saves the shell's process id */
		shell_pgid = getpid();

		/* Take control of the terminal */
		tcsetpgrp(shell_terminal, shell_pgid);

		/* Save the current termios to a variable, so it can be restored later. */
		tcgetattr(shell_terminal, &shell_tmodes);
	}
}

int main(unused int argc, unused char *argv[]) {
	init_shell();

	static char line[4096];
	int line_num = 0;
	/* get the environment variable PATH */
	char *path = getenv("PATH");
	/* set deliminator to colon */
	char *delim = ":";
	/* init temp exepath to store temp string */
	char execPath[100];
	/* Please only print shell prompts when standard input is not a tty */
	if (shell_is_interactive)
		fprintf(stdout, "%d: ", line_num);

	while (fgets(line, 4096, stdin)) {
		/* Split our line into words. */
		struct tokens *tokens = tokenize(line);
		/* file descripter */
		int fd;
		/* redirection flag for free mem use */
		_Bool redir_out = false;
		_Bool redir_in = false;
		/* Find which built-in function to run. */
		int fundex = lookup(tokens_get_token(tokens, 0));

		if (fundex >= 0) {
			cmd_table[fundex].fun(tokens);
		} else {
			pid_t child_pid;
			int status;
			child_pid = fork();
			if(child_pid == 0) {
				/* This is done by the child process. */
				execv(tokens_get_token(tokens, 0), get_tokens(tokens));
				/* if this call return, meaning PATH is not correct */
				char *token = strtok(path, delim);
				while(token != NULL)
				{
					strcpy(execPath, token);
					strcat(execPath, "/");
					strcat(execPath, tokens_get_token(tokens, 0));
					execv(execPath, get_tokens(tokens));
					/* free the memory if execution failed */
					// free(temp_tokens);
					token = strtok(NULL, delim);
				}

				/* If execvp returns, it must have failed. */
				printf("Unknown command\n");
				tokens_destroy(tokens);			
				exit(0);
			}
			else {
				wait(&status);
			}
			// free(temp_tokens);
		}

		if (shell_is_interactive)
			/* Please only print shell prompts when standard input is not a tty */
			fprintf(stdout, "%d: ", ++line_num);
		/* if stdout is redirected we need to close the fd file */
		if (redir_out) {
			redir_out = false;
		}
		/* if stdin is redirected we need to modify back*/
		if (redir_in) {
			redir_in = false;
		}
		/* Clean up memory */
		tokens_destroy(tokens);
	}

	return 0;
}
