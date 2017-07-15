#include <stdio.h>
#include <dirent.h>
#include <string.h>
#include <malloc.h>
#define __USE_LARGEFILE64
#define _LARGEFILE_SOURCE
#define _LARGEFILE64_SOURCE
#include <sys/stat.h>
#include <limits.h>
#include <stdlib.h>

int main (int argc, char **argv)
{
    if (argc < 2)
    {
	fprintf(stderr, "Usage:\t%s <path>\n", argv[0]);
	return(-1);
    }
    char *abspath = (char *)malloc(sizeof(argv[1]));
    if(abspath == NULL)
	exit(-1);
    realpath(argv[1], abspath);
    if (NULL != abspath)
    {
	printf("%s\n", abspath);
	free(abspath);
    }
    exit(0);
}
