#include <stdio.h>
#include <syscall.h>

int main(void)
{
    printf("test1");
	sleep(5000);
    printf("test2");

	return EXIT_SUCCESS;
}
