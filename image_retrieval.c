#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>
#include <float.h>
#include "worker.h"


int total_sub_dir(char *dir);

int main(int argc, char **argv) {
	char ch;
	char path[PATHLENGTH];
	char *startdir = ".";
        char *image_file = NULL;

	while((ch = getopt(argc, argv, "d:")) != -1) {
		switch (ch) {
			case 'd':
			startdir = optarg;
			break;
			default:
			fprintf(stderr, "Usage: queryone [-d DIRECTORY_NAME] FILE_NAME\n");
			exit(1);
		}
	}

        if (optind != argc-1) {
	     fprintf(stderr, "Usage: queryone [-d DIRECTORY_NAME] FILE_NAME\n");
        } else
             image_file = argv[optind];

	// Open the directory provided by the user (or current working directory)
	
	DIR *dirp;
	if((dirp = opendir(startdir)) == NULL) {
		perror("opendir");
		exit(1);
	} 
	
	/* For each entry in the directory, eliminate . and .., and check
	* to make sure that the entry is a directory, then call run_worker
	* to process the image files contained in the directory.
	*/
		
	struct dirent *dp;
        
        // The image being that is being compared to everything else,
        // Note that this is an image pointer! not image
        Image* comp_image = read_image(image_file);

        CompRecord CRec;
        CRec.distance = FLT_MAX;
		
        int pipe_fd[total_sub_dir(startdir)][2];
        int i = 0;

	while((dp = readdir(dirp)) != NULL) {

		if(strcmp(dp->d_name, ".") == 0 || 
		   strcmp(dp->d_name, "..") == 0 ||
		   strcmp(dp->d_name, ".svn") == 0){
			continue;
		}
		strncpy(path, startdir, PATHLENGTH);
		strncat(path, "/", PATHLENGTH - strlen(path) - 1);
		strncat(path, dp->d_name, PATHLENGTH - strlen(path) - 1);


		struct stat sbuf;
		if(stat(path, &sbuf) == -1) {
			//This should only fail if we got the path wrong
			// or we don't have permissions on this entry.
			perror("stat");
			exit(1);
		} 

		// Only call process_dir if it is a directory
		// Otherwise ignore it.
		if(S_ISDIR(sbuf.st_mode)) { 
			//Before we call fork, call pipe
            if ((pipe(pipe_fd[i])) == -1){
				perror("pipe");
				exit(1);
			}
			int result = fork();
			if (result < 0){
				perror("fork");
				exit(1);
			}

			//else if children process
			//Note that this is only being ran once, and it doesn't
			//fork any more children
			else if (result == 0){ 

				//--------------------Setting up pipe--------------------------
				// child only writes to the pipe, so close the reading end
				if (close(pipe_fd[i][0]) == -1){
					perror("pipe");
					exit(1);
				}
				//Before we forked, parent had open the reading ends to 
				//all perviously forked children; so close those
				int child_no;
				for(child_no = 1; child_no < i; child_no++){
					if(close(pipe_fd[child_no][0]) == -1){
						perror("close reading ends of perviously forked children");
					}

				}
				//-----------------Finish setting up the pipe----------------------

				//I am writing back to the parent process FROM process_dir directly
				//by giving it the file descriptor that it's suppose to write to
				process_dir(path, comp_image, pipe_fd[i][1]); 
				
				if(close(pipe_fd[i][1] == -1)){
					perror("close pipe after writing");
				}
				//Don't fork children on next loop iteration
				exit(0);
            }
            //else the parent process
            else{
				if(close(pipe_fd[i][1]) == -1){
					perror("close writing end of pipe in parent");
				}
			}
			//increment i, for creating a new pipe for the new child
	        i += 1;

		}
	}


	int j;
	CompRecord temp_CRec;
	//close previous pipes
	for(j = 0; j < i; j++){
		if(read(pipe_fd[j][0], &temp_CRec, sizeof(CompRecord)) == -1){
			perror("reading from pipe from a child");
		}
		if(temp_CRec.distance < CRec.distance){
			CRec.distance = temp_CRec.distance;
            // They are the same type, the max length of the strings are the same
            strcpy(CRec.filename, temp_CRec.filename);
        }
    }
    printf("The most similar image is %s with a distance of %f\n", CRec.filename, CRec.distance);
	
	return 0;
}


/*
* Returns the total number of directories (sub-directories) in the given directory dir
*/
int total_sub_dir(char *dir) {
	int total=0;
	char path[128];
	DIR *dirp;
	if((dirp = opendir(dir)) == NULL) {
		perror("opendir");
		exit(1);
	} 
	
	/* For each entry in the directory, eliminate . and .., and check
	* to make sure that the entry is a directory, then call run_worker
	* to process the image files contained in the directory.
	*/
		
	struct dirent *dp;

	while((dp = readdir(dirp)) != NULL) {

		if(strcmp(dp->d_name, ".") == 0 || 
		   strcmp(dp->d_name, "..") == 0 ||
		   strcmp(dp->d_name, ".svn") == 0){
			continue;
		}
		strncpy(path, dir, PATHLENGTH);
		strncat(path, "/", PATHLENGTH - strlen(path) - 1);
		strncat(path, dp->d_name, PATHLENGTH - strlen(path) - 1);

		struct stat sbuf;
		if(stat(path, &sbuf) == -1) {
			perror("stat");
			exit(1);
		} 

		// Only call process_dir if it is a directory
		// Otherwise ignore it.
		if(S_ISDIR(sbuf.st_mode)) {
			total ++;
		}
	}
	return total;
}





