#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <string.h>

#define STR_SIZE 20


struct Msg
{
	long int mtype;
	int number[1];
} msg1;

long int GetInt (char **argv)
{
    char* endptr = NULL;
    long int number = strtol(argv[0], &endptr, 0);
    if (number == 0)
	{
        fprintf (stderr, "Inappropriate Symbol!\n");
        return -1;
    }
    else if (*endptr != 0)
	{
        fprintf (stderr, "Inappropriate Symbol in the end - [%c]\n", *endptr);
        return -1;
    }
    else if (errno == ERANGE)
	{
        fprintf (stderr, "Number out of range!\n");
        return -1;
    }
    else if (number < 0)
	{
        fprintf (stderr, "Number is below ZERO!!!");
        return -1;
    }
    else
	{
        return number;
    }
}

int main (int argc, char *argv[])
{
	long int n = 0;
	int pid = 0;
	int id = 0;
	int key = 1234;
	int size = 0;
	int i = 0, k = 0;
	int *pids = NULL;
	
	if (argc != 2)
	{
		fprintf (stderr, "Usage: <name> n\n");
		exit (EXIT_FAILURE);
	}

	if ((n = GetInt (argv + 1)) < 0)
	{
		fprintf (stderr, "Wrong number n: %s\n", argv[1]);
		exit (EXIT_FAILURE);
	}

	pids = (int *) calloc (n, sizeof (*pids));

	if ((id = msgget (IPC_PRIVATE, IPC_CREAT | 0666)) < 0)
	{
		fprintf (stderr, "Can't get a message queue identifier\n");
		exit (EXIT_FAILURE);
	}

//	printf ("id = %d\n", id);
	
	
	for (i = 0; i < n; i++)
	{
		pid = fork ();
		if ((pid == 0))
		{
			struct Msg msg;
			msgrcv (id, &msg, sizeof (msg.number), i + 1, 0);
//			printf ("Child №%d, message with type %ld received\n", i, msg.mtype);
			
//			printf ("%s ", strings[i]);
			printf ("%d ", i);
			fflush (stdout);
			

			{
				struct Msg msg;
				msg.mtype = i + n + 1;
				msg.number[0] = i;		
				msgsnd (id, (void *) &msg, sizeof (msg.number), IPC_NOWAIT);
			}
			return 0;
		}
		else
		{
			pids[i] = pid;
			if (i == n - 1)
			{
//				printf ("Now n children are alive\n");
				for (k = 0; k < n; k++)
				{
					{
						struct Msg msg;
						msg.mtype = k + 1;
						msg.number[0] = k;		
						msgsnd (id, (void *) &msg, sizeof (msg.number), IPC_NOWAIT);
					}
					{
						struct Msg msg;
						size = msgrcv (id, &msg, sizeof (msg.number), n + k + 1, 0);
//						printf ("Confirmation of printing with type %ld received\n", msg.mtype);
					}
				}
				for (k = 0; k < n; k++)
					waitpid (pids[k], NULL, 0);
				break;
			}
		}
	}
			

	msgctl (id, IPC_RMID, NULL);
	
	free (pids);
	
	return 0;
}

