#include "readFile.h"

enum ReadFileResult readFile(char *filepath, char *buffer, size_t *pBufSize) {
	/* Reads the file in filepath in binary format
	 * If buffer is empty, records the size of the file in pBufSize
	 * Otherwise, at most pBufSize characters from the file is recorded into buffer 
	 * */
	enum ReadFileResult result = READ_FILE_SUCCESS;
	FILE *file = fopen(filepath, "rb");
	if (!file) {
		// error in opening file
		result = READ_FILE_ERROR_OPEN;
		goto cleanup;
	}
	if (fseek(file, 0L, SEEK_END)) {
		// error in finding end of file
		result = READ_FILE_ERROR_END;
		goto cleanup;
	}
	long fileSize = ftell(file);
	if (fileSize == -1L) {
		// error in finding position of EOF
		result = READ_FILE_ERROR_END;
		goto cleanup;
	}
	if (!buffer) {
		// If buffer is empty, only record the size of file in numChars
		*pBufSize = (size_t) fileSize;
		result = READ_FILE_SUCCESS;
		goto cleanup;
	}
	if ((size_t) fileSize > *pBufSize) {
		// size of file exceeds the size of buffer
		// file will be read incompletely
		result = READ_FILE_INCOMPLETE;
	}
	// begin reading file
	rewind(file);
	size_t charsRead = fread(buffer, sizeof(char), *pBufSize, file);
	// Number of chars read must equal that of size of min(fileSize, bufSize) 
	size_t minSize = ((size_t) fileSize < *pBufSize) ? (size_t) fileSize : *pBufSize;
	if (charsRead < minSize) result = READ_FILE_ERROR_READING;
cleanup:
	if (file) fclose(file);
	return result;
}
