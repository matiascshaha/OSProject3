#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdint.h>

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
void listDir(int desc, int cluster);
void getDir(int desc, int cluster);
int findCluster(int x);

struct BPB {
    unsigned int BytesPerSec;
    unsigned int SecPerClust;
    unsigned int ResvSecCnt;
    unsigned int numFATs;
    unsigned int totalSectors;
    unsigned int FATsize;
    unsigned int rootCuster;
};

struct DIRENTRY{
	unsigned char DIRName[11];
	unsigned char DIRAttr;
	uint8_t DIRNTRes;
	uint8_t DIRCTTenth;
	uint16_t DIRCTime;
	uint16_t DIRCDate;
	uint16_t LADate;
	uint16_t highCluster;
	uint16_t DIRWTime;
	uint16_t DIRWDate;
	uint16_t lowCluster;
	uint32_t DIRSize;
}__attribute__((packed));


struct DIRENTRY dir[16];
int currentCluster;
int prevCluster;
int root[100];
int rootTrack;
struct BPB bpb;

int main(int argc, char **argv)
{
	rootTrack = 0;
	prevCluster = 2;
    int fd;
	int nextCluster;
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
	getDir(fd, bpb.rootCuster);
	
	//set initial root cluster into array
	root[rootTrack] = bpb.rootCuster;
	rootTrack++;
	if((lseek(fd, 16392, SEEK_SET)) == -1){
        printf("Cannot seek fat32.img\n");
    }
	
	while(1){
		if(!(read(fd, buff, 4) == 4))
		{
			printf("ERROR: cannot read %s\n", argv[1]);
			exit(0);
		}
		nextCluster = 0;
		for(int i = 0; i < 4; i++){
			nextCluster += (unsigned int) buff[i];
		}
		if( nextCluster < 268435448)
		{
			root[rootTrack] = nextCluster;
			nextCluster = findCluster(nextCluster);
			if((lseek(fd, nextCluster, SEEK_SET)) == -1){
				printf("Cannot seek fat32.img\n");
			}
			rootTrack++;
		}
		else
			break;
	};
	currentCluster = 2;
	
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
			int check;
			tokenlist *tok;
			for(int i = 0; i < 16; i++){
				tok = get_tokens(dir[i].DIRName);	
				if((check = strcmp(tokens->items[1], tok->items[0])) == 0){
					check = 0;
					printf("%d\n", dir[i].DIRSize);
					break;
				}
			}
			if(check != 0)
				printf("ERROR: %s does not exist.", tokens->items[1]);
            //fileSize(tokens->items[1]);

        }

        else if(strcmp(tokens->items[0], "ls") == 0)
        {
			int check;
			tokenlist *tok;
			if(tokens->size == 1 || strcmp(tokens->items[1], ".") == 0)
				listDir(fd, currentCluster);
			else{
				for(int i = 0; i < 16; i++){
					tok = get_tokens(dir[i].DIRName);	
					if((check = strcmp(tokens->items[1], tok->items[0])) == 0){
						listDir(fd, dir[i].lowCluster);
						break;
					}
				}
				if(check != 0)
					printf("ERROR: %s does not exist.", tokens->items[1]);
			}
		}

        else if(strcmp(tokens->items[0], "cd") == 0)
        {
			
		}

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
	memset(buff,0,4);

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

int findCluster(int x){
	if(x < 2)
		return ((bpb.ResvSecCnt * bpb.BytesPerSec) + (bpb.numFATs * bpb.FATsize * bpb.BytesPerSec)) + ((2 - 2) * bpb.BytesPerSec);
	return ((bpb.ResvSecCnt * bpb.BytesPerSec) + (bpb.numFATs * bpb.FATsize * bpb.BytesPerSec)) + ((x - 2) * bpb.BytesPerSec);
}

//ls the current directory
void listDir(int desc, int cluster)
{
	if(cluster > currentCluster){
		int offset = findCluster(cluster);
		if((lseek(desc, offset, SEEK_SET)) == -1)
			printf("Cannot seek fat32.img\n");

		//loop fills dir[] with a directory in each slot or until done
		//need file names still
		for (int i = 0; i < 16; i++)
		{
			if(!(read(desc, &dir[i], 32) == 32))
				printf("Cannot read fat32.img\n");
			//checks if Name and Attribute of directory exist
			if ((dir[i].DIRName[0] != (char)0xe5) &&
				(dir[i].DIRAttr == 0x1 || dir[i].DIRAttr == 0x10 || dir[i].DIRAttr == 0x20))
			{
				printf("%s\n", dir[i].DIRName);
			}
		}
		offset = findCluster(currentCluster);
		if((lseek(desc, offset, SEEK_SET)) == -1)
			printf("Cannot seek fat32.img\n");
		getDir(desc, currentCluster);
	}
	else{
		for (int i = 0; i < 16; i++)
		{
			//checks if Name and Attribute of directory exist
			if ((dir[i].DIRName[0] != (char)0xe5) &&
				(dir[i].DIRAttr == 0x1 || dir[i].DIRAttr == 0x10 || dir[i].DIRAttr == 0x20))
			{
				printf("%s\n", dir[i].DIRName);
			}
		}
	}
}

void getDir(int desc, int cluster){
    int offset = findCluster(cluster);
    if((lseek(desc, offset, SEEK_SET)) == -1)
		printf("Cannot seek fat32.img\n");
	
	for (int i = 0; i < 16; i++)
    {
        if(!(read(desc, &dir[i], 32) == 32))
			printf("Cannot read fat32.img\n");

    }
	
}
