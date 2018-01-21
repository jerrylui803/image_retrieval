#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>
#include <float.h>
#include "worker.h"


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
        // 	Note that this is an image pointer! not image
        Image* comp_image = read_image(image_file);
        CompRecord CRec;
        CRec.distance = FLT_MAX;
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
                        CompRecord temp = process_dir(path, comp_image, STDOUT_FILENO); // This should be in children process
                        //if parent do
                        	//get the compRecord from the pipe
                        //if children do
                        	//call process_dir
                        if(temp.distance < CRec.distance){
                        	CRec.distance = temp.distance;
                        	// They are the same type, the max length of the strings are the same
                        	strcpy(CRec.filename, temp.filename);
                        }
		}
	}
        printf("The most similar image is %s with a distance of %f\n", CRec.filename, CRec.distance);	
	return 0;
}
