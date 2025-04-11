#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>

/*
 Created by chat gpt from prompt:
    write a C function which creates a temp file, writes "hello" to the new file, renames the file, writes to it again, closes the file, and unlinks the file.
 */

void create_temp_file() {
    char temp_filename[] = "/tmp/tempfileXXXXXX";  // Template for mkstemp
    int fd = mkstemp(temp_filename);  // Create a unique temporary file
    
    if (fd == -1) {
        perror("mkstemp failed");
        exit(EXIT_FAILURE);
    }

    FILE *file = fdopen(fd, "w+");  // Open file stream from file descriptor
    if (!file) {
        perror("fdopen failed");
        close(fd);
        exit(EXIT_FAILURE);
    }

    // Write "hello" to the file
    fprintf(file, "hello\n");
    fflush(file);

    // New file name
    char new_filename[] = "/tmp/renamed_tempfile.txt";

    // Rename the file
    if (rename(temp_filename, new_filename) != 0) {
        perror("rename failed");
        fclose(file);
        exit(EXIT_FAILURE);
    }

    // Write more data to the renamed file
    fprintf(file, "world\n");
    fflush(file);

    // Close the file
    fclose(file);

    // Unlink (delete) the file
    if (unlink(new_filename) != 0) {
        perror("unlink failed");
        exit(EXIT_FAILURE);
    }

    printf("Temporary file created, modified, renamed, and deleted successfully.\n");

    // Now use openat explicitly
    const char *filename = "testfile.txt";
    
    int dirfd = open("/tmp", O_DIRECTORY);
    if (dirfd == -1) {
        perror("open directory");
        exit(EXIT_FAILURE);
    }

    fd = openat(dirfd, filename, O_CREAT | O_WRONLY, 0644);
    if (fd == -1) {
        perror("openat");
        close(dirfd);
        exit(EXIT_FAILURE);
    }

    write(fd, "Written to /tmp/testfile.txt\n", 28);
    close(fd);
    if (unlinkat(dirfd, filename, 0) == -1) {
        perror("unlinkat");
        close(dirfd);
        exit(EXIT_FAILURE);
    }
    close(dirfd);
}

int main() {
    create_temp_file();
    return 0;
}
