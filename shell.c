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
int detect_redirection(struct tokens *tokens) {
	for (int i = 0; i < tokens_get_length(tokens); i++) {
		/* redirect input detected */
		if (!strcmp(tokens_get_token(tokens, i), "<")) {
			// check the source of input direction
			if (i != tokens_get_length(tokens) - 2) {
				return -2;
			} else { // input redirection detected
				return 0;
			}
		/* redirect output detected */
		} else if (!strcmp(tokens_get_token(tokens, i), ">")) {
			/* check if there is a target or source for redirection */
			if (i != tokens_get_length(tokens) - 2) {
				return -2;
			} else { // output redirection detected
				return 1;
			}
		}
	}
	return -1;
}

/* detect bg processing operator '&' */
int bg_proc_detect(struct tokens *tokens) {
	if (tokens_get_length(tokens) == 0)
		return -1;
    if(!strcmp(tokens_get_token(tokens, tokens_get_length(tokens) - 1), "&"))
        return 0;
    else
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

void child_handler() {
	printf("child process terminated.\n");
}

int main(unused int argc, unused char *argv[]) {
	init_shell();

	static char line[4096];
	/* get the environment variable PATH */
	char *path = getenv("PATH");
	/* set deliminator to colon */
	char *delim = ":";
	/* init bg jobs */
	int jobs[100];
	/* init temp exepath to store temp string */
	char execPath[100];
	/* Please only print shell prompts when standard input is not a tty */
	if (shell_is_interactive)
		fprintf(stdout, "Anqi's Shell >> ");
	while (fgets(line, 4096, stdin)) {
		/* Split our line into words. */
		struct tokens *tokens = tokenize(line);
		/* get a copy of tokens pointer array */
		char *tokens_copy[20] = {NULL};
		memcpy(tokens_copy, get_tokens(tokens), tokens_get_length(tokens) * sizeof(char*)); 
		/* file descripter */
		int fd;
		int saved_stdout = dup(1);
		int saved_stdin = dup(0);
		/* redirection flag for free mem use */
		_Bool redir_out = false;
		_Bool redir_in = false;
		_Bool bg_proc = false;
		/* Find which built-in function to run. */
		int fundex = lookup(tokens_get_token(tokens, 0));
		/* cg processing detect */
		if (!bg_proc_detect(tokens)) {
			bg_proc = true;
			// prevent children from being zombie
        	signal(SIGCHLD, child_handler);
			tokens_copy[tokens_get_length(tokens) - 1] = NULL;
			shorten_tokens(tokens, 1);
		} else {
			signal(SIGCHLD, SIG_DFL);
		}
		/* check redirection output */
		if (detect_redirection(tokens) == 1) {
			/* open file */
			fd = open(tokens_get_token(tokens, tokens_get_length(tokens) - 1), O_WRONLY | O_CREAT);
			/* redirect output */
			if (dup2(fd, 1) < 0)
				printf("Convert output failed.");
			/* set flag */
			redir_out = true;
			/* shorten the tokens array: eliminate last two elements */
			tokens_copy[tokens_get_length(tokens) - 1] = NULL;
			tokens_copy[tokens_get_length(tokens) - 2] = NULL;
			shorten_tokens(tokens, 2);
		/* check redirection input */
		} else if (detect_redirection(tokens) == 0) {
			/* open input redirection file */
			fd = open(tokens_get_token(tokens, tokens_get_length(tokens) - 1), O_RDONLY);
			/* error checking */
			if (fd < 0) {
				printf("Input file doesn't exist.\n");
				exit(0);
			}
			/* redirection input*/
			if (dup2(fd, 0) < 0)
				printf("Convert input failed.");
			/* set flag */
			redir_in = true;
            tokens_copy[tokens_get_length(tokens) - 1] = NULL;
            tokens_copy[tokens_get_length(tokens) - 2] = NULL;
		}
		if (fundex >= 0) {
			cmd_table[fundex].fun(tokens);
		} else {
			pid_t child_pid;
			int status;
			child_pid = fork();
			if(child_pid == 0) {
				/* let signal work as default */
				signal(SIGSTOP, SIG_DFL);
				signal(SIGINT, SIG_DFL);
				signal(SIGQUIT, SIG_DFL);
				/* set parent group id */
				pid_t cpid = getpid();
				/* change process group to its own group */
				if (setpgid(cpid, cpid) < 0)
					perror("setpgid failed.");
				/* ignore tty output signal */
				signal(SIGTTOU, SIG_IGN);
				/* change to fg process */
				if (!bg_proc) {
					if(tcsetpgrp(0, cpid) < 0)
                    	perror("tcsetpgrp failed.");
				}
				/* This is done by the child process. */
				execv(tokens_get_token(tokens, 0), tokens_copy);
				/* if this call return, meaning PATH is not correct */
				char *token = strtok(path, delim);
				while(token != NULL)
				{
					/* concatenate Full Path */
					strcpy(execPath, token);
					strcat(execPath, "/");
					strcat(execPath, tokens_get_token(tokens, 0));
					tokens_copy[0] = execPath;
					/* execute the command */
					execv(execPath, tokens_copy);
					token = strtok(NULL, delim);
				}

				/* If execvp returns, it must have failed. */
				printf("Unknown command\n");
				tokens_destroy(tokens);			
				exit(0);
			}
			else {
				/* ignore signals in the background process */
				signal(SIGINT, SIG_IGN);
				signal(SIGQUIT, SIG_IGN);
				signal(SIGSTOP, SIG_IGN);
				signal(SIGTTOU, SIG_IGN);
				/* get parent process id */
				pid_t ppid = getpid();
				/* set background process to its own group */
				if (setpgid(ppid, ppid) < 0)
					perror("set group id failed.");
				/* wait for child process */
				if (!bg_proc) {
					wait(&status);
				}
				if (tcsetpgrp(0, ppid) < 0)
					perror("convert background to fg failed.");
				signal(SIGTTOU, SIG_DFL);
			}
			/* check if the output is redirected */
			if (redir_out) {
				if(dup2(saved_stdout, 1) < 0)
					printf("convert stdout failed...\n");
				close(saved_stdout);
				close(fd);
				redir_out = false;
			}
			/* check if the input is redirected */
			if (redir_in) {
				if(dup2(saved_stdin, 0) < 0)
					printf("Convert stdin Back failed...\n");
				close(saved_stdin);
				close(fd);
				redir_in = false;
			}
		}

		if (shell_is_interactive)
			/* Please only print shell prompts when standard input is not a tty */
			fprintf(stdout, "Anqi's Shell >> ");
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
