#include <sys/stat.h>
#include <unistd.h>

char* pkg_cache_folder = "__pkg_cache__";

int main(void) {
    (void) mkdir(pkg_cache_folder, S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH);
    return 0;
}
