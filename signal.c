#include <stdio.h>
#include <stdlib.h>

#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>

#define ALRM_TIME 1
#define BUF_SIZE 512


int bit = 0;
int got_usr_sig = 0;
int got_poll_sig = 0;

int buf_pos = 0;
char buf[BUF_SIZE];

void SigUsr1 (int signal);
void SigUsr2 (int signal);
void SigChld (int signal);
void SigAlrm (int signal);

int InitSignals (void);
int GetAnswer (void);
int ChildProc (pid_t ppid, int fd);
int WaitData (void);
int ParentProc (pid_t cpid);

int main (int argc, char **argv)
{
	int fd = -1;
	pid_t cpid = -1;
	pid_t ppid = getpid ();

	if (argc != 2)
	{
		fprintf (stderr, "Usage: <name>  <filename>\n");
		exit (EXIT_FAILURE);
	}

	if ((fd = open (argv[1], O_RDONLY)) < 0)
	{
		perror ("Can't open file");
		exit (EXIT_FAILURE);
	}

	InitSignals ();

	switch (cpid = fork ())
	{
		case -1:
			fprintf (stderr, "Can't fork process\n");
			exit (EXIT_FAILURE);
			break;

		case 0:
			ChildProc (ppid, fd);
			break;

		default:
			close (fd);
			ParentProc (cpid);
			break;
	}

	return 0;
}

int InitSignals (void)
{
	struct sigaction act;
	sigset_t set;

	act.sa_handler = SigUsr1;
	sigaction (SIGUSR1, &act, NULL);
	
	act.sa_handler = SigUsr2;
	sigaction (SIGUSR2, &act, NULL);
	
	act.sa_handler = SigChld;
	sigaction (SIGCHLD, &act, NULL);

	act.sa_handler = SigAlrm;
	sigaction (SIGALRM, &act, NULL);
	
	sigemptyset (&set);
	sigaddset (&set, SIGUSR1);
	sigaddset (&set, SIGUSR2);
	
	sigprocmask (SIG_BLOCK, &set, NULL);

	return 0;	
}
				
int GetAnswer (void)
{
	sigset_t set;
	sigemptyset (&set);

	while (bit == 0)
	{
		alarm (ALRM_TIME);
		sigsuspend (&set);
		alarm (0);
	}

	bit = 0;

	return 0;
}

int ChildProc (pid_t ppid, int fd)
{
	int bytes_read = -1;
	int i = 0;
	int mask = 128;
	char cur_byte;

	do
	{
		bytes_read = read (fd, buf, BUF_SIZE);
		if (bytes_read < 0)
		{
			perror ("read");
			exit (EXIT_FAILURE);
		}

		for (buf_pos = 0; buf_pos < bytes_read; buf_pos++)
		{
			if (-1 == kill (ppid, SIGUSR1))
			{
				perror ("kill");
				exit (EXIT_FAILURE);
			}

			GetAnswer ();
			
			cur_byte = buf[buf_pos];
			mask = 128;

			for (i = 0; i < 8; i++)
			{
				if (-1 == kill (ppid, ((cur_byte & (mask >> i)) == 0) ? SIGUSR1 : SIGUSR2))
				{
					perror ("kill");
					exit (EXIT_FAILURE);
				}

				GetAnswer ();
			}
		}
	} while (bytes_read > 0);
	
	close (fd);
	if (-1 == kill (ppid, SIGUSR2))
	{
		perror ("kill");
		exit (EXIT_FAILURE);

	}
	GetAnswer ();
	
	exit (EXIT_SUCCESS);
}

int WaitData (void)
{
	sigset_t set;
	sigemptyset (&set);

	while (got_usr_sig == 0)
	{
		alarm (ALRM_TIME);
		sigsuspend (&set);
		alarm (0);
	}

	got_usr_sig = 0;

	return 0;
}


int ParentProc (pid_t cpid)
{
	int i = 0;
	char cur_byte;
	
	buf_pos = 0;
	
	while (1)
	{
		WaitData ();
	
		if (i == 0)
		{
			if (-1 == kill (cpid, SIGUSR2))
			{
				perror ("kill");
				exit (EXIT_FAILURE);
			}
			
			if (bit == 1)
			{
				siginterrupt (SIGCHLD, 0);
				if (-1 == write (STDOUT_FILENO, buf, buf_pos))
				{
					perror ("write");
					exit (EXIT_FAILURE);
				}
				exit (EXIT_SUCCESS);
			}
			
			WaitData ();
		}

		i++;

		cur_byte = (cur_byte << 1) + bit;

		if (i == 8)
		{
			i = 0;
			buf[buf_pos] = cur_byte;
			buf_pos++;

			if (buf_pos == BUF_SIZE)
			{
				if (-1 == write (STDOUT_FILENO, buf, buf_pos))
				{
					perror ("write");
					exit (EXIT_FAILURE);
				}
				buf_pos = 0;
			}
		}
		
		if (-1 == kill (cpid, SIGUSR2))
		{
			perror ("kill");
			exit (EXIT_FAILURE);
		}
	}
}

void SigUsr1 (int signal)
{
	got_usr_sig = 1;
	bit = 0;
}

void SigUsr2 (int signal)
{
	got_usr_sig = 1;
	bit = 1;
}

void SigAlrm (int signal)
{
	fprintf (stderr, "No response from the other process\n");
	exit (EXIT_FAILURE);
}

void SigChld (int signal)
{
	if (!bit)
	{
		printf ("Child died\n");
		exit (EXIT_FAILURE);
	}
}

