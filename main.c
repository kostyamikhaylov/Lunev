#include "dict.h"
#include <fcntl.h>


int main (int argc, char **argv)
{
	int fd = -1;
	struct obj_fn *dict1;
	if (argc == 2)
	{
		if ((fd = open (argv[1], O_RDONLY)) < 0)
		{
			perror ("open");
			exit (EXIT_FAILURE);
		}
	}
	else if (argc == 1)
		fd = STDIN_FILENO;
	else
	{
		fprintf (stderr, "Usage: <name>  [filename]\n");
		exit (EXIT_FAILURE);
	}

	dict1 = Constructor (fd);

	dict1->Print (dict1);

	printf ("Frequency of word \"%s\" is %d\n", "word_example", dict1->Frequency (dict1, "word_example"));

	dict1->Add (dict1, "add_word");
	
	dict1->Print (dict1);

	dict1->Destructor (dict1);

	return 0;
}

