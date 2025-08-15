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
void sigchld_handler();
void remove_node(pid_t pid);
void cleanup_jobs();
char*** parse_pipes(char** args, int numpipes);
bool launch_pipes(char*** commands);
int count_pipes(char** args);
char infile[256] = "";
char outfile[256] = "";
bool inflag;
bool outflag;
bool outflagappend;
bool backgroundflag;
int backgroundid;
struct BackgroundTask
{
	int id;
	char command[256];
	struct BackgroundTask* next;
	pid_t pid;
	bool done;
};
struct BackgroundTask* currjob;
int main(int argc, char **argv)
{
	signal(SIGCHLD, sigchld_handler);
	loop();
	return EXIT_SUCCESS;
}

void loop()
{
	char* in;
	char** args;
	bool status = true;
	backgroundid = 0;
	currjob = NULL;
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
		int numpipes = count_pipes(args);
		if(numpipes == 0)
		{
			status = execute(args);
		}
		else
		{
			char*** commands = parse_pipes(args, numpipes);
                        launch_pipes(commands);
			for (int i = 0; commands[i] != NULL; i++)
			{
				for (int j = 0; commands[i][j] != NULL; j++)
				{
			        	free(commands[i][j]);
			    	}
			    	free(commands[i]);
			}
			free(commands);

		}
		free(in);
		for(int i = 0; args[i] != NULL; i++)
		{
			free(args[i]);
		}
		free(args);
	}
}

int count_pipes(char** args)
{
	int count = 0;
	for(int i = 0; args[i] != NULL; i++)
	{
		if(strcmp(args[i], "|") == 0)
		{
			count++;
		}
	}
	return count;
}

bool launch_pipes(char*** commands)
{
	int prev_fd = -1;
	for(int i = 0; commands[i] != NULL; i++)
	{
		int fd[2];
		if(commands[i + 1] != NULL)
		{
			int s = pipe(fd);
			if(s < 0)
			{
				perror("pipe");
				return true;
			}
		}
	        pid_t pid = fork();
	        if(pid == 0)
        	{
			if(i > 0)
			{
				dup2(prev_fd, 0);
				close(prev_fd);
			}
			if(commands[i + 1] != NULL)
			{
				dup2(fd[1], 1);
				close(fd[0]);
				close(fd[1]);
			}
	        	if(i == 0 && inflag)
                	{
                        	io_redirect(inflag, false, false, infile, NULL);
	                }
			if(commands[i + 1] == NULL && outflag)
			{
				io_redirect(false, outflag, outflagappend, NULL, outfile);
			}
        	        int status = execvp(commands[i][0], commands[i]);
                	if(status == -1)
	                {
        	                perror("execvp");
                	}
	                exit(EXIT_FAILURE);
        	}
	        else if(pid < 0)
        	{
                	perror("fork");
			return true;
	        }
        	else
        	{
			if(i > 0)
			{
				close(prev_fd);
			}
			if(commands[i + 1] != NULL)
			{
				close(fd[1]);
				prev_fd = fd[0];
			}
                	int status;
	                if(!backgroundflag)
        	        {
                	        waitpid(pid, &status, WUNTRACED);
                	}
                	else if(commands[i + 1] == NULL)
	                {
        	                backgroundid++;
                	        struct BackgroundTask* job = malloc(sizeof(struct BackgroundTask));
                        	job->pid = pid;
	                        job->done = false;
        	                job->command[0] = '\0';
				for(int i = 0; commands[i] != NULL; i++)
				{
					for(int j = 0; commands[i][j] != NULL; j++)
					{
						strcat(job->command, commands[i][j]);
			                        strcat(job->command, " ");
					}
					if(commands[i + 1] != NULL)
					{
						strcat(job->command, "| ");
					}
				}
                        	job->command[strlen(job->command) - 1] = '\0';
	                        job->id = backgroundid;
        	                job->next = currjob;
                	        currjob = job;
                        	printf("Background PID %d\n", pid);
	                }
		}
	}
	return true;
}

