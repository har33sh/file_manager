#include<stdio.h>
#include<dirent.h>

int main(void)
{
    DIR *d;
    int i=0,j;
    struct dirent *dir;
    char files[100][100];
    d = opendir("/home/ghost/Downloads");
    if (d)
    {
        while ((dir = readdir(d)) != NULL)
        {
            // printf("%s\n", dir->d_name);
            snprintf(files[i], sizeof(files[i]),dir->d_name);
            i++;
        }
        closedir(d);
    }

    for (j=0;j<i;j++){
        printf("%d %s\n",j,files[j] );
    }
    return(0);
}
