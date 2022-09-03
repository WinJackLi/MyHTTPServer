#include <sys/stat.h>
#include <unistd.h>
#include <sys/stat.h>
#include <string.h>
#include <stdio.h>
#define SIZE 100

int main() {
    /*
     *修改文件权限的小程序，仅限于当前文件夹文件
     *可以把这个文件放置于目标文件夹中。输入1:私有，外界不能直接访问该资源
     *输入2:公开，外界可通过ip:port/url的形式直接访问到
     */
    char path[SIZE] = {0};
    getcwd(path, SIZE);
    while(1) 
    {
        char name[SIZE] = {0};
        char temp[2*SIZE] = {0};
        printf("请输入文件名 >>> ");
        scanf("%s", name);
        getchar();
        snprintf(temp, 2*SIZE, "%s/%s", path, name);   
        struct stat file_stat;
        char x;
        if (stat(temp, &file_stat) < 0) 
        {
            printf("没有该文件，是否继续(y/n) >>> ");
            scanf("%c", &x);
            getchar();
            if (x == 'y' || x == 'Y') continue;
            else break;
        }
        else
        {
            int l;
            printf("请输入文件权限\n1:私有\n2:公开\n>>> ");
            scanf("%d", &l);
            if (l == 1) {
                chmod(temp, S_IRWXU);
                printf("修改成功，是否继续(y/n) >>> ");
                scanf("%c", &x);
                getchar();
                if (x == 'y' || x == 'Y') continue;
                else break;
            }
            if (l == 2) {
                chmod(temp, file_stat.st_mode|S_IRUSR);
                printf("修改成功，是否继续(y/n) >>> ");
                scanf("%c", &x);
                getchar();
                if (x == 'y' || x == 'Y') continue;
                else break;
            }
            else {
                printf("输入了错误指令\n");
            }
        }

    }
 

}
          