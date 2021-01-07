#include "syscall.h"

int strcmp(const char *str1, const char *str2) {
    while(*str1 && *str2 && (*str1 == *str2)) {
        ++ str1;
        ++ str2;
    }
    return *str1 - *str2;
}

int strncmp(const char *str1, const char *str2, int n){
    while(*str1 && (*str1 == *str2)) {
        ++ str1;
        ++ str2;
        if (--n == 0)
            return 0;
    }
    return *str1 - *str2;
}

int
main()
{
    SpaceId newProc;
    OpenFileId input = ConsoleInput;
    OpenFileId output = ConsoleOutput;
    char prompt[7], ch, buffer[60];
    int i;

    prompt[0] = '\360';
    prompt[1] = '\237';
    prompt[2] = '\220';
    prompt[3] = '\266';
    prompt[4] = '>';
    prompt[5] = '>';
    prompt[6] = ' ';

    while( 1 )
    {
	Write(prompt, 7, output);

        i = 0;

        do {

            Read(&buffer[i], 1, input);

        } while( buffer[i++] != '\n' );

        buffer[--i] = '\0';

        if( i > 0 ) {
            // Write("Read command: ", 15, output);
            // Write(buffer, i, output);
            // Write("\n", 2, output);
            if (!strcmp("help", buffer) || !strcmp("?", buffer)) {
                Help();
            }
            else if (!strcmp("exit", buffer) || !strcmp("quit", buffer)) {
                Write("GoodBye!\n", 10, output);
                Exit(0);
            } else if (!strcmp("halt", buffer)) {
                Write("System Shut Down!\n", 18, output);
                Halt();
            } else if (!strcmp("pwd", buffer)) {
                Pwd();
            } else if (!strcmp("ls", buffer)) {
                Ls();
            } else if (!strncmp("cd", buffer, 2)) {
                // Write((buffer + 3), 20, output);
                // Write("\n", 2, output);
                Cd((buffer + 3));
            } else if (!strncmp("touch", buffer, 5)) {
                // Write((buffer + 6), 20, output);
                // Write("\n", 2, output);
                Create((buffer + 6));
            } else if (!strncmp("mkdir", buffer, 5)) {
                // Write((buffer + 6), 20, output);
                // Write("\n", 2, output);
                MkDir((buffer + 6));
            } else if (!strncmp("rm", buffer, 2)) {
                // Write((buffer + 3), 20, output);
                // Write("\n", 2, output);
                Remove((buffer + 3));
            } else if (!strncmp("rmdir", buffer, 2)) {
                // Write((buffer + 6), 20, output);
                // Write("\n", 2, output);
                RmDir((buffer + 6));
            } else if (!strncmp("uptime", buffer, 6)) {
                Uptime();
            } else if (!strncmp("exec", buffer, 4)){
                // Write((buffer + 5), 20, output);
                // Write("\n", 2, output);
                newProc = Exec((buffer + 5));
                Join(newProc);
            } else {
                Write("Unsupported command! Try help or ? for support\n", 48, output);
            }
        }
    }
}

