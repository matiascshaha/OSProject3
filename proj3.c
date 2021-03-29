#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>

#define MAX_PATH 260

typedef struct {
    int size;
    char **items;
} tokenlist;
unsigned char * buff[5];
char *get_input(void);
tokenlist *get_tokens(char *input);

tokenlist *new_tokenlist(void);
void add_token(tokenlist *tokens, char *item);
void free_tokens(tokenlist *tokens);
void getInfo(int desc);
void printInfo();
void fileSize(const char *file);

struct BPB {
    unsigned int BytesPerSec;
    unsigned int SecPerClust;
    unsigned int ResvSecCnt;
    unsigned int numFATs;
    unsigned int totalSectors;
    unsigned int FATsize;
    unsigned int rootCuster;
};

struct BPB bpb;

int main(int argc, char **argv)
{
    int fd;
    if(argc != 2)
    {
        printf("ERROR: wrong number of args\n");
        exit(0);
    }

    if((fd = open(argv[1], O_RDWR)) == -1)
    {
        printf("ERROR: cannot open %s\n", argv[1]);
        exit(0);
    }

    else if(!(read(fd, buff, 2) == 2))
    {
        printf("ERROR: cannot read %s\n", argv[1]);
        exit(0);
    }
    getInfo(fd);
    while(1)
    {
        printf("$ ");
        char *input = get_input();
        tokenlist *tokens = get_tokens(input);

        if(strcmp(tokens->items[0], "exit") == 0)
            break;

        else if(strcmp(tokens->items[0], "info") == 0)
        {
            printInfo();
        }

        else if(strcmp(tokens->items[0], "size") == 0)
        {
            fileSize(tokens->items[1]);

        }

        else if(strcmp(tokens->items[0], "ls") == 0)
        {}

        else if(strcmp(tokens->items[0], "cd") == 0)
        {}

        else if(strcmp(tokens->items[0], "creat") == 0)
        {}

        else if(strcmp(tokens->items[0], "mkdir") == 0)
        {}

        else if(strcmp(tokens->items[0], "mv") == 0)
        {}

        else if(strcmp(tokens->items[0], "open") == 0)
        {}

        else if(strcmp(tokens->items[0], "close") == 0)
        {}

        else if(strcmp(tokens->items[0], "lseek") == 0)
        {}

        else if(strcmp(tokens->items[0], "read") == 0)
        {}

        else if(strcmp(tokens->items[0], "write") == 0)
        {}

        else if(strcmp(tokens->items[0], "rm") == 0)
        {}

        else if(strcmp(tokens->items[0], "cp") == 0)
        {}

            /*extra credit if we get to it
            else if(strcmp(tokens->items[0], "rmdir") == 0)
            {}
            else if(strcmp(tokens->items[0], "cp") == 0 &&
                            strcmp(tokens->items[1] == "-r") == 0)
            {}
            */

        else //not a recognized command
            printf("%s: command not found\n", tokens->items[0]);

        free(input);
        free_tokens(tokens);
    }
    //free dynamic memory if necessary
    return 0;
}
void fileSize(const char *file)
{
    unsigned int fsize;
    if(access(file, F_OK) == 0)
    {
        int fd = open(file, O_RDONLY);
        off_t pos = lseek(fd, 0, SEEK_CUR); //start position of file pointer
        fsize = lseek(fd, 0, SEEK_END); //traverse file and store size
        lseek(fd, pos, SEEK_SET); //set file pointer back to start
        printf("%s size: %d bytes\n", file, fsize);
        close(fd); //close file
    }
    else
        printf("ERROR: %s does not exist\n", file);
}

void printInfo()
{
    printf("Bytes Per Sector: %d\n", bpb.BytesPerSec);
    printf("Sectors Per Cluster: %d\n", bpb.SecPerClust);
    printf("Reserved: %d\n", bpb.ResvSecCnt);
    printf("Number of FATs: %d\n", bpb.numFATs);
    printf("Total Sectors: %d\n", bpb.totalSectors);
    printf("FAT size: %d\n", bpb.FATsize);
    printf("Root Cluster: %d\n", bpb.rootCuster);

}

