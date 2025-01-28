#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <zlib.h>

#include "logger/logger.h"

#include "zlib_wrapper.h"

#define inflate_chunk_size 262144
/**
 * Using reference from https://www.zlib.net/zlib_how.html
 */
int zputs(const char* compressed_string, int s_size) {
	z_stream strm = {
		.zalloc   = Z_NULL,
		.zfree    = Z_NULL,
		.opaque   = Z_NULL,
		.avail_in = 0,
		.next_in  = Z_NULL
	};

	int ret = inflateInit2(&strm, MAX_WBITS + 32);
	if (ret != Z_OK) {
		error_printf(" Failure initialising the deflate algorithm.\n");
		return ret;
	}

	Bytef* in = (Bytef*) malloc((s_size + inflate_chunk_size) * sizeof(char));
	if (in == NULL) {
		(void) error_perror(" Inflating package names failed");
		(void) inflateEnd(&strm);
		return errno;
	}
	Bytef* out = in + s_size * sizeof(char);
	(void) memcpy(in, compressed_string, s_size * sizeof(char));

	strm.next_in  = in;
	strm.avail_in = s_size;

	do {
		strm.avail_out = inflate_chunk_size;
		strm.next_out  = out;

		ret = inflate(&strm, Z_NO_FLUSH);
		assert(ret != Z_STREAM_ERROR); // state not clobbered
		switch (ret) {
			case Z_NEED_DICT:
				ret = Z_DATA_ERROR;
			case Z_DATA_ERROR:
				(void) error_printf(" Inflating compressed string failed - string provided seems to not be a gz-compressed stream.\n");
				free(in);
				(void) inflateEnd(&strm);
				return ret;
			case Z_MEM_ERROR:
				(void) error_printf(" Inflating compressed string failed - gz could not allocate memory to inflate.\n");
				free(in);
				(void) inflateEnd(&strm);
				return ret;
		}

		int inflated_size = inflate_chunk_size - strm.avail_out;
		int written_size  = fwrite(out, sizeof(char), inflated_size, stdout);
		if (written_size < 0) {
			(void) error_perror(" Writing to stdout failed");
			free(in);
			(void) inflateEnd(&strm);
			return Z_ERRNO;
		}
	} while (strm.avail_out == 0);

	free(in);
	(void) inflateEnd(&strm);
	return ret == Z_STREAM_END ? Z_OK : Z_DATA_ERROR;
}
