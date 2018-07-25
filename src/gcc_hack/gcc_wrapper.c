// To build with "gcc -o build/gcc_wrapper src/gcc_hack/gcc_wrapper.c

#include <stdio.h>
#include <string.h>
#include <unistd.h>

#define MAX_NOF_ARGS 255
#define STATE_UNDEFINED  0
#define STATE_LINKING_SO 1

int main( int argc, char const* argv[], char* const envp[])
{
	char const* argvcopy[ MAX_NOF_ARGS+1];
	argvcopy[0] = "/usr/bin/gcc";
	int ai = 1;
	int ci = 1;
	int state = STATE_UNDEFINED;
	for (;  ai != argc && ai < MAX_NOF_ARGS; ++ai)
	{
		int argumentAccepted = 1;
		if (0==strcmp( argv[ ai], "-shared"))
		{
			state = STATE_LINKING_SO;
		}
		else if (state == STATE_LINKING_SO && 0==strcmp( argv[ ai], "-pie"))
		{
			argumentAccepted = 0;
		}
		if (argumentAccepted)
		{
			argvcopy[ ci++] = argv[ ai];
		}
	}
	argvcopy[ ci++] = NULL;
	return execve( argv[0], (char* const*)argvcopy, envp);
}


