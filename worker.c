#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <math.h>
#include <float.h>
#include "worker.h"

float read_header(FILE *file, Image *img, float *comp);
float read_body(FILE *file, Image *img, int *total_pixels);
float check_valid_p3_ppm(FILE *file);


/*
 * Read an image from a file and create a corresponding 
 * image struct 
*/
Image* read_image(char *filename)
{
        FILE *my_file = fopen(filename, "r");
        if (my_file == NULL){
            fprintf(stderr, "No such file\n");
            exit(1);
        }
        //Allocate more space to check if the string is longer than "P3" + sizeof('\0')
        int row, col, max_colour;
        //Scan the header of the file to get col and row number and other information to
        //initilize the for loop below
        int i, j, red, green, blue;
        //In case it overflows p3
        if (check_valid_p3_ppm(my_file) != 0){
            fprintf(stderr, "read_image: P3 header missing\n"); //Changed by jerry, so its more clear 
            return NULL;
        }
		fscanf(my_file, "%d%d%d", &col, &row, &max_colour);
        Image *img = malloc(sizeof(Image));
        img->width = col;
        img->height = row;
        img->max_value = max_colour;
        //array of type(struct Pixel)s
        img->p = malloc((col * row) * (sizeof(Pixel)));
        //Read the image and put into struct Image
        for (i = 0; i < row; i++){
            for(j = 0; j < col; j++){
                int check;
                check = fscanf(my_file, "%d %d %d ", &red, &green, &blue);
                if(check != 3){
                    fprintf(stderr, "fscanf error\n");
                    exit(1);
                }
                //[col * i + j] is the same as having another variable for iterating the array
                ((img->p)[col * i + j]).red = red;
                ((img->p)[col * i + j]).green = green;
                ((img->p)[col * i + j]).blue = blue;
            }
        }
        return img;
}

/*
 * Print an image based on the provided Image struct 
 */

void print_image(Image *img){
        printf("P3\n");
        printf("%d %d\n", img->width, img->height);
        printf("%d\n", img->max_value);
       
        for(int i=0; i<img->width*img->height; i++)
           printf("%d %d %d  ", img->p[i].red, img->p[i].green, img->p[i].blue);
        printf("\n");
}

/*
 * Compute the Euclidian distance between two pixels 
 */
float eucl_distance (Pixel p1, Pixel p2) {
       return sqrt( pow(p1.red - p2.red,2 ) + pow( p1.blue - p2.blue, 2) + pow(p1.green - p2.green, 2));
}

 
/*
 * Compute the average Euclidian distance between the pixels 
 * in the image provided by img1 and the image contained in
 * the file filename
 */

float compare_images(Image *img1, char *filename) {
	FILE *file;
	float comp;
	int total_pixels;
	file = fopen(filename, "r");
	if (file == NULL) {
		fprintf(stderr, "Could not open file\n");
		exit(1); 
	}
	//check if file starts with P3
	if (read_header(file, img1, &comp) != 0) { 
		return FLT_MAX; 
	}
	//If file started with P3 (i.e. it isn't FLT_MAX already), use
	//Euclidian method to figure out distance
	if (comp != FLT_MAX){
		total_pixels = (img1->width)*(img1->height);
		comp = read_body(file, img1, &total_pixels);
	}
	if (fclose(file) != 0) {
		fprintf(stderr, "Could not close file\n");
		exit(1); 
	}
    return comp;
}


/*
* Read the header of the given file and determines whether it starts with P3 and has the same
* height and width as of the image being compared to
* @param file File of the image that needs to be processed
* @param img Image to which the image in file is being processed
* @param comp Distance between img and image in file
*/
float read_header(FILE *file, Image *img, float *comp){
	int width, height, colors;
	if (check_valid_p3_ppm(file) != 0) {
		return 1;
	}
	fscanf(file,"%d%d%d", &width, &height, &colors);
	if (width != img->width || height != img->height) {
		*comp = FLT_MAX;
	} 
	return 0;
}

/*
* Checks whether the file starts with P3
*/
float check_valid_p3_ppm(FILE *file){
	int read_var;
	char magic_num[3];
	read_var = fscanf(file,"%2s", magic_num);
	if (read_var != 1 || strcmp(magic_num, "P3") != 0) {
		return 1;
	}
	return 0;
}


/*
* Reads the body (pixels in the image) of a valid ppm file (i.e. file must start with P3) and
* returns the distance between the given img and img in file
*
* @param file File who's body is to be read
* @param img Image to which the image in file is being compared to
* @param total_pixels Total pixels to be read in the body
* REQ: file must be valid ppm (must start with P3) and be open for reading
* REQ: Header must already be read
* REQ: colors in the file must be in the order: Red Green Blue (RGB)
*/
float read_body(FILE *file, Image *img, int *total_pixels) {
	float comp = 0;
	int at_pixel = 0;
	Pixel curr_pixel;
	int at_color;
	int curr_num;
	//loop through as many pixels as specified (by total_pixels) of the given file
	for (;at_pixel < *total_pixels; at_pixel++) {
		//For each pixel, read all three colours (RGB)
		//Determine which colour is being read, and add it to Pixel struct accordingly
		// (Note: RGB order is being assumed)
		at_color = 0;
		for (;at_color <3; at_color ++){
			fscanf(file, "%d", &curr_num);
			if (at_color == 0) {
				curr_pixel.red = curr_num;
			}else if (at_color == 1) {
				curr_pixel.green = curr_num;
			} else {
				curr_pixel.blue = curr_num;
			}
		}
		//once the pixel is read, pass it, along with image being commpared's pixel to
		// eucl_distance func to calculate euclidean distance
		comp += eucl_distance ((img->p)[at_pixel], curr_pixel);
	}
	return comp/(*total_pixels);
}
 	

/* process all files in one directory and find most similar image among them
* - open the directory and find all files in it 
* - for each file read the image in it 
* - compare the image read to the image passed as parameter 
* - keep track of the image that is most similar 
* - write a struct CompRecord with the info for the most similar image to out_fd
*/
CompRecord process_dir(char *startdir, Image *img, int out_fd){

        CompRecord CRec;
        CRec.distance = FLT_MAX;
	char path[PATHLENGTH];
	DIR *dirp;
	if((dirp = opendir(startdir)) == NULL) {
		perror("process_dir: opendir");
		exit(1);
	} 
	
	/* For each entry in the directory, eliminate . and .., and check
	* to make sure that the entry is a directory, then call run_worker
	* to process the image files contained in the directory.
	*/
		
	struct dirent *dp;
	float next_distance;

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
			perror("process_dir: stat\n");
			exit(1);
		} 
		// Only process if it's a file
		if(S_ISREG(sbuf.st_mode)){
			next_distance = compare_images(img, path);
			if (next_distance < CRec.distance){
				CRec.distance = next_distance;
				strcpy(CRec.filename, path);
			}
		}	
	}
	write(out_fd, &CRec, sizeof(CompRecord));
        return CRec;
}
