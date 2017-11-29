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
#ifdef WIN32
            /* putchar on windows auto-converts \n to \r\n */
			putchar( '\n' );
#else
            /* other platforms must output CR LF manually */
			putchar(015);	/* ^M */
			putchar(012);	/* ^J */
#endif
		} else {
			putchar(c);
		}
	}

	exit(0);
}
