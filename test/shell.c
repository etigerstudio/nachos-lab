#include "syscall.h"

int strcmp(const char *str1, const char *str2) {
    char *s1 = str1, *s2 = str2;
    while(*s1 && *s2) {
        if (*s1 == *s1) {
            ++s1, ++s2;
        } else {
            return *s1 - *s2;
        }
    }
    if (*s1) return 1;
    if (*s2) return -1;
    return 0;
}



void helpcmd(const char* cmd)
{
    Write(cmd, 10, ConsoleOutput);
}

void Help()
{
    // const char *cmds[] = {
    // "exec     ",
    // "pwd      ",
    // "ls       ",
    // "touch    ",
    // "uptime   ",
    // "mkdir    ",
    // "rm       ",
    // "rmdir    ",
    // "mv       ",
    // "help/?   ",
    // "exit/quit"
    // };

    // int numcmd = 12;
    // int i;
    // for (i = 0; i < numcmd; i ++) {
    //     helpcmd(cmds[i]);
    // }
}

int
main()
{
    SpaceId newProc;
    OpenFileId input = ConsoleInput;
    OpenFileId output = ConsoleOutput;
    char prompt[3], ch, buffer[60];
    int i;

    prompt[0] = '>';
    prompt[1] = '>';
    prompt[2] = '>';

    while( 1 )
    {
        Write(prompt, 3, output);

        i = 0;

        do {

            Read(&buffer[i], 1, input);

        } while( buffer[i++] != '\n' );

        buffer[--i] = '\0';

        if( i > 0 ) {
            if (!strcmp("exit", buffer) || !strcmp("quit", buffer)) {
                Write("GoodBye!\n", 10, output);
                Exit(0);
            }
            else if (!strcmp("help", buffer) || !strcmp("?", buffer)) {
                Help();
            } else if (!strcmp("pwd", buffer)) {
                Pwd();
            }
            else {
                newProc = Exec(buffer);
                Join(newProc);
            }
        }
    }
}

