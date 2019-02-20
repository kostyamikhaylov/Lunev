#include "dict.h"
#include <ctype.h>
#include <string.h>

#define MAX_DICT_LEN 1000
#define MAX_WORD_LEN 20


struct dict
{
	struct word *words;
	int dict_len;
};

struct word
{
	char *w;
	int freq;
};

static int d_index = 0;

struct dict *creat_dict (int fd);
char *read_word (int fd);
int find_word (char *word, struct dict *d);
int add_freq (char *word, struct dict *d);
int add_word (char *word, struct dict *d, int offset);

int Destroy (struct obj_fn *obj);
int GetFrequency (struct obj_fn *obj, char *word);
int PrintDict (struct obj_fn *obj);
int AddWord (struct obj_fn *obj, char *word);

struct obj_fn *Constructor (int fd)
{
	struct obj_fn *new_obj = (struct obj_fn *) calloc (1, sizeof (*new_obj) + sizeof (struct dict));
	new_obj->Print = PrintDict;
	new_obj->Destructor = Destroy;
	new_obj->Frequency = GetFrequency;
	new_obj->Add = AddWord;
	struct dict *d =  creat_dict (fd);
	memcpy (new_obj + 1, d, sizeof (*d));
	free (d);

	return new_obj;
}

int Destroy (struct obj_fn *obj)
{
	struct dict *d = (struct dict *) (obj + 1);
	for (int i = 0; i < d->dict_len; i++)
		free (d->words[i].w);

	free (d->words);
	free (obj);

	return 0;
}

int GetFrequency (struct obj_fn *obj, char *word)
{
	struct dict *d = (struct dict *) (obj + 1);
	int i = 0;

	for (i = 0; i < d->dict_len; i++)
	{
		if (!strcmp (word, d->words[i].w))
			return d->words[i].freq;
	}

	return 0;
}

int PrintDict (struct obj_fn *obj)
{
	struct dict *d = (struct dict *) (obj + 1);
	for (int i = 0; i < d->dict_len; i++)
	{
		printf ("Word:\t%s\tfrequency\t%d\n", d->words[i].w, d->words[i].freq);
	}

	return 0;
}

int AddWord (struct obj_fn *obj, char *word)
{
	struct dict *d = (struct dict *) (obj + 1);
	if (find_word (word, d))
	{
		printf ("Word \"%s\" already exists\n", word);
		return 0;
	}

	if (d_index == MAX_DICT_LEN)
	{
		fprintf (stderr, "Can't add more words, exiting\n");
		return 1;
	}

	char *w = (char *) calloc (strlen (word) + 1, sizeof (*w));
	memcpy (w, word, strlen (word) + 1);

	d->words[d_index].w = w;
	d->words[d_index].freq = 1;

	d_index++;
	d->dict_len++;
	
	printf ("Word \"%s\" was successfully added\n", word);
	return 0;
}

struct dict *creat_dict (int fd)
{
	struct dict *d = NULL;
	char *w = NULL;

	d = (struct dict *) calloc (1, sizeof (*d));
	d->words = (struct word *) calloc (MAX_DICT_LEN, sizeof (*(d->words)));

	while ((w = read_word (fd)))
	{
		if (find_word (w, d))
		{
			add_freq (w, d);
			free (w);
		}
		else
		{
			if (d_index == MAX_DICT_LEN)
			{
				fprintf (stderr, "Maximum dictionary size reached, exiting");
				break;
			}
			
			add_word (w, d, d_index);
		
			d_index++;
			d->dict_len++;
		}
	}

	return d;
}

char *read_word (int fd)
{
	int bytes_read = -1, i = 0;
	char buf[MAX_WORD_LEN];
	char *word = NULL;

	do
	{
		bytes_read = read (fd, buf, 1);
		if (bytes_read <= 0)
			break;
		if (!isspace (*buf) && !ispunct (*buf))
		{
			i++;
			break;
		}
	} while (1);
	
	for ( ; i < MAX_WORD_LEN - 1; i++)
	{
		bytes_read = read (fd, buf + i, 1);
		if (bytes_read <= 0)
			break;
		if (isspace (buf[i]) || ispunct (buf[i]))
			break;
	}
	buf[i] = '\0';

	if (i == 0)
		return NULL;

	word = (char *) calloc (i + 1, sizeof (*word));
	memcpy (word, buf, i + 1);

	return word;
}

int find_word (char *word, struct dict *d)
{
	for (int i = 0; i < d->dict_len; i++)
	{
		if (!strcmp (word, d->words[i].w))
			return 1;
	}

	return 0;
}

int add_freq (char *word, struct dict *d)
{
	for (int i = 0; i < d->dict_len; i++)
	{
		if (!strcmp (word, d->words[i].w))
		{
			d->words[i].freq++;
			return 0;
		}
	}

	return 1;
}

int add_word (char *word, struct dict *d, int offset)
{
	d->words[offset].freq = 1;
	d->words[offset].w = word;
	return 0;
}

