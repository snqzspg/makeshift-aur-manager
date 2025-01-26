#ifndef FILE_UTILS_H_INCLUDED
#define FILE_UTILS_H_INCLUDED

typedef struct {
	char*  content;
	size_t len;
} streamed_content_t;

extern streamed_content_t STR_CT_NULL_RET;

streamed_content_t stream_fd_content_alloc(int fd);
void stream_fd_content_dealloc(streamed_content_t* s);

int load_file_contents(char* __restrict__ dest, size_t limit, const char* path);

int file_exists(const char* fname);

#endif // FILE_UTILS_H_INCLUDED
