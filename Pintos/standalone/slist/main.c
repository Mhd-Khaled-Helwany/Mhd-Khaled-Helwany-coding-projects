#include "list.h"
#include <stdio.h>
#include <stdlib.h>

int main()
{
	struct list_item root;
	root.value = -1;
	root.next = NULL;

	append(&root, 1);
	append(&root, 2);
	prepend(&root, 5);
	input_sorted(&root, 0);
	input_sorted(&root, 3);

	print(&root);
	// should print: 0,3,5,1,2
	clear(&root);
	/* Write your test cases here */
	return 0;
}
