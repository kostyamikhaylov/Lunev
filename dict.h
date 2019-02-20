#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>


struct dict;
struct word;

struct obj_fn
{
	int (*Print) (struct obj_fn *obj);
	int (*Destructor) (struct obj_fn *obj);
	int (*Frequency) (struct obj_fn *obj, char *word);
	int (*Add) (struct obj_fn *obj, char *word);
};

struct obj_fn *Constructor (int fd);

