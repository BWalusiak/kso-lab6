// A program realizing custom filesystem\virtual disk stored in a file with the following options:
// --help: print the help
// --build-filesystem: build the filesystem from the file
// --remove-filesystem: remove the filesystem
// --list-files: list the files in the filesystem
// --put-file: copy the file from the current directory to the filesystem
// --get-file: copy the file from the filesystem to the current directory
// --delete-file: remove the file from the filesystem
// --print-map: print the map of the filesystem
// --print-memory: print the memory of the filesystem
//
// The disk is stored in a file with the following format:
// First MAX_FILES_NUMBER * sizeof(file_descriptor) bytes: the list of file_descriptor structures
// Next bytes: the file data
//
// The file_descriptor structure is:
// uint32_t file_size;
// uint32_t file_offset;
// uint32_t file_name_length;
// char file_name[file_name_length];
//
// After file is deleted, the file_descriptor structure is filled with zeros and all the following data is shifted to the left.


#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/mman.h>


#define MAX_FILENAME_LENGTH 32
#define MAX_FILES_NUMBER 10
#define MAX_FILE_SIZE (1024)
#define MAX_FILESYSTEM_SIZE (MAX_FILES_NUMBER * (MAX_FILE_SIZE + sizeof(struct file_descriptor)))
#define FILE_DATA_OFFSET (MAX_FILES_NUMBER * sizeof(struct file_descriptor))


struct file_descriptor {
    unsigned int file_size;
    unsigned int file_offset;
    char file_name[MAX_FILENAME_LENGTH];
};

void print_help() {
    printf("Usage: ./filesystem [options]\n");
    printf("Options:\n");
    printf("--help: print the help\n");
    printf("--build-filesystem <file>: build the filesystem from the file\n");
    printf("--remove-filesystem <file>: remove the filesystem\n");
    printf("--list-files <file>: list the files in the filesystem\n");
    printf("--put-file <filesystem> <file> <destination>: copy the file from the current directory to the filesystem\n");
    printf("--get-file <filesystem> <file> <destination>: copy the file from the filesystem to the current directory\n");
    printf("--delete-file <filesystem> <file>: remove the file from the filesystem\n");
    printf("--print-map <filesystem>: print the map of the filesystem\n");
}

void print_error(char *error_message) {
    printf("Error: %s\n", error_message);
}

void print_error_and_exit(char *error_message) {
    print_error(error_message);
    exit(1);
}

