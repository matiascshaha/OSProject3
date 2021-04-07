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
void findFatSequence(int fd, int cluster);
void getBig(int desc, int cluster);
void createFile(int fd,int cluster,char *filename);
void makeDir(int fd,int cluster,char *filename);
void remDir(int desc, unsigned int cluster, int off, int current);
int checkAttr(char* from, char* to); //checks to see if mv args are legal
void moveEntry(int desc, int index, int moveToCluster);
void remEntry(int desc, int off, int current);
void copyFile(int desc, int newClust, int origClust);
void myReadFunc(int fd, int filePosOffset,int bytesToRead, int fileCluster, int file_size,int read_to_end,char *filename);

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

struct openTable{
    unsigned char filename[11];
    unsigned char mode[3];
    unsigned int startCluster;
    unsigned int offset_position;
};

struct openTable op[100];
struct DIRENTRY dir[16];
struct DIRENTRY redDir[208];
int openCheck;
int redTrack;
int dirTrack;
int currentCluster;
int prevCluster;
int root[100];
int rootTrack;
int fat[500];
int fatTrack;
struct BPB bpb;
char fileName[32];
int cdClust[100];
int cdTrack;

int main(int argc, char **argv)
{
	//data initialization region
    strcpy(fileName,argv[1]);
    rootTrack = 0;
    fatTrack = 0;
    prevCluster = 2;
    dirTrack = 0;
    cdClust[0] = 2;
    cdTrack = 1;
    redTrack = 0;
    openCheck = 0;
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
    getInfo(fd);	//populates BPB struct

    //set initial root cluster into array
    findFatSequence(fd,bpb.rootCuster);
    currentCluster = 2;
    getDir(fd, bpb.rootCuster);
	
	//utility loop runs until exit is called.
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
            for(int i = 0; i < dirTrack; i++){
                tok = get_tokens(dir[i].DIRName);
                if((check = strcmp(tokens->items[1], tok->items[0])) == 0){
                    check = 0;
                    printf("%d\n", dir[i].DIRSize);
                    break;
                }
            }
            if(check != 0)
                printf("ERROR: %s does not exist.\n", tokens->items[1]);
            //fileSize(tokens->items[1]);

        }

        else if(strcmp(tokens->items[0], "ls") == 0)
        {
            int check;
            int d;
            tokenlist *tok;
            if(tokens->size == 1 || strcmp(tokens->items[1], ".") == 0)
                listDir(fd, currentCluster);
            else{
                for(int i = 0; i < dirTrack; i++){
                    tok = get_tokens(dir[i].DIRName);
                    if(((check = strcmp(tokens->items[1], tok->items[0])) == 0)
                       && (dir[i].DIRAttr != 0x20)){
                        listDir(fd, dir[i].lowCluster);
                        break;
                    }
                    else if((strcmp(tokens->items[1], tok->items[0]) == 0)
                            && (dir[i].DIRAttr == 0x20)){
                        d = 1;
                        break;
                    }
                }
                if(d == 1)
                    printf("ERROR: %s is not a directory.\n", tokens->items[1]);
                else if(check != 0)
                    printf("ERROR: %s does not exist.\n", tokens->items[1]);
            }
        }

        else if(strcmp(tokens->items[0], "cd") == 0)
        {
            int check;
            char * red;
            int redFlip;
            red = tokens->items[1];
            tokenlist *tok;
            if(strcmp(tokens->items[1], "..") == 0){
                if(cdTrack > 1){
                    currentCluster = cdClust[cdTrack];
                    cdTrack--;
                }
                else
                    printf("ERROR: In the root directory.\n");

            }
            else if(strcmp(tokens->items[1], ".") == 0)
                currentCluster = currentCluster;
            else if(tokens->size == 1){
                currentCluster = 2;
                cdTrack = 1;
            }
            else if((strlen(red) > 3) && (red[0] == 'R' && red[1] == 'E' && red[2] == 'D')){
                if(red[0] == 'R' && red[1] == 'E' && red[2] == 'D'){
                //if directory entered is RED then parse giant directory array instead
					for(int i = 0; i < redTrack; i++){
                        tok = get_tokens(redDir[i].DIRName);
                        if((check = strcmp(tokens->items[1], tok->items[0])) == 0){
                            cdTrack++;
                            cdClust[cdTrack] = currentCluster;
                            currentCluster = redDir[i].lowCluster;
                            break;
                        }
                    }
                }
            }
            else{
                redFlip = 0;
                int d = 0;
                for(int i = 0; i < dirTrack; i++){
                    tok = get_tokens(dir[i].DIRName);
                    if(((check = strcmp(tokens->items[1], tok->items[0])) == 0)
                       && (dir[i].DIRAttr != 0x20)){
                        cdTrack++;
                        cdClust[cdTrack] = currentCluster;
                        currentCluster = dir[i].lowCluster;
						//if RED or GREEN then true
                        if((strcmp(tokens->items[1], "RED") == 0)
                           || (strcmp(tokens->items[1], "GREEN") == 0)){
                            redFlip = 1;
                        }
                        break;
                    }
                    else if((strcmp(tokens->items[1], tok->items[0]) == 0)
                            && (dir[i].DIRAttr == 0x20)){
                        d = 1;
                        break;
                    }
                }
                if(d == 1)
                    printf("ERROR: %s is not a directory.\n", tokens->items[1]);
                else if(check != 0)
                    printf("ERROR: %s does not exist.\n", tokens->items[1]);
            }
            if(redFlip == 1){	//if RED or GREEN populate large DIRENTRY array
                findFatSequence(fd, currentCluster);
                getBig(fd, currentCluster);
            }
            else{
                findFatSequence(fd, currentCluster);
                getDir(fd, currentCluster);
            }
        }

        else if(strcmp(tokens->items[0], "creat") == 0)
        {
            if(tokens->size == 1)
            {
                printf("ERROR: missing file operand\n");
                continue;
            }
            else if(strlen(tokens->items[1]) > 8)
                printf("ERROR: %s is more than the 8 character limit.\n", tokens->items[1]);
            else
            {
                int temp =0;
                //check if file already exits
                tokenlist *tok;
                for(int i = 0; i < dirTrack; i++)
                {
                    tok = get_tokens(dir[i].DIRName);
                    if(strcmp(tokens->items[1], tok->items[0]) == 0)
                    {
                        printf("ERROR: %s already exist!\n", tok->items[0]);
                        temp = 1;
                        break;
                    }
                }

                //if it doesn't exist, create it
                if(temp != 1)
                {
                    createFile(fd, currentCluster,tokens->items[1]);
                }
            }
        }

        else if(strcmp(tokens->items[0], "mkdir") == 0)
        {
            if(tokens->size == 1)
            {
                printf("ERROR: missing file operand\n");
                continue;
            }
            else if(strlen(tokens->items[1]) > 8)
                printf("ERROR: %s is more than the 8 character limit.\n", tokens->items[1]);
            else
            {
                int temp =0;
                //check if file already exits
                tokenlist *tok;
                for(int i = 0; i < dirTrack; i++)
                {
                    tok = get_tokens(dir[i].DIRName);
                    if(strcmp(tokens->items[1], tok->items[0]) == 0)
                    {
                        printf("ERROR: %s already exist!\n", tok->items[0]);
                        temp = 1;
                        break;
                    }
                }

                //if it doesn't exist, create it
                if(temp != 1)
                {
                    makeDir(fd, currentCluster,tokens->items[1]);
                }
            }
        }

        else if(strcmp(tokens->items[0], "mv") == 0)
        {
            if(tokens->size < 3)
            {
                printf("ERROR: wrong number of args\n");
                continue;
            }

            if(!checkAttr(tokens->items[1], tokens->items[2]))
            {
                printf("Cannot move directory: invalid destination argument\n");
                continue;
            }
            //if 'TO' already exist
            tokenlist *tok;
            int exist = 0;
            for(int i = 0; i < dirTrack; i++)
            {
                tok = get_tokens(dir[i].DIRName);
                if((strcmp(tokens->items[2], tok->items[0]) == 0)
                   && (dir[i].DIRAttr == 0x20))
                {
                    printf("ERROR: %s already exist\n", tokens->items[2]);
                    exist = 1;
                    break;
                }
            }
            if(exist)
                continue;
            else
            {
                //valid case, continue with mv
                int temp = 0;
                int mvClust = 0;
                int check1 = 0, check2 = 0;
                for(int i = 0; i < dirTrack; i++)
                {
                    tok = get_tokens(dir[i].DIRName);
                    if(strcmp(tokens->items[1], tok->items[0]) == 0)
                    {
                        temp = i;
                        check1 = 1;
                    }
                    else if(strcmp(tokens->items[2], tok->items[0]) == 0)
                    {
                        mvClust = dir[i].lowCluster;
                        check2 = 1;
                    }
                }
                if(check1 != 1)
                    printf("ERROR: %s does not exist.\n", tokens->items[1]);

                else if(check2 != 1){
                    strncpy(dir[temp].DIRName, tokens->items[2], 11);

                    if ((lseek(fd, (findCluster(currentCluster)+(32*temp)), SEEK_SET)) == -1){
                        printf("Cannot seek fat32.img\n");
                    }

                    if(!(write(fd,&dir[temp], 32) == 32))
                    {
                        printf("ERROR: cannot write %s\n", fileName);
                        exit(0);
                    }
                }
                else
                {
                    moveEntry(fd, temp, mvClust);
                    remEntry(fd, temp, currentCluster);
                }
            }
        }

        else if(strcmp(tokens->items[0], "open") == 0)
        {
            tokenlist *tok;
            int isOpen, exist;
            if(tokens->size == 1)
                printf("ERROR: Unable to open ' '.\n");
                //checks correct mode flag is passed
            else if(tokens->size < 3 || (!(strcmp(tokens->items[2], "r") == 0)
                && !(strcmp(tokens->items[2], "w") == 0) && !(strcmp(tokens->items[2], "wr") == 0)
                && !(strcmp(tokens->items[2], "rw") == 0))){
                printf("Cannot open %s in mode %s.\n", tokens->items[1], tokens->items[2]);
            }
            else{
                isOpen = 0;
                exist = 0;

                //checks if file is already open
                for(int j = 0; j < 100; j++){
                    if(strcmp(tokens->items[1], op[j].filename) == 0){
                        printf("ERROR: %s is already open.\n", tokens->items[0]);
                        isOpen = 1;
                        break;
                    }
                }
                //parses directory entries for opening
                for(int i = 0; i < dirTrack; i++)
                {
                    tok = get_tokens(dir[i].DIRName);

                    if((strcmp(tokens->items[1], tok->items[0]) == 0) && (isOpen == 0))
                    {
                        exist = 1;
                        openCheck++; //tracks total number of files open

                        //adds new file to the open table at first empty space in array
                        for(int j = 0; j < 100; j++){
                            if(op[j].filename[0] == '\0'){
                                op[j].startCluster = dir[i].lowCluster;
                                op[j].offset_position = 0;
                                strcpy(op[j].filename, tokens->items[1]);
                                strcpy(op[j].mode, tokens->items[2]);
                                break;
                            }
                        }
                        break;
                    }
                }
                if(exist != 1)
                    printf("ERROR: %s does not exist.\n", tok->items[1]);
            }
        }

        else if(strcmp(tokens->items[0], "close") == 0)
        {
            tokenlist *tok;
            int isOpen;
            if(tokens->size == 1)
                printf("ERROR: Unable to open (null).\n");
            else{
                isOpen = 1;
                for(int j = 0; j < 100; j++){
					//if file is open then set values to NULL
                    if(strcmp(tokens->items[1], op[j].filename) == 0){
                        op[j].startCluster = 0;
                        op[j].offset_position = 0;
                        memset(op[j].filename, 0, 11);
                        memset(op[j].mode, 0, 3);
                        openCheck--;
                        isOpen = 0;
                        break;
                    }
                }
                if(isOpen == 1)
                    printf("ERROR: %s is not open.\n", tokens->items[1]);
            }
        }

        else if(strcmp(tokens->items[0], "lseek") == 0)
        {
            tokenlist *tok;
            int exist = 1;
            int isOpen = 1;
            if(tokens->size < 3)
                printf("ERROR: Unable to lseek (null).\n");
            else
            {
                for(int i = 0; i < dirTrack; i++)
                {
                    tok = get_tokens(dir[i].DIRName);
                    //check if file exists in current directory
                    if(strcmp(tokens->items[1],tok->items[0]) == 0)
                    {
                        exist = 0;
                        //check if it is a directory
                        if(dir[i].DIRAttr == 0x10)
                        {
                            printf("ERROR: %s is a directory\n", tokens->items[1]);
                            break;
                        }
                        for(int x = 0; x < 100; x++)
                        {
                            //check if file is in open table
                            if(strcmp(tokens->items[1],op[x].filename) == 0)
                            {
                                isOpen = 0;
                                //check to see if offset argument is greater than file size
                                if((atoi(tokens->items[2]) < 0))
                                {
                                    printf("ERROR: offset %d is less than 0\n",tokens->items[2]);
                                }
                                else if(atoi(tokens->items[2]) > dir[i].DIRSize)
                                {

                                    printf("ERROR: offset %s is bigger than file size for ( %s ) \n",tokens->items[2],tokens->items[1]);
                                }
                                else
                                {
                                    //change offset position of open file
                                    op[x].offset_position = atoi(tokens->items[2]);
                                }
                                break;
                            }

                        }
                        if(isOpen != 0)
                            printf("ERROR: file %s is not open to lseek\n",tokens->items[1]);

                        break;
                    }

                }
                if(exist != 0)
                    printf("ERROR: file %s doesn't exist\n",tokens->items[1]);
            }
        }

        else if(strcmp(tokens->items[0], "read") == 0)
        {
            tokenlist *tok;
            int exist = 1;
            int isOpen = 1;
            if(tokens->size < 3)
            {
                printf("ERROR: Unable to read (null).\n");
            }
            else
            {
                for(int i = 0; i < dirTrack; i++)
                {
                    tok = get_tokens(dir[i].DIRName);
                    //check if file exists in current directory
                    if(strcmp(tokens->items[1],tok->items[0]) == 0)
                    {
                        exist = 0;
                        //check if it is a directory
                        if(dir[i].DIRAttr == 0x10)
                        {
                            printf("ERROR: %s is a directory\n", tokens->items[1]);
                            break;
                        }
                        for(int x = 0; x < 100; x++)
                        {
                            //check if file is in open table
                            if(strcmp(tokens->items[1],op[x].filename) == 0)
                            {
                                isOpen = 0;

                                //if offset + SIZE is grater than fileSize
                                if((atoi(tokens->items[2]) + op[x].offset_position) > dir[i].DIRSize)
                                {
                                    //call read function here and read to end
                                    myReadFunc(fd,(findCluster(dir[i].lowCluster) + op[x].offset_position),
                                               (atoi(tokens->items[2]) - dir[i].DIRSize),dir[i].lowCluster,dir[i].DIRSize,1,tokens->items[1]);

                                }
                                else
                                {//printf("%d\n", dir[i].lowCluster);
                                    //call read function here and read the number of bytes given
                                    myReadFunc(fd,(findCluster(dir[i].lowCluster) + op[x].offset_position),
                                               atoi(tokens->items[2]),dir[i].lowCluster,dir[i].DIRSize,0,tokens->items[1]);
                                }
                                break;
                            }

                        }
                        if(isOpen != 0)
                            printf("ERROR: file %s is not open to read\n",tokens->items[1]);

                        break;
                    }

                }
                if(exist != 0)
                    printf("ERROR: file %s doesn't exist\n",tokens->items[1]);


            }
        }

        else if(strcmp(tokens->items[0], "write") == 0)
        {}

        else if(strcmp(tokens->items[0], "rm") == 0)
        {
            if(tokens->size == 1)
                printf("ERROR: Cannot remove (null).\n");
            else{
                int d;
                int temp = 0;
                int offset = 0;
                //check if file already exists
                tokenlist *tok;
                for(int i = 0; i < dirTrack; i++)
                {

                    tok = get_tokens(dir[i].DIRName);
                    if((strcmp(tokens->items[1], tok->items[0]) == 0)
                       && (dir[i].DIRAttr != 0x10))
                    {
                        remDir(fd, dir[i].lowCluster, i, currentCluster);
                        temp = 1;
						printf("%s removed: %d bytes freed.\n", tokens->items[1], dir[i].DIRSize);
                        break;
                    }
                    else if((strcmp(tokens->items[1], tok->items[0]) == 0)
                            && (dir[i].DIRAttr == 0x10)){
                        d = 1;
                        break;
                    }
                }

                //if it doesn't exist
                if(d == 1)
                    printf("ERROR: %s is a directory.\n", tokens->items[1]);
                else if(temp != 1)
                    printf("ERROR: %s doesn't exist.\n", tokens->items[1]);
            }
        }

        else if(strcmp(tokens->items[0], "cp") == 0)
        {
            int temp = 0;
            if(tokens->size < 3)
            {
                printf("ERROR: wrong number of args\n");
                continue;
            }

            if(!checkAttr(tokens->items[1], tokens->items[2]))
            {
                printf("Cannot move directory: invalid destination argument\n");
                continue;
            }
            //if 'TO' already exist
            tokenlist *tok;
            int exist = 0;
            for(int i = 0; i < dirTrack; i++)
            {
                tok = get_tokens(dir[i].DIRName);
                if((strcmp(tokens->items[2], tok->items[0]) == 0)
                   && (dir[i].DIRAttr == 0x20))
                {
                    printf("ERROR: %s already exist\n", tokens->items[2]);
                    exist = 1;
                    break;
                }
            }
            if(exist)
                continue;
            else
            {
                //valid case, continue with mv
                int temp = 0;
                int mvClust = 0;
                int check1 = 0, check2 = 0;
                for(int i = 0; i < dirTrack; i++)
                {
                    tok = get_tokens(dir[i].DIRName);
                    if(strcmp(tokens->items[1], tok->items[0]) == 0)
                    {
                        temp = dir[i].lowCluster;
                        check1 = 1;
                    }
                    else if(strcmp(tokens->items[2], tok->items[0]) == 0)
                    {
                        mvClust = dir[i].lowCluster;
                        check2 = 1;
                    }
                }
                if(check1 != 1)
                    printf("ERROR: %s does not exist.\n", tokens->items[1]);

                else if(check2 != 1){
                    strncpy(dir[temp].DIRName, tokens->items[2], 11);

                    if ((lseek(fd, (findCluster(currentCluster)+(32*temp)), SEEK_SET)) == -1){
                        printf("Cannot seek fat32.img\n");
                    }

                    if(!(write(fd,&dir[temp], 32) == 32))
                    {
                        printf("ERROR: cannot write %s\n", fileName);
                        exit(0);
                    }
                }
                else
                {
                    createFile(fd, mvClust, tokens->items[1]);
                    findFatSequence(fd, mvClust);
                    getDir(fd, mvClust);
                    tokenlist * tok;
                    for(int i = 0; i < dirTrack; i++)
                    {
                        printf("%s\n", dir[i].DIRName);
                        tok = get_tokens(dir[i].DIRName);
                        if(strcmp(tokens->items[1], tok->items[0]) == 0)
                        {
                            printf("%d %d\n", temp, dir[i].lowCluster);
                            //copyFile(fd, dir[i].lowCluster, temp);
                            break;
                        }
                    }

                }

            }

        }    /*extra credit if we get to it
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
//reads passed file from passed starting offset x # of bytes
void myReadFunc(int fd, int filePosOffset,int bytesToRead, int fileCluster, int file_size,int read_to_end,char *filename)
{

    findFatSequence(fd, fileCluster);
    unsigned char buffer[bytesToRead];
    int fileByteTracker = filePosOffset;

    int trackClust = 0;
    //lseek to file position to begin reading
    if ((lseek(fd, filePosOffset, SEEK_SET)) == -1){
        printf("Cannot seek fat32.img\n");
    }

    int clusterIndex_atFilePos = (int)(filePosOffset - findCluster(fileCluster))/512;
    int endClust = (int)((filePosOffset + bytesToRead) - findCluster(fileCluster))/512;
    endClust = fat[endClust];
    trackClust = fat[clusterIndex_atFilePos];

    //strings to hold read data to print
    unsigned char *buf3;
    int sizeOfReadBuff;


    int byteTrackerFile = filePosOffset;
    int whereToReadUpto = (findCluster(trackClust + 1));

    //if bytesToRead goes through more than one cluster
    if(endClust > trackClust) {
        while (1) {
            //end of cluster - position in file
            sizeOfReadBuff = whereToReadUpto - byteTrackerFile;

            buf3 = (unsigned char *) malloc(sizeof(unsigned char) * sizeOfReadBuff);
            //read to end of tracked cluster
            if (!(read(fd, buf3, ( whereToReadUpto - byteTrackerFile)) ==
                  (whereToReadUpto - byteTrackerFile))) {
                printf("ERROR: cannot read\n");
                exit(0);
            }
            fileByteTracker += sizeOfReadBuff;
            printf("%s\n", buf3);

            //if already read end clust
            if(trackClust == endClust || fileByteTracker == filePosOffset + bytesToRead)
                break;

            //increment tracked clust
            //dont go to next cluster if file already read all the way through
            if((clusterIndex_atFilePos + 1) < fatTrack)
            {
                trackClust = fat[clusterIndex_atFilePos + 1];
                clusterIndex_atFilePos++;
            }
            else
            {
                free(buf3);
                break;
            }

            //update position in file
            byteTrackerFile = findCluster(trackClust);
            //update where to read up to
            if(trackClust == endClust)
            {
                //read remaining bytes left to read
                whereToReadUpto = findCluster(trackClust) + (bytesToRead % 512);
            }
            else
            {
                //read full cluster if not at endCluster
                whereToReadUpto = findCluster(trackClust + 1);
            }




            //lseek to new cluster
            if ((lseek(fd, findCluster(trackClust), SEEK_SET)) == -1) {
                printf("Cannot seek fat32.img\n");
            }
            //free buffer data
            free(buf3);
        }

    }
    else{
        //read remaining bytes
        if(!(read(fd,&buffer, bytesToRead) == bytesToRead))
        {
            printf("ERROR: cannot read\n");
            exit(0);
        }
        printf("%s\n", buffer);
    }

    printf("\n");


    //return to current cluster
    if ((lseek(fd, findCluster(currentCluster), SEEK_SET)) == -1){
        printf("Cannot seek fat32.img\n");
    }
    findFatSequence(fd, currentCluster);
    getDir(fd, currentCluster);

    //change offset position of open file
    if(read_to_end == 0)
    {
        for(int x = 0; x < 100; x++)
        {
            if(strcmp(filename,op[x].filename) == 0)
            {
                op[x].offset_position = (filePosOffset - findCluster(fileCluster)) + bytesToRead;
                break;
            }
        }
    }

    else
    {
        for(int i = 0; i < dirTrack; i++)
        {
            if (strcmp(filename, dir[i].DIRName) == 0)
            {
                for (int x = 0; x < 100; x++)
                {
                    if(strcmp(filename,op[x].filename) == 0)
                    {
                        op[x].offset_position = dir[i].DIRSize;
                        break;
                    }
                }
                break;
            }
        }
    }


}
//moves file or directory to provided directory
void moveEntry(int desc, int index, int moveToCluster)
{
    struct DIRENTRY attempt;
    for(int i = 0; i < 11; i++){
        attempt.DIRName[i] = dir[index].DIRName[i];
    }
    attempt.DIRAttr = dir[index].DIRAttr;
    attempt.DIRNTRes = dir[index].DIRNTRes;
    attempt.DIRCTTenth = dir[index].DIRCTTenth;
    attempt.DIRCTime = dir[index].DIRCTime;
    attempt.DIRCDate = dir[index].DIRCDate;
    attempt.LADate = dir[index].LADate;
    attempt.highCluster = dir[index].highCluster;
    attempt.DIRWTime = dir[index].DIRWTime;
    attempt.DIRWDate = dir[index].DIRWDate;
    attempt.lowCluster = dir[index].lowCluster;
    attempt.DIRSize = dir[index].DIRSize;

    findFatSequence(desc, moveToCluster);
    getDir(desc, moveToCluster);
    int temp = 0;
    for(int i = 0; i < 16; i++){
        if((dir[i].DIRName[0] == (char) 0x0) || dir[i].DIRName[0] == (char) 0xe5){
            temp = i;
            break;
        }
    }

    if ((lseek(desc, (findCluster(moveToCluster)+(32*temp)), SEEK_SET)) == -1){
        printf("Cannot seek fat32.img\n");
    }

    if(!(write(desc,&attempt, 32) == 32))
    {
        printf("ERROR: cannot write %s\n", fileName);
        exit(0);
    }

}
//removes file from current directory
void remEntry(int desc, int off, int current){
    findFatSequence(desc, current);
    getDir(desc, current);

    currentCluster = current;
    //location of direntry in current cluster
    if ((lseek(desc, (findCluster(currentCluster)+(32*off)), SEEK_SET)) == -1){
        printf("Cannot seek fat32.img\n");
    }
    unsigned char checkName = (unsigned char) 0xe5;
    if(!(write(desc,&checkName, 1) == 1))
    {
        printf("ERROR: cannot write %s\n", fileName);
        exit(0);
    }

    unsigned char finishName[31];
    for(int i = 0; i < 31; i++){
        finishName[i] = (unsigned char) 0x0;
    }

    if(!(write(desc,&finishName, 31) == 31))
    {
        printf("ERROR: cannot write %s\n", fileName);
        exit(0);
    }

    //return to current cluster
    if ((lseek(desc, findCluster(currentCluster), SEEK_SET)) == -1){
        printf("Cannot seek fat32.img\n");
    }
    findFatSequence(desc, currentCluster);
    getDir(desc, currentCluster);

}

//check to see if mv arguments are valid
//returns 1 if its a legal move, 0 otherwise
int checkAttr(char* from, char* to)
{
    tokenlist *tok;
    unsigned int fromAttr;
    unsigned int toAttr;
    //get attribute of the 'from' arg
    for(int i = 0; i < dirTrack; i++)
    {
        tok = get_tokens(dir[i].DIRName);
        if(strcmp(from, tok->items[0]) == 0)
            fromAttr = dir[i].DIRAttr;
    }

    //get attribute of the 'to' arg
    for(int i = 0; i < dirTrack; i++)
    {
        tok = get_tokens(dir[i].DIRName);
        if(strcmp(to, tok->items[0]) == 0)
            toAttr = dir[i].DIRAttr;
    }
    //if the 'from' arg is a dir and 'to' is a file
    if(fromAttr == 0x10 && toAttr == 0x20)
        return 0;
    else
        return 1;
}
//removes directory from current directory
void remDir(int desc, unsigned int cluster, int off, int current){

    findFatSequence(desc, cluster);
    getDir(desc, cluster);
    unsigned int buffer[512];

    //define buffer indices to 0;
    for(int i = 0; i < 512; i++){
        buffer[i] = (unsigned char) 0x0;
    }

    //goes through cluster chain for removed file and turns all data to 0
    for(int i = 0; i < fatTrack; i++){

        //fat region location
        if ((lseek(desc, 16384 + (fat[i] * 4), SEEK_SET)) == -1) {
            printf("Cannot seek fat32.img\n");
        }

        unsigned int buf = (int)0x0;
        if(!(write(desc,&buf, 4) == 4))
        {
            printf("ERROR: cannot write %s\n", fileName);
            exit(0);
        }
        //data region location
        if ((lseek(desc, findCluster(fat[i]), SEEK_SET)) == -1){
            printf("Cannot seek fat32.img\n");
        }

        if(!(write(desc,&buffer, 512) == 512))
        {
            printf("ERROR: cannot write %s\n", fileName);
            exit(0);
        }
    }
    currentCluster = current;
    //location of direntry in current cluster
    if ((lseek(desc, (findCluster(currentCluster)+(32*off)), SEEK_SET)) == -1){
        printf("Cannot seek fat32.img\n");
    }
    unsigned char checkName = (unsigned char) 0xe5;
    if(!(write(desc,&checkName, 1) == 1))
    {
        printf("ERROR: cannot write %s\n", fileName);
        exit(0);
    }

    unsigned char finishName[31];
    for(int i = 0; i < 31; i++){
        finishName[i] = (unsigned char) 0x0;
    }

    if(!(write(desc,&finishName, 31) == 31))
    {
        printf("ERROR: cannot write %s\n", fileName);
        exit(0);
    }

    //return to current cluster
    if ((lseek(desc, findCluster(currentCluster), SEEK_SET)) == -1){
        printf("Cannot seek fat32.img\n");
    }
    findFatSequence(desc, currentCluster);
    getDir(desc, currentCluster);

}
//create snew file in current directory
void createFile(int fd,int cluster,char *filename)
{
    int nextCluster = cluster;
    int lastCluster;
    int emptyCluster;
    int lastCluster_offset;
    int emptyCluster_offset;
    int offset_track;
    int numCluster = 2;

    if ((lseek(fd, 16384 + (cluster * 4), SEEK_SET)) == -1) {
        printf("Cannot seek fat32.img\n");
    }
    offset_track = 16384 + (cluster * 4);   //set to root cluster offset

    while(1){

        //read next fat data
        if(!(read(fd, buff, 4) == 4))
        {
            printf("ERROR: cannot read %s\n", filename);
            exit(0);
        }
        nextCluster = 0;
        for(int i = 0; i < 4; i++){
            nextCluster += (unsigned int) buff[i];
        }

        //if not at end of cluster chain
        if( nextCluster < 268435448)
        {
            if((lseek(fd, 16384 + (nextCluster * 4), SEEK_SET)) == -1){
                printf("Cannot seek fat32.img\n");
            }
            offset_track = 16384 + (nextCluster * 4);
        }
            //if at EoC chain
        else
        {
            //at this point offset track is set to the fffffff EoC entry
            lastCluster_offset = offset_track;

            if(cluster > 2)
            {
                lastCluster = ((offset_track - 4) - 16384)/4;
                printf("last cluster offset: %d\n",16384 + (lastCluster * 4));

            }
            else
            {
                lastCluster = (offset_track - 16384)/4;
                printf("last cluster offset: %d\n",16384 + (lastCluster * 4));
            }

            memset(buff,0,4);

            //search fat in sequence now until empty block is found
            while(nextCluster != 0)
            {
                //read next immediate block
                if(!(read(fd, buff, 4) == 4))
                {
                    printf("ERROR: cannot read %s\n", filename);
                    exit(0);
                }
                offset_track+=4;

                nextCluster=0;
                for(int i = 0; i < 4; i++){
                    nextCluster += (unsigned int) buff[i];
                }
                if(nextCluster == 0)
                {
                    emptyCluster_offset = offset_track;
                    emptyCluster = (emptyCluster_offset - 16384) / 4;
                    printf("emptyCluster offset: %d\n",emptyCluster_offset);
                    break;
                }

                memset(buff,0,4);   //flush buffer
                numCluster++;
            }

            printf("empty cluster offset in data: %d\n",findCluster(emptyCluster));
            printf("Number of clusters: %d\n", numCluster);
            printf("Empty cluster: %d\n", emptyCluster);

            int totalSpace = (bpb.FATsize*bpb.BytesPerSec) - ((numCluster+1) * 4);
            printf("Total space in FAT: %d\n", totalSpace);

            //add new entry to fat array = cluster number of new entry in fat
            fat[fatTrack] = emptyCluster;
            fatTrack++;
            for(int i = 0; i < fatTrack;i++)
            {
                printf("fat entry: %d\n",fat[i]);
            }
            printf("last entry of fat array just added: %d\n",(emptyCluster_offset - 16384) / 4);
            //go to empty cluster offset
            if ((lseek(fd, emptyCluster_offset, SEEK_SET)) == -1) {
                printf("Cannot seek fat32.img\n");
            }
            //write to emptyCluster offset
            unsigned int buf = (int)0x0FFFFFF8;
            if(!(write(fd, &buf, 4) == 4))
            {
                printf("ERROR: cannot read %s\n", fileName);
                exit(0);
            }
            //go to data region
            if ((lseek(fd, (findCluster(currentCluster)+(32*dirTrack)), SEEK_SET)) == -1) {
                printf("Cannot seek fat32.img\n");
            }
            struct DIRENTRY dir_to_be_added;
            strcpy(dir_to_be_added.DIRName,filename);
            dir_to_be_added.DIRAttr = 0x20;
            dir_to_be_added.DIRSize = 0;
            dir_to_be_added.lowCluster = emptyCluster;
            dir_to_be_added.highCluster = 0;
            dir_to_be_added.DIRNTRes = 0;

            //write to data region
            if(!(write(fd,&dir_to_be_added, 32) == 32))
            {
                printf("ERROR: cannot write %s\n", fileName);
                exit(0);
            }

            //update dir list
            findFatSequence(fd, currentCluster);
            getDir(fd,currentCluster);

            //reset lseek
            if ((lseek(fd, findCluster(cluster), SEEK_SET)) == -1) {
                printf("Cannot seek fat32.img\n");
            }

            //print out prompt
            printf("%s created: %d bytes used, %d bytes remaining\n", filename, 4, totalSpace);
            break;

        }
    }
}
//creates new directory in current directory
void makeDir(int fd,int cluster,char *filename)
{
    int nextCluster = cluster;
    int lastCluster;
    int emptyCluster;
    int lastCluster_offset;
    int emptyCluster_offset;
    int offset_track;
    int numCluster = 2;

    if ((lseek(fd, 16384 + (cluster * 4), SEEK_SET)) == -1) {
        printf("Cannot seek fat32.img\n");
    }
    offset_track = 16384 + (cluster * 4);   //set to root cluster offset

    while(1){

        //read next fat data
        if(!(read(fd, buff, 4) == 4))
        {
            printf("ERROR: cannot read %s\n", filename);
            exit(0);
        }
        nextCluster = 0;
        for(int i = 0; i < 4; i++){
            nextCluster += (unsigned int) buff[i];
        }

        //if not at end of cluster chain
        if( nextCluster < 268435448)
        {
            if((lseek(fd, 16384 + (nextCluster * 4), SEEK_SET)) == -1){
                printf("Cannot seek fat32.img\n");
            }
            offset_track = 16384 + (nextCluster * 4);
        }
            //if at EoC chain
        else
        {
            //at this point offset track is set to the fffffff EoC entry
            lastCluster_offset = offset_track;

            if(cluster > 2)
            {
                lastCluster = ((offset_track - 4) - 16384)/4;
                printf("last cluster offset: %d\n",16384 + (lastCluster * 4));

            }
            else
            {
                lastCluster = (offset_track - 16384)/4;
                printf("last cluster offset: %d\n",16384 + (lastCluster * 4));
            }

            memset(buff,0,4);

            //search fat in sequence now until empty block is found
            while(nextCluster != 0)
            {
                //read next immediate block
                if(!(read(fd, buff, 4) == 4))
                {
                    printf("ERROR: cannot read %s\n", filename);
                    exit(0);
                }
                offset_track+=4;

                nextCluster=0;
                for(int i = 0; i < 4; i++){
                    nextCluster += (unsigned int) buff[i];
                }
                if(nextCluster == 0)
                {
                    emptyCluster_offset = offset_track;
                    emptyCluster = (emptyCluster_offset - 16384) / 4;
                    printf("emptyCluster offset: %d\n",emptyCluster_offset);
                    break;
                }

                memset(buff,0,4);   //flush buffer
                numCluster++;
            }

            printf("empty cluster offset in data: %d\n",findCluster(emptyCluster));
            printf("Number of clusters: %d\n", numCluster);
            printf("Empty cluster: %d\n", emptyCluster);

            int totalSpace = (bpb.FATsize*bpb.BytesPerSec) - ((numCluster+1) * 4);
            printf("Total space in FAT: %d\n", totalSpace);

            //add new entry to fat array = cluster number of new entry in fat
            fat[fatTrack] = emptyCluster;
            fatTrack++;
            for(int i = 0; i < fatTrack;i++)
            {
                printf("fat entry: %d\n",fat[i]);
            }
            printf("last entry of fat array just added: %d\n",(emptyCluster_offset - 16384) / 4);

            //go to empty cluster offset
            if ((lseek(fd, emptyCluster_offset, SEEK_SET)) == -1) {
                printf("Cannot seek fat32.img\n");
            }
            //write to emptyCluster offset
            unsigned int buf = (int)0x0FFFFFF8;

            if(!(write(fd, &buf, 4) == 4))
            {
                printf("ERROR: cannot read %s\n", fileName);
                exit(0);
            }
            //go to data region
            if ((lseek(fd, (findCluster(currentCluster)+(32*dirTrack)), SEEK_SET)) == -1) {
                printf("Cannot seek fat32.img\n");
            }
            struct DIRENTRY dir_to_be_added;
            strcpy(dir_to_be_added.DIRName,filename);
            dir_to_be_added.DIRAttr = 0x10;
            dir_to_be_added.DIRSize = 0;
            dir_to_be_added.lowCluster = emptyCluster;
            dir_to_be_added.highCluster = 0;
            dir_to_be_added.DIRNTRes = 0;

            //write to data region
            if(!(write(fd,&dir_to_be_added, 32) == 32))
            {
                printf("ERROR: cannot write %s\n", fileName);
                exit(0);
            }

            if ((lseek(fd, findCluster(emptyCluster), SEEK_SET)) == -1) {
                printf("Cannot seek fat32.img\n");
            }
			//writing . and .. directories to new directory
            struct DIRENTRY curDir;
            strcpy(curDir.DIRName,".");
            curDir.DIRAttr = 0x10;
            curDir.DIRSize = 0;
            curDir.lowCluster = emptyCluster;
            curDir.highCluster = 0;
            curDir.DIRNTRes = 0;

            if(!(write(fd,&curDir, 32) == 32))
            {
                printf("ERROR: cannot write %s\n", fileName);
                exit(0);
            }

            if ((lseek(fd, (findCluster(emptyCluster)+32), SEEK_SET)) == -1) {
                printf("Cannot seek fat32.img\n");
            }
            struct DIRENTRY prevDir;
            strcpy(prevDir.DIRName,"..");
            prevDir.DIRAttr = 0x10;
            prevDir.DIRSize = 0;
            prevDir.lowCluster = currentCluster;
            prevDir.highCluster = 0;
            prevDir.DIRNTRes = 0;

            if(!(write(fd,&prevDir, 32) == 32))
            {
                printf("ERROR: cannot write %s\n", fileName);
                exit(0);
            }

            //update dir list
            findFatSequence(fd, currentCluster);
            getDir(fd,currentCluster);

            //reset lseek
            if ((lseek(fd, findCluster(cluster), SEEK_SET)) == -1) {
                printf("Cannot seek fat32.img\n");
            }

            //print out prompt
            printf("%s created: %d bytes used, %d bytes remaining\n", filename, 4, totalSpace);
            break;

        }
    }
}
//displays files filesize
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
//populates the BPB struct
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
//the next 5 functions were provided from project 1
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

//returns data region offset for the passed in cluster number
int findCluster(int x){
    if(x < 2)
        return ((bpb.ResvSecCnt * bpb.BytesPerSec) + (bpb.numFATs * bpb.FATsize * bpb.BytesPerSec)) + ((2 - 2) * bpb.BytesPerSec);
    return ((bpb.ResvSecCnt * bpb.BytesPerSec) + (bpb.numFATs * bpb.FATsize * bpb.BytesPerSec)) + ((x - 2) * bpb.BytesPerSec);
}

//finds fat non-contiguous sequence
void findFatSequence(int fd, int cluster)
{
    int nextCluster;

    fatTrack=0;
    fat[fatTrack] = cluster;
    fatTrack++;

    //seek to start of fat sequence + offset
    //root offset is 8
    if((lseek(fd, 16384 + (cluster * 4), SEEK_SET)) == -1){
        printf("Cannot seek fat32.img\n");
    }

    //start populating fat array
    while(1){
        if(!(read(fd, buff, 4) == 4))
        {
            printf("ERROR: cannot read %s\n", fileName);
            exit(0);
        }
        nextCluster = 0;
        for(int i = 0; i < 4; i++){
            nextCluster += (unsigned int) buff[i];
        }
        if( nextCluster < 268435448)
        {
            fat[fatTrack] = nextCluster;
            if((lseek(fd, 16384 + (nextCluster * 4), SEEK_SET)) == -1){
                printf("Cannot seek fat32.img\n");
            }
            fatTrack++;
        }
        else
            break;
    }
    //reset buff
    memset(buff,0,4);
}
//ls the current directory
void listDir(int desc, int cluster)
{
    if(cluster > currentCluster){
        int offset;
        findFatSequence(desc,cluster);
        for(int i = 0; i < fatTrack; i++)
        {
            //hop to next entry in fat
            offset = findCluster(fat[i]);
            if((lseek(desc, offset, SEEK_SET)) == -1)
                printf("Cannot seek fat32.img\n");

            //loop fills dir[] with a directory in each slot or until done
            //need file names still
            for(int x = 0; x < 16; x++)
            {
                if(!(read(desc, &dir[x], 32) == 32))
                    printf("Cannot read fat32.img\n");
                //checks if Name and Attribute of directory exist
                if ((dir[x].DIRName[0] != (char)0xe5) &&
                    (dir[x].DIRAttr == 0x1 || dir[x].DIRAttr == 0x10 || dir[x].DIRAttr == 0x20))
                {
                    printf("%s\n", dir[x].DIRName);
                }
            }


        }

        //reset everything back to current cluster settings
        offset = findCluster(currentCluster);
        if((lseek(desc, offset, SEEK_SET)) == -1)
            printf("Cannot seek fat32.img\n");
        findFatSequence(desc,currentCluster);
        getDir(desc, currentCluster);


    }
    else{
        for(int i = 0; i < fatTrack; i++)
        {
            //hop to next entry in fat
            if((lseek(desc,findCluster(fat[i]), SEEK_SET)) == -1)
                printf("Cannot seek fat32.img\n");

            for(int x = 0; x < 16; x++)
            {
                if(!(read(desc, &dir[x], 32) == 32))
                    printf("Cannot read fat32.img\n");
                if ((dir[x].DIRName[0] != (char)0xe5) &&
                    (dir[x].DIRAttr == 0x1 || dir[x].DIRAttr == 0x10 || dir[x].DIRAttr == 0x20))
                {
                    printf("%s\n",dir[x].DIRName);
                }
            }


        }
    }
}

void getDir(int desc, int cluster){
    //int offset = findCluster(cluster);
    //int clusterTracker = cluster;

    for(int i = 0; i < fatTrack; i++)
    {
        //hop to next entry in fat
        if((lseek(desc,findCluster(fat[i]), SEEK_SET)) == -1)
            printf("Cannot seek fat32.img\n");

        for(int x = 0; x < 16; x++)
        {
            if(!(read(desc, &dir[x], 32) == 32))
                printf("Cannot read fat32.img\n");
            if ((dir[x].DIRName[0] != (char)0xe5) &&
                (dir[x].DIRAttr == 0x1 || dir[x].DIRAttr == 0x10 || dir[x].DIRAttr == 0x20))
            {
                dirTrack = x + 1;
            }
        }
    }
}

void getBig(int desc, int cluster){
    //int offset = findCluster(cluster);
    //int clusterTracker = cluster;
    int value;
    value = 0;
    for(int i = 0; i < fatTrack; i++)
    {
        //hop to next entry in fat
        if((lseek(desc,findCluster(fat[i]), SEEK_SET)) == -1)
            printf("Cannot seek fat32.img\n");

        for(int x = value; x < 208; x++)
        {
            if(!(read(desc, &redDir[x], 32) == 32))
                printf("Cannot read fat32.img\n");
            if ((redDir[x].DIRName[0] != (char)0xe5) &&
                (redDir[x].DIRAttr == 0x1 || redDir[x].DIRAttr == 0x10 || redDir[x].DIRAttr == 0x20))
            {
                redTrack = x + 1;
            }
            value++;
            if(value % 16 == 0)
                break;
        }
    }
}
/*
void copyFile(int desc, int newClust, int origClust, int cpToClust)
{
	unsigned char cp[512];
	findFatSequence(desc, origClust);
	for(int i = 1; i < fatTrack; i++){
		//seeks to data to be copied
		if((lseek(desc,findCluster(fat[origClust]), SEEK_SET)) == -1)
            printf("Cannot seek fat32.img\n");

		if(!(read(desc, &cp, 512) == 512))
                printf("Cannot read fat32.img\n");

		//seeks to data region for copied data to be written to
		if((lseek(desc,findCluster(newClust), SEEK_SET)) == -1)
            printf("Cannot seek fat32.img\n");

		if(!(write(desc, &cp, 512) == 512))
                printf("Cannot read fat32.img\n");




}*/