char*** parse_pipes(char** args, int numpipes)
{
	char*** commands = malloc((numpipes+2)*sizeof(char**));
	int j = 0;
	int k = 0;
	int i;
	for(i = 0; args[i] != NULL; i++)
	{
		if(strcmp(args[i], "|") == 0)
		{
			int length = i - k;
			char** cmd = malloc((length + 1)*sizeof(char*));
			for(int n = 0; n < length; n++)
			{
				cmd[n] = strdup(args[k + n]);
			}
			cmd[length] = NULL;
			commands[j] = cmd;
			j++;
			k = i + 1;
		}
		if(j == numpipes)
		{
			break;
		}
	}
	if(args[k] != NULL)
	{
		int length = 0;
		while (args[k + length] != NULL)
		{
			length++;
		}
	        char** cmd = malloc((length + 1)*sizeof(char*));
        	for(int n = 0; n < length; n++)
        	{
        		cmd[n] = strdup(args[k + n]);
	        }
        	cmd[length] = NULL;
	        commands[j] = cmd;
        	j++;
		commands[j] = NULL;
	}
	return commands;
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

void cleanup_jobs()
{
	struct BackgroundTask* curr = currjob;
	while(curr != NULL)
	{
		struct BackgroundTask* currnext = curr->next;
		if(curr->done)
		{
			remove_node(curr->pid);
		}
		curr = currnext;
	}
}

void sigchld_handler()
{
	int status;
	pid_t pid;
	pid = waitpid(-1, &status, WNOHANG);
	while (pid > 0)
	{
		for(struct BackgroundTask* j = currjob; j != NULL; j = j->next)
		{
			if(j->pid == pid)
			{
				j->done = true;
				break;
			}
		}
    		pid = waitpid(-1, &status, WNOHANG);
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
	if(strcmp(args[0], "jobs") == 0)
	{
		for(struct BackgroundTask* j = currjob; j != NULL; j = j->next)
		{
			printf("%d %d %s %s\n", j->id, j->pid, j->done ? "Done" : "Running", j->command);
		}
		cleanup_jobs();
		return true;
	}
	if(strcmp(args[0], "fg") == 0)
	{
		if(!args[1])
		{
			perror("fg");
			return true;
		}
		for(struct BackgroundTask* j = currjob; j != NULL; j = j->next)
		{
			if(j->id == atoi(args[1]))
			{
				int status;
				waitpid(j->pid, &status, WUNTRACED);
				remove_node(j->pid);
				return true;
			}
		}
		printf("Job id %d doesn't exist\n", atoi(args[1]));
		return true;
	}
	if(strcmp(args[0], "kill") == 0)
	{
		if(!args[1])
		{
			perror("kill");
			return true;
		}
		for(struct BackgroundTask* j = currjob; j != NULL; j = j->next)
		{
			if(j->id == atoi(args[1]))
			{
				remove_node(j->pid);
				return true;
			}
		}
		printf("Job id %d doesn't exist\n", atoi(args[1]));
		return true;
	}
	return launch(args);
}

void remove_node(pid_t pid)
{
	struct BackgroundTask* prev = NULL;
	struct BackgroundTask* curr = currjob;
	while(curr != NULL && curr->pid != pid)
	{
		prev = curr;
		curr = curr->next;
	}
	if(curr == NULL)
	{
		perror("remove");
		return;
	}
	if (prev == NULL)
	{
	    currjob = curr->next;
	}
	else
	{
	    prev->next = curr->next;
	}
	free(curr);

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
			backgroundid++;
			struct BackgroundTask* job = malloc(sizeof(struct BackgroundTask));
			job->pid = pid;
			job->done = false;
			job->command[0] = '\0';
			for (int i = 0; args[i] != NULL; i++)
			{
			    strcat(job->command, args[i]);
			    strcat(job->command, " ");
			}
			job->command[strlen(job->command) - 1] = '\0';
			job->id = backgroundid;
			job->next = currjob;
			currjob = job;
			printf("Background PID %d\n", pid);
		}
	}
	return true;
}



