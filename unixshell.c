#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <sys/wait.h>
#include <unistd.h>

void loop();
char* read_input();
char** split_input(char* in);
bool execute(char** args);
bool launch(char** args);
void io_redirect(bool inflag, bool outflag, bool outflagappend, char* infile, char* outfile);
char infile[256] = "";
char outfile[256] = "";
bool inflag;
bool outflag;
bool outflagappend;
bool backgroundflag;

int main(int argc, char **argv)
{
	loop();

	return EXIT_SUCCESS;
}

void loop()
{
	char* in;
	char** args;
	bool status = true;
	while(status)
	{
		inflag = false;
		outflag = false;
		outflagappend = false;
		backgroundflag = false;
		strcpy(infile, "");
		strcpy(outfile, "");
		printf("> ");
		in = read_input();
		args = split_input(in);
		status = execute(args);
		free(in);
		for(int i = 0; args[i] != NULL; i++)
		{
			free(args[i]);
		}
		free(args);
	}
}

char* read_input()
{
	char buffer[1024];
	if(fgets(buffer, sizeof(buffer), stdin) == NULL)
	{
		return NULL;
	}
	buffer[strcspn(buffer, "\n")] = '\0';
	char* buf = malloc(strlen(buffer) + 1);
	strcpy(buf, buffer);
	return buf;
}
char** split_input(char* in)
{
	char* args[64];
	int i = 0;
	args[i] = strtok(in, " ");
	while(args[i] != NULL && i < 63)
	{
		i++;
		args[i] = strtok(NULL, " ");
	}
	char** arguments = malloc((i + 1)*sizeof(char*));
	if(arguments == NULL)
	{
		perror("malloc");
		exit(EXIT_FAILURE);
	}
	for(int j = 0; j < i; j++)
	{
		arguments[j] = malloc(strlen(args[j]) + 1);
		if(arguments[j] == NULL)
		{
			perror("malloc");
			exit(EXIT_FAILURE);
		}
		strcpy(arguments[j], args[j]);
	}
	arguments[i] = NULL;
	for(int k = 0; k < i; k++)
	{
		if((strcmp(arguments[k], ">")) == 0 || (strcmp(arguments[k], ">>")) == 0)
		{
			if(arguments[k+1] == NULL)
			{
				perror("Invalid command");
				exit(EXIT_FAILURE);
			}
			strcpy(outfile,arguments[k+1]);
			outflag = true;
			if(strcmp(arguments[k], ">>") == 0)
			{
				outflagappend = true;
			}

			for(int m = k; m < i - 2; m++)
			{
				arguments[m] = arguments[m + 2];
			}
			i -= 2;
			k--;
			arguments[i] = NULL;
		}
		else if(strcmp(arguments[k], "<") == 0)
		{
			if(arguments[k+1] == NULL)
                        {
                                perror("Invalid command");
                                exit(EXIT_FAILURE);
                        }
			strcpy(infile, arguments[k+1]);
			inflag = true;

			for(int m = k; m < i - 2; m++)
                        {
                                arguments[m] = arguments[m + 2];
                        }
			i -= 2;
			k--;
			arguments[i] = NULL;
		}
	}
	if(i > 0 && (strcmp(arguments[i - 1], "&") == 0))
	{
		backgroundflag = true;
		i--;
		arguments[i] = NULL;
	}

	return arguments;
}

void io_redirect(bool inflag, bool outflag, bool outflagappend, char* infile, char* outfile)
{
	if(inflag)
	{
		int fd = open(infile, O_RDONLY);
		if(fd == -1)
		{
			perror("open");
			exit(EXIT_FAILURE);
		}
		dup2(fd, 0);
		close(fd);
	}
	if(outflag)
	{
		int fd = -1;
		if(outflagappend)
		{
			fd = open(outfile, O_WRONLY | O_APPEND, 0644);
		}
		else
		{
			fd = open(outfile, O_WRONLY | O_TRUNC, 0644);
		}
		if(fd == -1)
		{
			perror("open");
			exit(EXIT_FAILURE);
		}
		dup2(fd, 1);
		close(fd);
	}
}

bool execute(char** args)
{
	if(args[0] == NULL)
	{
		return true;
	}
	if(strcmp(args[0], "exit") == 0)
	{
		return false;
	}
	if(strcmp(args[0], "cd") == 0)
	{
		if(args[1])
		{
			if(chdir(args[1]) != 0)
			{
				perror("cd");
			}
		}
		else
		{
			if(chdir(getenv("HOME")) != 0)
			{
				perror("cd");
			}
		}
		return true;
	}
	return launch(args);
}

bool launch(char** args)
{
	pid_t pid = fork();
	if(pid == 0)
	{
		if(outflag || inflag)
        	{
                	io_redirect(inflag, outflag, outflagappend, infile, outfile);
        	}
		int status = execvp(args[0], args);
		if(status == -1)
		{
			perror("execvp");
		}
		exit(EXIT_FAILURE);
	}
	else if(pid < 0)
	{
		perror("fork");
	}
	else
	{
		int status;
		if(!backgroundflag)
		{
			waitpid(pid, &status, WUNTRACED);
		}
		else
		{
			printf("Background PID %d\n", pid);
		}
	}
	return true;
}




