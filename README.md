# Anqi-s-Shell
Author: Anqi Ni

## Intro
This shell is built by myself using operation system knowledge. This shell can support various functionality including basic instructions like `pwd`, `cd`, `help`.
It can also run a command by specifying a executable file defined by users like `/usr/bin/wc shell.c`. We included signal handling in this project.
So, it may be a little user-unfriendly since **control-c** interrupt signal is disabled but you can exit the shell by typing exit due to the principle that a shell cannot exit from itself using an interrput.
We also process group and background processing in this shell, so you can have a lot of fun using it.
