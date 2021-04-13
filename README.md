# OSProject_3
 Implementing a FAT32 File System
 
 Andres Gonzalez | Matias Perichon | Kyle Wise 

Division of Labor: 

        Kyle Wise: info,size,ls,cd, mkdir,read, open, cp, rm, mv, rmdir 

        Matias Perichon: Creat Function,size, read, lseek, cp, info

        Andres Gonzalez:size, ls, cd, write, close, cp, rm, mv, overall case insensitivity
        
        ***** we worked on a lot of the code together as a group through discord screen share *****
        
Tar File:

        proj3.c (main File): contains all the logic and impementation of project functionality. Contains all 15 functions listed in project writeup. One big while loop in main with 15 if conditions nested inside that contain functionality.
        
        README: contains all special need-to-know information about our project, such as division of labor, file content, bugs,etc.
        
        Makefile: compiles proj3.c and renames to executable project3.
            - first thing to do is run command make
            - then Must be ran like so once compiled: project3 fat32.img

Bugs: 

        read:
         - When trying to read to a file in GREEN, errors for not existing or errors in writing occur.
        
        write:
         - When trying to write to a file in GREEN, errors for not existing or errors in writing occur.
         - Trying to overwrite currently used bytes, the code sometimes writes just the first word in the string.
    
Special Considerations: 

       - Whenever you read or write the offset position of the file is changed, so if you want to read the beginning of a file after you have to make sure to lseek FILENAME 0 to go  back to the beginning position.
