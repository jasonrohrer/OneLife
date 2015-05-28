#include <stdio.h>
#include <stdlib.h>

int
main(void)
{
	while(1) {
		int c = getchar();

		if(c == EOF)
			exit(0);

		if(c == '\n') {
			putchar(015);	/* ^M */
			putchar(012);	/* ^J */
		} else {
			putchar(c);
		}
	}

	exit(0);
}
