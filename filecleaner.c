/* Переход по древу каталога и удаление всех повторных жестких ссылок
1. Первый аргумент -cur     -начиная с текущего каталога
        1. Получить данные о пути каталога
        2. Вызвать рекурсивную функцию прохода по первичному адресу.

        Работа функции:
        Просмотр потока файлов до его окончания
        Если файл является простым файлом, то проверка наличия жесткой ссылки. в случае наличия - удаление.
        Если файл является каталогом, то рекурсивный вызов по новому адресу.

2. Первый аргумент -path, второй /a/b/c - путь по каталогу
*/

#include <stdio.h>
#include <stdlib.h>

#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <strings.h>
#include <fcntl.h>

#define PATH_MAX 4096
#define GONEXT current = current->next;
#define TESTING

struct link_node
{
    int link;
    struct link_node *next;
};

struct link_node *first;
struct link_node *current;
struct link_node *last;

void add_link(int link); //Добавление жесткой ссылки (в соотв. место)
unsigned check_link(int link); //Проверка наличия, 0 - остутствие, 1 - наличие(нужно удалять файл)

char *get_cwd(void);
unsigned nurc(const char* directory);
void define_type(struct stat* curr_stat);

int main(void)
{
    first = NULL;
    current = NULL;
    last = NULL;

    add_link(1);
    add_link(20);


    printf("%s\n", get_cwd());

    nurc(get_cwd());

    return 0;
}

unsigned nurc(const char* directory) //Возврат
{
    DIR* loc_dir = NULL;
    if(!(loc_dir = opendir(directory)))
    {
        printf("OpenDirError\n");
        printf("%s\n", directory);
        return 0;
    }

    #ifdef TESTING
    printf("\n\nNew directory: %s\n", directory);
    #endif // TESTING

    struct dirent* curr_file;
    while((curr_file = readdir(loc_dir)) != NULL)
    {
        if((strcmp(curr_file->d_name, ".") != 0) && (strcmp(curr_file->d_name, "..") != 0)) //Отбрасываем ссылки на род. каталог и себя
        {
            printf("NewFile: %s, %u\n", curr_file->d_name, (unsigned)curr_file->d_ino);

            //Формирование строки - полного пути
            char fullpath[PATH_MAX];
            strconc(directory, curr_file->d_name, fullpath);
            printf("%s\n", fullpath);
            //Получение структуры stat
            struct stat statbuf;
            if((lstat(fullpath, &statbuf)) == -1) //Вот тут возникает ошибка и дамп ядра
            {
                printf("LstatErr\n");
                exit(7);
            }


            if(S_ISDIR(statbuf.st_mode))
            {
                //рекурсия
                if(nurc(fullpath))
                {
                    //Удаление пустой директории
                    printf("Now is EMPTY\n");
                }
            }
            else
            {
                if(S_ISLNK(statbuf.st_mode)) //Удаление мягких ссылок
                {
                    printf("File is SYMLINK: %s\n", fullpath);
                }
                else //Удаление повторных жестких ссылок
                {
                    if(check_link((unsigned)curr_file->d_ino))
                    {
                        printf("File repeat: %s\n", fullpath);
                        if(remove(fullpath) == -1)
                        {
                            printf("RemoveError\n");
                            exit(8);
                        }
                    }
                    else
                    {
                        add_link((unsigned)curr_file->d_ino);
                    }
                }
            }
        }
    }
    //Проверка пустого каталога
    curr_file = NULL;
    rewinddir(loc_dir);
    unsigned counter = 0;
    while((curr_file = readdir(loc_dir)) != NULL)
    {
        counter++;
    }

    if(counter < 2) //Пустой каталог
    {
        return 0;
    }
    else
    {
        return 1;
    }
}

void strconc(const char* s1, const char* s2, char* res)
{
    strcpy(res, s1);
    unsigned s1_len = (unsigned)strlen(s1);
    *(res+s1_len) = '/';
    strcpy((res+s1_len+1), s2);
}

char* get_cwd(void)
{
    static char* cwd_path;
    cwd_path = (char*)malloc(sizeof(char)*4096);
    char* temp_ptr = getcwd(cwd_path, (size_t)PATH_MAX);
    if(!temp_ptr)
    {
        free(cwd_path);
        printf("CwdGet");
        exit(3);
    }
    return cwd_path;
}

struct link_node* node_nalloc(void)
{
    struct link_node *temp_node = (struct link_node*)malloc(sizeof(struct link_node));
    if(!temp_node)
    {
        printf("MallocError");
        exit(2);
    }
    temp_node->next = NULL;
    return temp_node;
}

void add_link(int link)
{
    if(!first)
    {
        first = node_nalloc();
        first->link = link;
        last = first;
    }
    else
    {
        last->next = node_nalloc();
        last = last->next;
        last->link = link;
    }
}

unsigned check_link(int link) //1 - наличие, 0 - отсутствие
{
    unsigned flag = 0;
    if(first)
    {
        current = first;
        while((current->next) && (current->link != link))
        {
            GONEXT;
        }
        if(current->link == link)
        {
            flag = 1;
        }
    }
    return flag;
}
