#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
//#include <poll.h>

#define FIFO_FILE "fifo.txt"

#define BUF_SIZE 512
#define NAME_SIZE 30

#define TIME_WAIT_MICROSEC 500

char fifo_name_str[NAME_SIZE] = {};
int fd = -1;

int Proc2 (void);
int WritePid (void);

int main ()
{
	Proc2 ();
    
	return 0;
}

int WritePid (void)
{
	int pid = getpid ();
    int res = mkfifo (FIFO_FILE, 0666);
    if ((res != 0) && (errno != EEXIST))
    {
    	fprintf (stderr, "WritePid: Can't create fifo for connecting\n");
        return -1;
    }

	if ((fd = open (FIFO_FILE, O_WRONLY)) < 0)
    {
        fprintf (stderr, "WritePid: Can't open fifo\n");
        return -1;
    }
    
    res = write (fd, &pid, sizeof (int));
	if (res != sizeof (int))
	{
        fprintf (stderr, "WritePid: Can't write pid to fifo\n");
        return -1;
	}

    return res;
}

int Proc2 ()
{
	int flag = 0;
	int bytes_available = -1;
	int pid = getpid ();
    snprintf (fifo_name_str, NAME_SIZE, "'%d'", pid);
    int res = mkfifo (fifo_name_str, 0666);
    if ((res != 0) && (errno != EEXIST))
    {
        fprintf (stderr, "Can't create fifo\n");
        exit (EXIT_FAILURE);
    }

	int fifo_rd_fd = open (fifo_name_str, O_RDONLY | O_NONBLOCK);
	fcntl (fifo_rd_fd, F_SETFL, O_RDONLY);

	while (1)
	{
    	if (flag == 0)
		{
			if (WritePid () != sizeof (int))
    		{
        		fprintf (stderr, "Can't send pid\n");
        		exit (EXIT_FAILURE);
    		}
			flag = 1;
		}
		else
		{
			close (fd);
			if (open (FIFO_FILE, O_WRONLY) < 0)
    		{
        		fprintf (stderr, "Can't open fifo\n");
        		exit (EXIT_FAILURE);
    		}
		}

		usleep (TIME_WAIT_MICROSEC);
		ioctl (fifo_rd_fd, FIONREAD, &bytes_available);
	
		if (bytes_available < 0)
    	{
        	fprintf (stderr, "Time is out, no response from Proc1\n");
			unlink (fifo_name_str);
			exit (EXIT_FAILURE);
    	}
		else if (bytes_available == 0)
		{
			continue;
		}
		else
			break;
	}

    char buf[BUF_SIZE] = {};
    int bytes_read = 0;
    while ((bytes_read = read (fifo_rd_fd, buf, BUF_SIZE)) > 0)
       write (fileno (stdout), buf, bytes_read);

	close (fifo_rd_fd);
    unlink (fifo_name_str);
    return 0;
}

/*
	struct pollfd pfd = {fifo_rd_fd, POLLIN, 0};

	res = poll (&pfd, 1, TIME_WAIT_MILLISEC);
	if	((res <= 0) || (pfd.revents != POLLIN))
	{
       	fprintf (stderr, "Time is out, no response from Proc1\n");
		unlink (fifo_name_str);
		exit (EXIT_FAILURE);
	}
*/	
