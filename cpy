else if(strcmp(tokens->items[0], "cp") == 0)
        {
            int temp = 0;
            if(tokens->size < 3)
            {
                printf("ERROR: wrong number of args\n");
                continue;
            }

            //check to see if source is file. Writeup says CP can only copy files
            for(int i = 0; i < dirTrack;i++)
            {
                if(strcmp(dir[i].DIRName,tokens->items[1]) == 0)
                {
                    //if from argument is a directory -> error
                    if(dir[i].DIRAttr == 0x10)
                    {
                        printf("Cannot move directory: invalid source argument");
                        continue;
                    }

                }
                else if(i == dirtrack - 1)
                {
                    printf("ERROR: FILENAME arg %s doesn't exist\n", tokens->items[1]);
                }
            }

            
            for(int i = 0; i < dirTrack;i++)
            {
                if(strcmp(dir[i].DIRName,tokens->items[2]) == 0)
                {
                    //if to argument is a file
                    if(dir[i].DIRAttr == 0x20)
                    {
                        printf("ERROR: cannot rename file to existing file\n");
                    }
                    else if(dir[i]DIRAttr == 0x10)
                    {
                        //call cpy func here
                    }

                }
                else if (i == dirTrack - 1)
                {
                    //call cpy here 
                    //cpy to current directory with toArg as name
                }

            }

            







        }
