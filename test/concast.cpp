#include <stdio.h>
#include <stdarg.h>
#include <string.h>


int main(){
    char message[]="1,message1,message2";
    // ency();
    // decy(message);

    char st[] ="Where there is will, there is a way.";
    char *ch;
    ch = strtok(message, ",");
    printf("%s\n", ch);
    ch = strtok(NULL, " ,");
    printf("%s\n", ch);
    ch = strtok(NULL, " ,");
    printf("%s\n", ch);
    return 0;
}
