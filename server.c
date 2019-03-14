#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <math.h>

#define SIZE_128K 131072
#define PARENT_BUFFER (1024 * pow (3, children_num - child_num) > SIZE_128K) ? SIZE_128K : 1024 * pow (3, children_num - child_num)
#define CHILD_BUFFER 4096

#define max(x, y) ((x) > (y)) ? (x) : (y)


struct Child
{
	int fd1[2];
	int fd2[2];
	int buf_size;
	char *buf;
	char *cur_pos;
	int nbytes;
	int closed;
};


int children_num = 0;

int Parent (struct Child *arr);
void Child (int read_fd, int write_fd, int num);

int main (int argc, char **argv)
{
	struct Child *children = NULL;
	int flags = 0;
	int child_num = 0;
	int file_fd = 0, rd_fd = 0, wr_fd = 0;

	if (argc != 3)
	{
		fprintf (stderr, "Usage: <name>  <number_of_children>  <file_name>\n");
		exit (EXIT_FAILURE);
	}

	if ((children_num = strtol (argv[1], NULL, 10)) < 0)
	{
		fprintf (stderr, "Wrong children number: %s\n", argv[1]);
		exit (EXIT_FAILURE);
	}

	if ((file_fd = open (argv[2], O_RDONLY)) < 0)
	{
		fprintf (stderr, "Can't open file %s\n", argv[2]);
		exit (EXIT_FAILURE);
	}
	
	if (!(children = (struct Child *) calloc (children_num + 1, sizeof (*children))))
	{
		fprintf (stderr, "Memory allocation error\n");
		exit (EXIT_FAILURE);
	}

	for (child_num = 0; child_num < children_num + 1; child_num++)
	{
		if (pipe (children[child_num].fd1) < 0 || pipe (children[child_num].fd2) < 0)
		{
			fprintf (stderr, "Can't create pipe №%d\n", child_num);
			exit (EXIT_FAILURE);
		}
		children[child_num].buf_size = (int) (PARENT_BUFFER);
		if (!(children[child_num].buf = (char *) calloc ((int) (PARENT_BUFFER), sizeof (char))))
		{
			fprintf (stderr, "Memory allocation error for buffer №%d\n", child_num);
			exit (EXIT_FAILURE);
		}
		children[child_num].cur_pos = children[child_num].buf;
	}
	
	for (child_num = 1; child_num <= children_num; child_num++)
	{
		switch (fork ())
		{
			case -1:
				fprintf (stderr, "Can't create child №%d\n", child_num);
				exit (EXIT_FAILURE);
				break;

			case 0:
				for (int i = 0; i < children_num + 1; i++)
				{
					free (children[i].buf);
					if (i == child_num)
						continue;

					close (children[i].fd1[0]);
					close (children[i].fd1[1]);
					close (children[i].fd2[0]);
					close (children[i].fd2[1]);
				}
				close (children[child_num].fd1[1]);
				close (children[child_num].fd2[0]);
				
				rd_fd = children[child_num].fd1[0];
				wr_fd = children[child_num].fd2[1];
				
				free (children);
				
				Child (rd_fd, wr_fd, child_num);

				break;

			default:
				flags = fcntl (children[child_num].fd1[1], F_GETFL, 0);
				flags |= O_NONBLOCK;
				fcntl (children[child_num].fd1[1], F_SETFL, flags);
				flags = fcntl (children[child_num].fd2[0], F_GETFL, 0);
				flags |= O_NONBLOCK;
				fcntl (children[child_num].fd2[0], F_SETFL, flags);
				
				close (children[child_num].fd1[0]);
				close (children[child_num].fd2[1]);
				break;
		}
	}

	close (children[0].fd1[1]);
	dup2 (file_fd, children[0].fd2[0]);
	close (file_fd);

	return Parent (children);
}

int Parent (struct Child *arr)
{
	int nbytes = -1;
	
	while (1)
	{
		fd_set set_rd, set_wr;
		FD_ZERO (&set_rd);
		FD_ZERO (&set_wr);
		int nfds = -1;
		
		for (int i = 0; i < children_num + 1; i++)
		{
			if (!(arr[i].closed) && (arr[i].nbytes == 0))
			{
				FD_SET (arr[i].fd2[0], &set_rd);
				nfds = max (nfds, arr[i].fd2[0]);
			}
		}

		for (int i = 1; i < children_num + 1; i++)
		{
			if (arr[i - 1].nbytes != 0)
			{
				FD_SET (arr[i].fd1[1], &set_wr);
				nfds = max (nfds, arr[i].fd1[1]);
			}
		}

		if (nfds == -1)
			break;

		if (-1 == select (nfds + 1, &set_rd, &set_wr, NULL, NULL))
		{
			perror ("select");
			return 1;
		}

		for (int i = 0; i < children_num + 1; i++)
		{
			if (FD_ISSET (arr[i].fd2[0], &set_rd))
			{
				nbytes = read (arr[i].fd2[0], arr[i].buf, arr[i].buf_size);
				if (nbytes == -1)
				{
					perror ("read");
					return -1;
				}
				else if (nbytes == 0)
				{
					close (arr[i].fd2[0]);
					if (i != children_num)
						close (arr[i + 1].fd1[1]);
					arr[i].closed = 1;
				}

				arr[i].nbytes += nbytes;

				if (i == children_num && nbytes != 0)
				{
					nbytes = write (STDOUT_FILENO, arr[i].buf, arr[i].nbytes);
					if (nbytes == -1)
					{
						perror ("write to stdout");
						return -1;
					}
					arr[i].nbytes -= nbytes;
				}
			}
		}
		
		for (int i = 1; i < children_num + 1; i++)
		{
			if (FD_ISSET (arr[i].fd1[1], &set_wr))
			{
				nbytes = write (arr[i].fd1[1], arr[i - 1].cur_pos, arr[i - 1].nbytes);
				if (nbytes == -1)
				{
					perror ("write");
					return -1;
				}
				
				arr[i - 1].nbytes -= nbytes;
				arr[i - 1].cur_pos += nbytes;

				if (arr[i - 1].nbytes == 0)
					arr[i - 1].cur_pos = arr[i - 1].buf;
			}
		}
	}	

	for (int i = 0; i < children_num + 1; i++)
		free (arr[i].buf);
	free (arr);
	return 0;
}

void Child (int read_fd, int write_fd, int num)
{
	char buf[CHILD_BUFFER];
	int bytes_read = -1;

	while ((bytes_read = read (read_fd, buf, CHILD_BUFFER)) > 0)
	{
/*
		if (num == 3)
		{
			usleep(10);
		}
*/
		if ((bytes_read = write (write_fd, buf, bytes_read)) < 0)
		{
			perror ("write");
			exit (EXIT_FAILURE);
		}
	}

	if (bytes_read < 0)
	{
		perror ("read");
		exit (EXIT_FAILURE);
	}

	close (read_fd);
	close (write_fd);
	
	exit (EXIT_SUCCESS);
}

