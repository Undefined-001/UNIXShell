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
	return arguments;
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
	return launch(args);
}

bool launch(char** args)
{
	pid_t pid = fork();
	if(pid == 0)
	{
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
		waitpid(pid, &status, WUNTRACED);
	}
	return true;
}


