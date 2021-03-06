#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#define MAXLINE 256

char history[MAXLINE][MAXLINE];
int hiscnt = 0;

void run(char *cmd, int *fd, int fdin, int out) {
	char *argv[MAXLINE];
	int argc, i, status;
	pid_t pid;
	char *pt;
	char rdinput, rdoutput;
	int bg;
	rdinput = ' ', rdoutput = ' ';
	argc = 0;
	bg = 0;
	argv[0] = cmd;
	for (pt = cmd; *pt != '\0'; ++pt) {
		if (*pt == ' ') {
			*pt = '\0';
			argv[++argc] = pt + 1;
		}
	}
	argv[++argc] = NULL;
	for (i = 0; i < argc; ++i) {
		if (!strcmp(argv[i], ">")) {
			rdoutput = 'w';
			argv[argc - 2] = NULL;
		}
		else if (!strcmp(argv[i], ">>")) {
			rdoutput = 'a';
			argv[argc - 2] = NULL;
		}
		else if (!strcmp(argv[i], "<")) {
			rdinput = 'r';
			argv[argc - 2] = NULL;
		}
		else if (!strcmp(argv[i], "&"))
			bg = 1;
	}
	if (!strcmp(argv[0], "exit"))
		exit(0);
	else if (!strcmp(argv[0], "cd")) {
		chdir(argv[1]);
		return;
	}
	else if (!strcmp(argv[0], "history")) {
		bg = argc > 1 ? atoi(argv[1]) : hiscnt;
		bg = bg > hiscnt ? hiscnt : bg;
		for (i = 0; i < bg; ++i)
			printf("  %d  %s\n", i, history[i]);
		return;
	}
	pid = fork();
	if (!pid) {
		if (rdinput == 'r')
			freopen(argv[argc - 1], "r", stdin);
		if (rdoutput == 'w')
			freopen(argv[argc - 1], "w", stdout);
		else if (rdoutput == 'a')
			freopen(argv[argc - 1], "a", stdout);
		dup2(fdin, STDIN_FILENO);
		if (!out)
			dup2(fd[1], STDOUT_FILENO);
		close(fd[0]);
		if (execvp(argv[0], argv) < 0) {
			fprintf(stderr, "%s: Command not found.\n", argv[0]);
			exit(0);
		}
	}

	if (!bg && waitpid(pid, &status, 0) < 0)
		printf("waitfg: waitpid error %d", status);
	else if (bg)
		printf("[%d] %s\n", pid, cmd);
}

void execute(char *lines[MAXLINE]) {
	char **pt;
	int fd[2];
	int out, fdin;
	out = 1, fdin = STDIN_FILENO;
	for (pt = lines; *pt; ++pt) {
		pipe(fd);
		if (*(pt + 1))
			out = 0;
		else
			out = 1;
		run(*pt, fd, fdin, out);
		close(fd[1]);
		fdin = fd[0];
	}
}

void parse(char *line) {
	char buf[MAXLINE];
	int len;
	int ncmd;
	char *pt;
	char *clauses[MAXLINE];
	len = 0;
	for (pt = line; *pt != '\0'; ++pt) {
		while (*pt == ' ' && (*(pt + 1) == ' ' || pt == line))
			++pt;
		if (!len && *pt == ' ')
			continue;
		buf[len++] = *pt != '\n' ? *pt : '\0';
	}
	if (buf[0] == '\0')
		return;
	strcpy(history[hiscnt++], buf);
	if (hiscnt == MAXLINE)
		hiscnt = 0;
	ncmd = 0;
	clauses[0] = buf;
	for (pt = buf; *pt != '\0'; ++pt) {
		if (*pt == '|') {
			*pt = '\0';
			clauses[++ncmd] = *(pt + 1) == ' ' ? pt + 2 : pt + 1;
		}
	}
	clauses[++ncmd] = NULL;
	execute(clauses);
}

int main() {
	char cmdline[MAXLINE];
	char cwd[MAXLINE];
	while (1) {
		printf("%s> ", getcwd(cwd, MAXLINE));
		fgets(cmdline, MAXLINE, stdin);
		if (feof(stdin))
			exit(0);
		parse(cmdline);
	}
	return 0;
}