int main(int argc, char *argv[]) {
    if (strcmp(argv[1], "--help") == 0 || argc < 2) {
        print_help();
        return 0;
    }

    if (strcmp(argv[1], "--build-filesystem") == 0) {
        if (argc != 3) {
            print_error("--build-filesystem requires a file name");
            return 1;
        }

        int fd = open(argv[2], O_RDWR | O_CREAT | O_TRUNC, 0666);
        if (fd == -1) {
            print_error_and_exit("Cannot open file");
        }

        if (ftruncate(fd, MAX_FILESYSTEM_SIZE) == -1) {
            print_error_and_exit("Cannot truncate file");
        }

        void *filesystem = mmap(NULL, MAX_FILESYSTEM_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
        if (filesystem == MAP_FAILED) {
            print_error_and_exit("Cannot map file");
        }

        memset(filesystem, 0, MAX_FILESYSTEM_SIZE);

        close(fd);
        return 0;
    }

    if (strcmp(argv[1], "--remove-filesystem") == 0) {
        if (argc != 3) {
            print_error("--remove-filesystem requires a file name");
            return 1;
        }

        int fd = open(argv[2], O_RDWR);
        if (fd == -1) {
            print_error_and_exit("Cannot open file");
        }

        if (remove(argv[2]) == -1) {
            print_error_and_exit("Cannot remove file");
        }

        return 0;
    }

    if (strcmp(argv[1], "--list-files") == 0) {
        if (argc != 3) {
            print_error("--list-files requires a file name");
            return 1;
        }

        int fd = open(argv[2], O_RDWR);
        if (fd == -1) {
            print_error_and_exit("Cannot open file");
        }

        void *filesystem = mmap(NULL, MAX_FILESYSTEM_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
        if (filesystem == MAP_FAILED) {
            print_error_and_exit("Cannot map file");
        }

        struct file_descriptor *file_descriptors = (struct file_descriptor *) (filesystem);

        for (int i = 0; i < MAX_FILES_NUMBER; i++) {
            if (file_descriptors[i].file_size != 0) {
                printf("%d: %s\n",i, file_descriptors[i].file_name);
            }
        }

        close(fd);
        return 0;
    }

    if (strcmp(argv[1], "--put-file") == 0) {
        if (argc != 5) {
            print_error("--put-file requires a file name");
            return 1;
        }

        int fd = open(argv[2], O_RDWR);
        if (fd == -1) {
            print_error_and_exit("Cannot open file");
        }

        void *filesystem = mmap(NULL, MAX_FILESYSTEM_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
        if (filesystem == MAP_FAILED) {
            print_error_and_exit("Cannot map file");
        }

        struct file_descriptor *file_descriptors = (struct file_descriptor *) (filesystem);

        /* check if the file already exists */
        for (int i = 0; i < MAX_FILES_NUMBER; i++) {
            if (strcmp(file_descriptors[i].file_name, argv[4]) == 0) {
                print_error_and_exit("File already exists");
            }
        }

        struct file_descriptor *file_descriptor = NULL;


        int offset = 0;

        for (int i = 0; i < MAX_FILES_NUMBER; i++) {
            offset += file_descriptors[i].file_size;
            if (file_descriptors[i].file_size == 0) {
                file_descriptor = &file_descriptors[i];
                break;
            }
        }

        if (file_descriptor == NULL) {
            print_error_and_exit("No free space");
        }

        int file_fd = open(argv[3], O_RDONLY);
        if (file_fd == -1) {
            print_error_and_exit("Cannot open file");
        }

        struct stat file_stat;
        if (fstat(file_fd, &file_stat) == -1) {
            print_error_and_exit("Cannot get file size");
        }

        if (file_stat.st_size > MAX_FILE_SIZE) {
            print_error_and_exit("File is too big");
        }

        void *file_data = mmap(NULL, file_stat.st_size, PROT_READ, MAP_SHARED, file_fd, 0);
        if (file_data == MAP_FAILED) {
            print_error_and_exit("Cannot map file");
        }

        strcpy(file_descriptor->file_name, argv[4]);
        file_descriptor->file_size = file_stat.st_size;
        file_descriptor->file_offset = offset;

        memcpy(filesystem + FILE_DATA_OFFSET + file_descriptor->file_offset, file_data, file_stat.st_size);

        close(file_fd);
        close(fd);
        return 0;
    }


    if (strcmp(argv[1], "--get-file") == 0) {
        if (argc != 5) {
            print_error("--get-file requires a file name");
            return 1;
        }

        int fd = open(argv[2], O_RDWR);
        if (fd == -1) {
            print_error_and_exit("Cannot open file");
        }

        void *filesystem = mmap(NULL, MAX_FILESYSTEM_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
        if (filesystem == MAP_FAILED) {
            print_error_and_exit("Cannot map file");
        }

        struct file_descriptor *file_descriptors = (struct file_descriptor *) (filesystem);
        struct file_descriptor *file_descriptor = NULL;
        for (int i = 0; i < MAX_FILES_NUMBER; i++) {
            if (strcmp(file_descriptors[i].file_name, argv[3]) == 0) {
                file_descriptor = &file_descriptors[i];
                break;
            }
        }

        if (file_descriptor == NULL) {
            print_error_and_exit("File not found");
        }

        int file_fd = open(argv[4], O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);
        if (file_fd == -1) {
            print_error_and_exit("Cannot open file");
        }


        posix_fallocate(file_fd, 0, file_descriptor->file_size);

        char *file_data = mmap(NULL, file_descriptor->file_size, PROT_WRITE, MAP_SHARED, file_fd, 0);
        if (file_data == MAP_FAILED) {
            print_error_and_exit("Cannot map file");
        }

        memcpy(file_data, filesystem + FILE_DATA_OFFSET + file_descriptor->file_offset, file_descriptor->file_size);

        close(file_fd);
        close(fd);
        return 0;
    }

    if (strcmp(argv[1], "--delete-file") == 0) {
        if (argc != 4) {
            print_error("--delete-file requires a file name");
            return 1;
        }

        int fd = open(argv[2], O_RDWR);
        if (fd == -1) {
            print_error_and_exit("Cannot open file");
        }

        void *filesystem = mmap(NULL, MAX_FILESYSTEM_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
        if (filesystem == MAP_FAILED) {
            print_error_and_exit("Cannot map file");
        }

        struct file_descriptor *file_descriptors = (struct file_descriptor *) (filesystem);
        struct file_descriptor *file_descriptor = NULL;
        int i = 0;
        for (; i < MAX_FILES_NUMBER; i++) {
            if (strcmp(file_descriptors[i].file_name, argv[3]) == 0) {
                file_descriptor = &file_descriptors[i];
                break;
            }
        }

        if (file_descriptor == NULL) {
            print_error_and_exit("File not found");
        }


        unsigned int offset = file_descriptor->file_offset;
        unsigned int fileSize = file_descriptor->file_size;

        // delete file data
        memset(filesystem + FILE_DATA_OFFSET + offset, 0, fileSize);
        // move all other file data to the left
        memmove(filesystem + FILE_DATA_OFFSET + offset,
                filesystem + FILE_DATA_OFFSET + offset + fileSize,
                MAX_FILESYSTEM_SIZE - (FILE_DATA_OFFSET + offset + fileSize));

        // delete file descriptor
        memset(file_descriptor, 0, sizeof(struct file_descriptor));

        // move all other file descriptors to the left
        memmove(file_descriptors + i, file_descriptors + i + 1,
                (MAX_FILES_NUMBER - i - 1) * sizeof(struct file_descriptor));

        // delete last used descriptor
        memset(file_descriptors + MAX_FILES_NUMBER - 1, 0, sizeof(struct file_descriptor));


        // update file descriptors
        for (int j = i; j < MAX_FILES_NUMBER ; j++) {
            if (file_descriptors[j].file_offset > 0) {
                file_descriptors[j].file_offset -= fileSize;
            } else {
                break;
            }
        }

        close(fd);
        return 0;
    }

    if (strcmp(argv[1], "--print-map") == 0) {
        if (argc != 3) {
            print_error("--print-map requires a file name");
            return 1;
        }

        int fd = open(argv[2], O_RDWR);
        if (fd == -1) {
            print_error_and_exit("Cannot open file");
        }

        void *filesystem = mmap(NULL, MAX_FILESYSTEM_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
        if (filesystem == MAP_FAILED) {
            print_error_and_exit("Cannot map file");
        }

        // print file descriptors
        struct file_descriptor *file_descriptors = (struct file_descriptor *) (filesystem);
        for (int i = 0; i < MAX_FILES_NUMBER; i++) {
            printf("%d: %s, %d, %d\n", i, file_descriptors[i].file_name, file_descriptors[i].file_offset, file_descriptors[i].file_size);
        }

        close(fd);
        return 0;
    }

    if (strcmp(argv[1], "--print-memory") == 0) {
        if (argc != 3) {
            print_error("--print-map requires a file name");
            return 1;
        }

        int fd = open(argv[2], O_RDWR);
        if (fd == -1) {
            print_error_and_exit("Cannot open file");
        }

        void *filesystem = mmap(NULL, MAX_FILESYSTEM_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
        if (filesystem == MAP_FAILED) {
            print_error_and_exit("Cannot map file");
        }

        struct file_descriptor *file_descriptors = (struct file_descriptor *) (filesystem);

        // print all addresses and data
        printf("File descriptors:\n");
        for (int i = 0; i < MAX_FILESYSTEM_SIZE; i+=8) {
            if ( i == FILE_DATA_OFFSET) {
                printf("\nFile data:\n");
            }
            printf("%p: ", i);
            for (int j = 0; j < 8; j++) {
                printf("%02x ", ((unsigned char *)filesystem)[i + j]);
            }
            printf("\n");
        }

        close(fd);
        return 0;
    }
}
