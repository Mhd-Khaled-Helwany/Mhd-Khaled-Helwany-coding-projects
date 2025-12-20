#include "userprog/syscall.h"

#include "threads/interrupt.h"
#include "threads/thread.h"

#include <stdio.h>
#include <syscall-nr.h>
#include "devices/timer.h"
#include "devices/shutdown.h"
#include "filesys/filesys.h"
#include "filesys/file.h"
#include "userprog/process.h"
#include "lib/kernel/stdio.h"
#include "threads/vaddr.h"
#include "userprog/pagedir.h"
#include "devices/input.h"

static void syscall_handler(struct intr_frame*);

void syscall_init(void)
{
	intr_register_int(0x30, 3, INTR_ON, syscall_handler, "syscall");
}

static void syscall_handler(struct intr_frame* f UNUSED)
{
	//printf("system call!\n");
	validate_buffer(f->esp,sizeof(int*));
	if(*(int *)f->esp < 0 || *(int *)f->esp >= SYS_NUMBER_OF_CALLS){
		thread_current() -> exit_status = -1;
		close_all_files();
		thread_exit();
	}
	int syscall_code = *(int *)f->esp;
	unsigned size = 0;										//Pre-initialize common arguments
	const char *name = NULL;
	int fd = -1;
	void *buffer = NULL;  
	switch (syscall_code){
		case SYS_HALT:
			shutdown_power_off();
			break;
		case SYS_EXIT:
			validate_buffer(f->esp + 4,8);
			struct thread *cur = thread_current();
			cur-> exit_status = *(int *)(f->esp + 4);
			close_all_files();
			thread_exit();
			break;
		case SYS_SLEEP:
			validate_buffer(f->esp + 4,4);
			int64_t milliseconds = *(int64_t *)(f->esp + 4);  // get number of milliseconds from esp
			timer_msleep(milliseconds);
			break;
		case SYS_CREATE:
			validate_buffer(f->esp + 4,8);
			validate_string(*(char **)(f->esp + 4));
			name = *(char **)(f->esp + 4);
			size = *(unsigned *)(f->esp + 8);
			f->eax = create(name,size);
			break;
		case SYS_REMOVE:
			validate_buffer(f->esp + 4,4);
			validate_string(*(char **)(f->esp + 4));
			name = *(char **)(f->esp + 4);
			f->eax = remove(name);
			break; 
		case SYS_OPEN:
			validate_buffer(f->esp + 4,4);
			validate_string(*(char **)(f->esp + 4));
			name = *(char **)(f->esp + 4);
			f->eax = open(name);
			break;
		case SYS_FILESIZE:
			validate_buffer(f->esp + 4,4);
			fd = *(int *)(f->esp + 4);
			f->eax = file_size(fd);
			break;
		case SYS_READ:
			validate_buffer(f->esp + 4,12);
			validate_buffer(*(void **)(f->esp + 8), *(unsigned *)(f->esp + 12));
			fd = *(int *)(f->esp + 4);
			buffer = *(void **)(f->esp + 8);
			size = *(unsigned *)(f->esp + 12);
			f->eax = read(fd, buffer, size);
			break;
		case SYS_WRITE:
			validate_buffer(f->esp,16);
			validate_buffer(*(void **)(f->esp + 8), *(unsigned *)(f->esp + 12));
			fd = *(int *)(f->esp + 4);
			buffer = *(void **)(f->esp + 8);
			size = *(unsigned *)(f->esp + 12);
			f->eax = write(fd, buffer, size);
			break;
		case SYS_SEEK:
			validate_buffer(f->esp + 4,8);
			fd = *(int *)(f->esp + 4);	
			off_t position = *(off_t *)(f->esp + 8);
			if (position > file_size(fd)){ // position exceeds size
				break;
			}
			file_seek(file_from_fd(fd), position);
			break;
		case SYS_TELL:
			validate_buffer(f->esp + 4,4);
			fd = *(int *)(f->esp + 4);
			f->eax = tell(fd);
			break;
		case SYS_CLOSE:
			validate_pointer(f->esp + 4);
			validate_buffer(f->esp + 4,4);
			fd = *(int *)(f->esp + 4);
			file_close(file_from_fd(fd));
			break;
		case SYS_EXEC:
			validate_buffer(f->esp + 4,4);
			validate_string(*(char **)(f->esp + 4));
			//validate_pointer(*(char **)(f->esp + 4));
			
			name = *(char **)(f->esp + 4);
			f->eax = process_execute(name);
			break;
		case SYS_WAIT:
			validate_buffer(f->esp + 4,4);
			tid_t pid = *(tid_t *)(f->esp + 4);
    		f->eax = process_wait(pid);
			break;
		default:
			//printf("Unhandled syscall: %d\n", syscall_code);
			thread_current()-> exit_status = -1;
            thread_exit();
	}
}

bool create(const char *file, unsigned initial_size){
	bool success = filesys_create(file, initial_size);
	return success;
}

int open (const char *file){
	struct file *f = filesys_open(file);
	if (!f){
		file_close(f);
		return -1;
	}
	return f->fd;
}

bool remove (const char *file_name){
	bool success = filesys_remove(file_name);
	return success;
}

int file_size (int fd){
	struct file *file = file_from_fd(fd);
	return file_length(file);
}

unsigned tell (int fd){
	struct file *file = file_from_fd(fd);
	return file_tell(file);
}

int write (int fd, const void *buffer, unsigned size){
	if (fd == 1) { // Output file descriptor
        putbuf(buffer, size);
        return size;
    }
	struct file *file = file_from_fd(fd);
	if (!file) return -1;
	return file_write(file, buffer, size);
}

int read (int fd, void *buffer, unsigned size){
	if (fd == 0) { // Reading from keyboard
        for (unsigned i = 0; i < size; i++) {
            char c = input_getc();
            ((char *)buffer)[i] = c;
            putbuf(&c, 1);
        }
        return size;
    }
	struct file *file = file_from_fd(fd);
	if (!file) {
		return -1;
	}
	return file_read(file, buffer, size);
}

void validate_pointer(const void* ptr) {
    if (ptr == NULL || is_kernel_vaddr(ptr) || pagedir_get_page(thread_current()->pagedir, ptr) == NULL) {
		thread_current()->exit_status = -1;
        close_all_files();
		thread_exit();
    }
}
void validate_string(const char* str) {
	validate_pointer(str);
    for (;;) {
        validate_pointer(str);  // check that page is present
        if (*str == '\0') return;             // found terminator
        str++;
    }
}

void validate_buffer(const void* buffer, unsigned size) {
	char *start = (char*)buffer;
	char *end = start+size;
	validate_pointer(start);
	if((size > 0 && !is_user_vaddr(end-1))){
		thread_current()->exit_status = -1;
        close_all_files();
		thread_exit();
	}
	char* page_start = pg_round_down(start);
	char* page_end = pg_round_up(end);
	for(char* page = page_start; page < page_end; page += PGSIZE){
		if(pagedir_get_page(thread_current()->pagedir, page) == NULL){
			thread_current()->exit_status = -1;
        	close_all_files();
			thread_exit();
		}
	}
}