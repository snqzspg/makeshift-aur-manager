#ifndef FILE_UTILS_H_INCLUDED
#define FILE_UTILS_H_INCLUDED

typedef struct {
	char*  content;
	size_t len;
} streamed_content_t;

extern streamed_content_t STR_CT_NULL_RET;

streamed_content_t stream_fd_content_alloc(int fd);
char* stream_fd_content_detach_str(streamed_content_t* s);
void stream_fd_content_dealloc(streamed_content_t* s);

int load_file_contents(char* __restrict__ dest, size_t limit, const char* path);

int file_exists(const char* fname);

/**
 * Checking if a file is text or binary according to the method
 * specified by GNU diff.
 * 
 * @param fd A unix file descriptor of the opened file.access
 * @return 0 for binary, 1 for text file.
 */
int is_text_file(int fd);

/**
 * A very Linux specific method to get all files for a directory.
 * It returns the files and folders in order of inodes.
 * 
 * @param dest Nullable, the output array of filenames to write to.
 * @param len_outs Nullable, an array of size_t where the lengths of the folder names are being written into.
 * @param is_dir_outs Nullable, an array of boolean values to be written to, which is true if the corresponding folder name is a directory.
 * @param dest_limit The size of the arrays. All arrays must have the same size. This is so that len_outs[0] is the length of dest[0], and is_dir_outs[0] tells if dest[0] is a directory.
 * @param folder The path to the folder to list the files.
 * @param recursive List all files inside the subfolders.
 * 
 * @return The number of entries inside this folder.
 */
size_t write_dir_filenames(char** dest, size_t* len_outs, char* is_dir_outs, size_t dest_limit, const char* folder, int recursive);

#endif // FILE_UTILS_H_INCLUDED
