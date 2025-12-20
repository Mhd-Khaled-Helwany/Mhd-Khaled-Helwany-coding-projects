#ifndef USERPROG_SYSCALL_H
#define USERPROG_SYSCALL_H

#include <stdbool.h>

void syscall_init(void);
bool create(const char *file, unsigned initial_size);   // Helper function for SYS_CREATE
int open (const char *file);                            // Helper function for SYS_OPEN
bool remove (const char *file_name);                    // Helper function for SYS_REMOVE
int file_size (int fd);                                 // Helper function for SYS_FILESIZE
unsigned tell (int fd);                                 // Helper function for SYS_TELL
int write (int fd, const void *buffer, unsigned size);  // Helper function for SYS_WRITE
int read (int fd, void *buffer, unsigned size);         // Helper function for SYS_READ
/* Helper function that validates pointers */
void validate_pointer(const void* ptr);
/* Helper function that validates strings */
void validate_string(const char* str);
/* Helper function that validates buffers */
void validate_buffer(const void* buffer, unsigned size);
#endif /* userprog/syscall.h */
