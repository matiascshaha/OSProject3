else if(strcmp(tokens->items[0], "cp") == 0)
        {
            int temp = 0;
            if(tokens->size < 3)
            {
                printf("ERROR: wrong number of args\n");
                continue;
            }

            //check to see if source is file. Writeup says CP can only copy files
            int source_file_cluster;
            error = 1;
            for(int i = 0; i < dirTrack;i++)
            {
                if(strcmp(dir[i].DIRName,tokens->items[1]) == 0)
                {
                    //if from argument is a directory -> error
                    if(dir[i].DIRAttr == 0x10)
                    {
                        printf("Cannot move directory: invalid source argument");
                        error = 0;
                        break;
                    }
                    else
                    {
                        //valid case
                        source_file_cluster = dir[i].lowCluster;
                    }

                }
                else if(i == dirtrack - 1)
                {
                    printf("ERROR: FILENAME arg %s doesn't exist\n", tokens->items[1]);
                    error = 0;
                    break;
                }
            }
            if(error == 0)
                continue;


            for(int i = 0; i < dirTrack;i++)
            {
                if(strcmp(dir[i].DIRName,tokens->items[2]) == 0)
                {
                    //if to argument is a file
                    if(dir[i].DIRAttr == 0x20)
                    {
                        printf("ERROR: cannot rename file to existing file\n");
                        break;
                    }
                    else if(dir[i]DIRAttr == 0x10)
                    {
                        //call cpy func here
                        myCpyFunc(fd,source_file_cluster, tokens->items[1],tokens->items[2],i);
                        break;
                    }

                }
                else if (i == dirTrack - 1)
                {
                    //call cpy here
                    //cpy to current directory with toArg as name
                    myCpyFunc(fd,source_file_cluster, tokens->items[1],tokens->items[2],-1);
                }

            }









        } 
        
        
        
        void myCpyFunc(fd, int srcfileCluster, char * from, char * to, int index_file)
{
    //grab fat for file
    findFatSequence(fd, srcFileClusteer);
    int track = 0;
    unsigned int buf;


    int newFatSeq[fatTrack];
    int newFatSeqTrack = fatTrack;

    for(int i; i < fatTrack; i++)
    {
        while(1)
        {
            //printf("TEST\n");
            //seek to start of FAT
            lseek(fd, 16384+track, SEEK_SET);
            //read 4 bytes
            read(fd, &buf, 4);
            if(buf == 0)
            {
                clusterNum = track/4;
                printf("Cluster num: %X\n", clusterNum);

                trackFAT++;
                break;
            }
            track += 4;
        }

        //adds the new cluster and makes the empty cluster the new end
        newFatSeq[i] = clusterNum;
        lseek(fd, 16384 + newFatSeq[i]*4, SEEK_SET);


        buf = (int)0x0FFFFFF8;
        write(fd, &buf, 4);
    }

    //write to fat
    for(i = 0; i < newFatSeqTrack - 1; i++)
    {
        lseek(fd,16384 + newFatSeq[i] *4,SEEK_SET);
        buf = (unsigned int) newFatSeq[i + 1];

        write(fd, &buf,4);


    }

    //write to data region
    struct DIRENTRY dir_to_be_added;
    dir_to_be_added.lowCluster = newFatSeq[0];
    strcpy(dir_to_be_added.DIRName,from);
    dir_to_be_added.DIRAttr = 0x20;

    unsigned char buff_write[512];

    //copy and write to data region
    for(int i = 0;i < newFatSeqTrack; i++)
    {
        lseek(fd,findCluster(fat[i]),SEEK_SET);
        read(fd, buff_write,512);

        lseek(fd,findCluster(newFatSeq[i]),SEEK_SET);
        write(fd,buff_write,512);
    }

    if(index_file != -1)
    {
        findFatSequence(fd,dir[index_file].lowCluster);
        getDir(fd,dir[index_file].lowCluster);
    }

    int temp = 0;
    for(int i = 0; i < 16; i++){
        if((dir[i].DIRName[0] == (char) 0x0) || dir[i].DIRName[0] == (char) 0xe5){
            temp = i;
            break;
        }
    }

    if(index_file != -1)
    {
        lseek(desc, (findCluster(dir[index_file].lowCluster)+(32*temp)), SEEK_SET);
        write(desc,&dir_to_be_added, 32);
    }
    else
    {
        lseek(desc, (findCluster(currentCluster)+(32*temp)), SEEK_SET);
        write(desc,&dir_to_be_added, 32);
    }


    //return to current cluster
    if ((lseek(fd, findCluster(currentCluster), SEEK_SET)) == -1){
        printf("Cannot seek fat32.img\n");
    }
    findFatSequence(fd, currentCluster);
    getDir(fd, currentCluster);





}
