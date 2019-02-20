#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/shm.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#define FILE_5IPC "file.txt"
#define PROJ_ID 1
#define DATA_SIZE 512
#define SHM_SIZE (sizeof (int) + DATA_SIZE)

enum sems
{
	WAIT_PRODUCER,
	WAIT_CONSUMER,
	OCCUPY_PRODUCER_POSITION,
	OCCUPY_CONSUMER_POSITION,
	MUTEX,
	IS_FULL,
	SEM_NUM,
};

struct sembuf *sem_buf = NULL;
int cur_elem = 0;

int producer (int sem_id, void *shm, char *filepath);

int main (int argc, char **argv)
{
	int key = -1;
	int sem_id = -1;
	int shm_id = -1;
	void *shm = NULL;
	
	key = ftok (FILE_5IPC, PROJ_ID);
	if (key < 0)
	{
		fprintf (stderr, "Can't generate System V IPC key with file %s\n", FILE_5IPC);
		exit (EXIT_FAILURE);
	}

	sem_id = semget (key, SEM_NUM, IPC_CREAT | 0666);
	if (sem_id < 0)
	{
		fprintf (stderr, "Can't get semaphore identificator\n");
		exit (EXIT_FAILURE);
	}

	shm_id = shmget (key, SHM_SIZE, IPC_CREAT | 0666);
	if (shm_id < 0)
	{
		fprintf (stderr, "Can't get shared memory identificator\n");
		exit (EXIT_FAILURE);
	}

	shm = shmat (shm_id, NULL, 0);
	if (shm == (void *) (-1))
	{
		fprintf (stderr, "Can't attach shared memory\n");
		exit (EXIT_FAILURE);
	}
	
	if (argc == 2)
	{
		producer (sem_id, shm, argv[1]);
	}
	else
	{
		fprintf (stderr, "Usage: <name> <file_name>\n");
		exit (EXIT_FAILURE);
	}

	free (sem_buf);
	return 0;
}
		
int PutElemToSemBuf (short int semnum, short int semop, short int semflg)
{
	if (!sem_buf)
		sem_buf = (struct sembuf *) calloc (SEM_NUM, sizeof (*sem_buf));
	
	sem_buf[cur_elem].sem_num = semnum;
	sem_buf[cur_elem].sem_op = semop;
	sem_buf[cur_elem].sem_flg = semflg;
	cur_elem++;

	return 0;
}

int SendSems (int sem_id)
{
	int res = 0;
	int sem_number = cur_elem;

	cur_elem = 0;
	res = semop (sem_id, sem_buf, sem_number);
	
	return res;
}


int producer (int sem_id, void *shm, char *filepath)
{
	int fd = -1;
	int bytes_read = -1, res = -1;
	char buf[DATA_SIZE] = {};
	int *shm_int = (int *) shm;

	fd = open (filepath, O_RDONLY);
	if (fd < 0)
	{
		fprintf (stderr, "Can't open file \"%s\"\n", filepath);
		exit (EXIT_FAILURE);
	}

	PutElemToSemBuf (WAIT_CONSUMER, 0, 0);
	PutElemToSemBuf (WAIT_CONSUMER, 1, SEM_UNDO);
	PutElemToSemBuf (OCCUPY_CONSUMER_POSITION, 0, 0);
	SendSems (sem_id);
	
	PutElemToSemBuf (WAIT_PRODUCER, -1, 0);
	PutElemToSemBuf (WAIT_PRODUCER, 1, 0);
	PutElemToSemBuf (OCCUPY_PRODUCER_POSITION, 1, SEM_UNDO);
	SendSems (sem_id);

	PutElemToSemBuf (IS_FULL, 1, SEM_UNDO);
	PutElemToSemBuf (IS_FULL, -1, 0);
	PutElemToSemBuf (MUTEX, 0, 0);
	PutElemToSemBuf (MUTEX, 1, SEM_UNDO);
	SendSems (sem_id);

	do
	{
		PutElemToSemBuf (WAIT_PRODUCER, -1, IPC_NOWAIT);
		PutElemToSemBuf (WAIT_PRODUCER, 1, 0);
		PutElemToSemBuf (IS_FULL, 0, 0);
		PutElemToSemBuf (MUTEX, -1, SEM_UNDO);
		res = SendSems (sem_id);

		if (res != 0)
		{
			fprintf (stderr, "Consumer doesn't work properly\n");
			exit (EXIT_FAILURE);
		}

		bytes_read = read (fd, buf, DATA_SIZE);
		*shm_int = bytes_read;
		memcpy ((char *) shm + sizeof (int), buf, bytes_read);
		
		PutElemToSemBuf (MUTEX, 1, SEM_UNDO);
		PutElemToSemBuf (IS_FULL, 1, 0);
		SendSems (sem_id);
	} while (bytes_read > 0);

	return 0;
}

