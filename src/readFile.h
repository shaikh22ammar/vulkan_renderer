#include <stdio.h>
#ifndef READ_FILE_H
#define READ_FILE_H

enum ReadFileResult {
	READ_FILE_ERROR_OPEN = -3,
	READ_FILE_ERROR_END,
	READ_FILE_ERROR_READING,
	READ_FILE_SUCCESS = 0,
	READ_FILE_INCOMPLETE = 1
};

enum ReadFileResult readFile(char *filepath, char *buffer, size_t *pBufSize);

#endif
