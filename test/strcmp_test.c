#include <stdio.h>

int strcmp_old(const char *str1, const char *str2){
    while(*str1 && *str2) {
        if (*str1 == *str2) {
            ++ str1, ++ str2;
        } else {
            return *str1 - *str2;
        }
    }
    if (*str1) return 1;
    if (*str2) return -1;
    return 0;
}

int strcmp(const char *str1, const char *str2) {
    while(*str1 && *str2 && (*str1 == *str2)) {
        ++ str1;
        ++ str2;
    }
    return *str1 - *str2;
}


int main()
{
    char *str1 = "help";
    char *str2 = "exit";
    char *str3 = "help11";
    printf("%d\n", strcmp(str1, str1));
    printf("%d\n", strcmp(str1, str2));
    printf("%d\n", strcmp(str1, str3));
    printf("%d\n", strcmp(str2, str3));

    printf("%d\n", strcmp_old(str1, str1));
    printf("%d\n", strcmp_old(str1, str2));
    printf("%d\n", strcmp_old(str1, str3));
    printf("%d\n", strcmp_old(str2, str3));
    return 0;
}