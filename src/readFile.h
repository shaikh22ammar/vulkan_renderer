#include <stdio.h>
#ifndef READ_FILE_H
#define READ_FILE_H

typedef enum {
	READ_FILE_ERROR_OPEN = -3,
	READ_FILE_ERROR_END,
	READ_FILE_ERROR_READING,
	READ_FILE_SUCCESS = 0,
	READ_FILE_INCOMPLETE = 1
} ReadFileResult;

ReadFileResult readFile(const char *filepath, char *buffer, size_t *pBufSize);
/* Reads the file in filepath in binary format
 * If buffer is empty, records the size of the file in pBufSize
 * Otherwise, at most pBufSize characters from the file is recorded into buffer 
 * */

#endif
