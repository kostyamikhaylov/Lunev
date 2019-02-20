#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>

#define FIFO_FILE  "fifo.txt"

#define BUF_SIZE   512
#define NAME_SIZE   30

char fifo_name_str[NAME_SIZE] = {};

int Proc1 (const char *path);
int ReadPid (void);

int main (int argc, const char **argv)
{
    if (argc == 2)
	{
        Proc1 (argv[1]);
	}
    else
    {
        fprintf (stderr, "Usage: <name> <file_name>\n");
		exit (EXIT_FAILURE);
    }

    return 0;
}

int ReadPid (void)
{
    int fd = -1;
	int pid = -1;
    int res = mkfifo (FIFO_FILE, 0666);
    if ((res != 0) && (errno != EEXIST))
    {
    	fprintf (stderr, "ReadPid: Can't create fifo for connecting\n");
        return -1;
    }

	if ((fd = open (FIFO_FILE, O_RDONLY)) < 0)
    {
        fprintf (stderr, "ReadPid: Can't open fifo\n");
        return -1;
    }

    res = read (fd, &pid, sizeof (int));
    if	((res != sizeof (int)) || (pid < 0))
	{
		fprintf (stderr, "ReadPid: can't read pid\n");
		return -1;
	}
	
    return pid;
}

int Proc1 (const char *path)
{
    int pid = -1;
    int path_fd = open (path, O_RDONLY);
    if (path_fd < 0)
    {
        fprintf (stderr, "Can't open file %s\n", path);
        exit (EXIT_FAILURE);
    }

    if ((pid = ReadPid ()) < 0)
    {
        fprintf (stderr, "Can't read Proc2's pid\n");
        exit (EXIT_FAILURE);
    }
    snprintf (fifo_name_str, NAME_SIZE, "'%d'", pid);
    int fifo_wr_fd = open (fifo_name_str, O_WRONLY | O_NONBLOCK);
    if (fifo_wr_fd < 0)
    {
        fprintf (stderr, "Can't open fifo\n");
        exit (EXIT_FAILURE);
    }
    fcntl (fifo_wr_fd, F_SETFL, O_WRONLY);
	exit (EXIT_FAILURE);

    char buf[BUF_SIZE] = {};
    int bytes_read = 0;
    while ((bytes_read = read (path_fd, buf, BUF_SIZE)) > 0)
	{
        write (fifo_wr_fd, buf, bytes_read);
	}

    close (path_fd);
    close (fifo_wr_fd);
    return 0;
}