void getInfo(int desc)
{
    unsigned char a[2];

    if((lseek(desc, 11, SEEK_SET)) == -1){
        printf("Cannot seek fat32.img\n");
    }
    else if(!(read(desc, buff, 2) == 2)){
        printf("Cannot read fat32.img\n");
    }
    bpb.BytesPerSec = 0;
    for(int i = 0; i < 2; i++){
        bpb.BytesPerSec += (unsigned int) buff[i];
    }
    memset(buff,0,4);       //reset buffer to empty
    //secperclust
    if(!(read(desc, buff, 1) == 1)){
        printf("Cannot read fat32.img\n");
    }
    bpb.SecPerClust = 0;
    for(int i = 0; i < 1; i++){
        bpb.SecPerClust += (unsigned int) buff[i];
    }
    memset(buff,0,4);       //reset buffer to empty

    //reserved
    if(!(read(desc, buff, 2) == 2)){
        printf("Cannot read fat32.img\n");
    }
    bpb.ResvSecCnt = 0;
    for(int i = 0; i < 2; i++){
        bpb.ResvSecCnt += (unsigned int) buff[i];
    }
    memset(buff,0,4);       //reset buffer to empty

    //numFat
    if(!(read(desc, buff, 1) == 1)){
        printf("Cannot read fat32.img\n");
    }
    bpb.numFATs = 0;
    for(int i = 0; i < 1; i++){
        bpb.numFATs += (unsigned int) buff[i];
    }
    memset(buff,0,4);       //reset buffer to empty

    //totalSectors
    if((lseek(desc, 32, SEEK_SET)) == -1){
        printf("Cannot seek fat32.img\n");
    }
    else if(!(read(desc, buff, 4) == 4)){
        printf("Cannot read fat32.img\n");
    }
    bpb.totalSectors = 0;
    for(int i = 0; i < 4; i++){
        bpb.totalSectors += (unsigned int) buff[i];
    }
    memset(buff,0,4);       //reset buffer to empty

    //fatsize
    if(!(read(desc, buff, 4) == 4)){
        printf("Cannot read fat32.img\n");
    }
    bpb.FATsize = 0;
    for(int i = 0; i < 4; i++){
        bpb.FATsize += (unsigned int) buff[i];
    }
    memset(buff,0,4);       //reset buffer to empty

    //root cluster
    if((lseek(desc, 44, SEEK_SET)) == -1){
        printf("Cannot seek fat32.img\n");
    }
    else if(!(read(desc, buff, 4) == 4)){
        printf("Cannot read fat32.img\n");
    }
    bpb.rootCuster = 0;
    for(int i = 0; i < 4; i++){
        bpb.rootCuster += (unsigned int) buff[i];
    }

}

tokenlist *new_tokenlist(void)
{
    tokenlist *tokens = (tokenlist *) malloc(sizeof(tokenlist));
    tokens->size = 0;
    tokens->items = (char **) malloc(sizeof(char *));
    tokens->items[0] = NULL; /* make NULL terminated */
    return tokens;
}

void add_token(tokenlist *tokens, char *item)
{
    int i = tokens->size;

    tokens->items = (char **) realloc(tokens->items, (i + 2) * sizeof(char *));
    tokens->items[i] = (char *) malloc(strlen(item) + 1);
    tokens->items[i + 1] = NULL;
    strcpy(tokens->items[i], item);

    tokens->size += 1;
}

char *get_input(void)
{
    char *buffer = NULL;
    int bufsize = 0;

    char line[5];
    while (fgets(line, 5, stdin) != NULL) {
        int addby = 0;
        char *newln = strchr(line, '\n');
        if (newln != NULL)
            addby = newln - line;
        else
            addby = 5 - 1;

        buffer = (char *) realloc(buffer, bufsize + addby);
        memcpy(&buffer[bufsize], line, addby);
        bufsize += addby;

        if (newln != NULL)
            break;
    }

    buffer = (char *) realloc(buffer, bufsize + 1);
    buffer[bufsize] = 0;

    return buffer;
}

tokenlist *get_tokens(char *input)
{
    char *buf = (char *) malloc(strlen(input) + 1);
    strcpy(buf, input);

    tokenlist *tokens = new_tokenlist();

    char *tok = strtok(buf, " ");
    while (tok != NULL) {
        add_token(tokens, tok);
        tok = strtok(NULL, " ");
    }

    free(buf);
    return tokens;
}

void free_tokens(tokenlist *tokens)
{
    for (int i = 0; i < tokens->size; i++)
        free(tokens->items[i]);
    free(tokens->items);
    free(tokens);
}
