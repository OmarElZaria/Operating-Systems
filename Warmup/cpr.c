#include "common.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <dirent.h>
#include <syscall.h>


/* make sure to use syserror() when a system call fails. see common.h */

void copy_file(char* file_source, char* file_destination){
    char buf[100000];
    int fd1, fd2, ret, out;
    
    //open source folder
    fd1 = open(file_source, O_RDONLY);

    if(fd1 < 0){
    
        syserror(open, file_source);
    
    }else{
        //create destination folder
        fd2 = creat(file_destination, 0777);

        if(fd2 < 0){
            
            syserror(creat, file_destination);
        
        }else{
            
            //read contents of source file
            ret = read(fd1, buf, 100000);
            
            if(ret < 0){
                
                syserror(read, file_source);
        
            }else{
                //write contents of source file to destination file
                out = write(fd2, buf, ret);
                
                if(out < 0){
                
                    syserror(write, file_destination);
                }
            }
            
            //close both folders
            close(fd1);
            close(fd2);
        }
    }
}

void copy_directory(char* src_directory, char* dest_directory){
    
    DIR* dp = NULL;
    struct stat buffer;
    struct stat temp;
    struct dirent* dptr;
    
    int ret;
    
    //open the source directory
    dp = opendir(src_directory);    
    
    //make a new directory in order to copy files into
    ret = mkdir(dest_directory, 0777);

    if(ret < 0){

        syserror(mkdir, dest_directory);

    }
    
    //check if source directory is empty or not
    if(dp){
        
        while( (dptr = readdir(dp)) != NULL){
            
            if (strcmp(dptr->d_name,".") == 0 || strcmp(dptr->d_name, "..") == 0) continue;
            
            //creates the full path of a directory or a file
            char* full_srcpath = (char*)malloc(1000*sizeof(char));
            char* full_destpath = (char*)malloc(1000*sizeof(char));
            
            strcpy(full_srcpath, src_directory);
            strcpy(full_destpath, dest_directory);
            strcat(full_srcpath,"/");
            strcat(full_destpath,"/");
            strcat(full_srcpath, dptr->d_name);
            strcat(full_destpath, dptr->d_name);

            
            //checks the status of the source path
            ret = stat(full_srcpath, &buffer);

            //if the source path is a directory, call the copy directory function again recursively until a file is reached
            if(S_ISDIR(buffer.st_mode)){
                
                copy_directory(full_srcpath, full_destpath);

            }
            else if(S_ISREG(buffer.st_mode)){
                
                //copies file into new directory
                copy_file(full_srcpath, full_destpath);

                //copies permissions for each file  
                ret = stat(full_srcpath, &temp);
                
                if(ret < 0)
                    syserror(stat, full_srcpath);
                
                ret = chmod(full_destpath, temp.st_mode);
                
                if(ret < 0)
                    syserror(chmod, full_destpath);
            }
            

            free(full_srcpath);
            free(full_destpath);
        }
        
        
    }
    
    //copies permissions from old directory into new directory
    ret = stat(src_directory, &buffer);
    
    if(ret < 0){
        
        syserror(stat, src_directory);
    
    }
    
    ret = chmod(dest_directory, buffer.st_mode);
    if(ret < 0){

       syserror(chmod, dest_directory);

    }
    
    closedir(dp);
}

void
usage()
{
	fprintf(stderr, "Usage: cpr srcdir dstdir\n");
	exit(1);
}

int
main(int argc, char *argv[])
{
        if (argc != 3) {
		usage();
	}else{
            copy_directory(argv[1], argv[2]);
        }
	return 0;
}
