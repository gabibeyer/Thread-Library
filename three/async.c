#include <aio.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>

#include "scheduler.h"

/* attempts to read up to count bytes from file descriptor fd 
 * into the buffer starting at buf.
 */
ssize_t read_wrap(int fd, void *buf, size_t count) {
	// Create instance of aiocb and zero out control block buffer
	struct aiocb aiocb;
	memset(&aiocb, 0, sizeof(struct aiocb));	

	// lseek to find current position
	off_t offset = lseek(fd, 0, SEEK_CUR);
	if (offset != -1) {
		aiocb.aio_offset = offset;
	} else {
		aiocb.aio_offset = 0;
		printf("File cannot be seeked\n");
	}

	// Define fields in struct with read parameters
	aiocb.aio_fildes = fd;
	aiocb.aio_buf = buf;
	aiocb.aio_nbytes = count;

	//Perform read call
	aio_read(&aiocb);

	// Yield while request is complete
	while (aio_error(&aiocb) == EINPROGRESS) {
		yield();
	}

	// The if statement is probably not needed
	if (aio_error(&aiocb) != EINPROGRESS) {
		return aio_return(&aiocb);
	}
}
