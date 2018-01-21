# image_retrieval
A program for comparing similarity betweem images with the use of multi-processing.

The main program will be provided with the name of a ppm image file and the name of the directory where we want to look for similar images. The program will search the directory for sub-directories and fork a new process for each sub-directory. Each child process will compare the original image to all the images in the sub-directory that it was assigned. It will then send to the parent process information on the most similar image in its sub-directory.
Similarity between two images is based on the Euclidean distance between two pixels and for simplicity work only with images that have the same dimension (same width and height). i.e. it will compute all the pairwise Euclidean distances between the corresponding pixels in the two images and then return the average of all these distances.

Enter "make" compile:

After compilation, there will be 2 executables: oneprocess, and image_retrieval.
They do the same thing, except "image_retrieval" is the parallel version of "oneprocess", and the function process_dir will be run by a child process that the parent process forked, then uses pipe to communicate back to the parent.

This program can be run from the command line as follows:

./one_process -d DIR_NAME IMAGE_FILE 
or
./image_retrieval -d DIR_NAME IMAGE_FILE 

IMAGE_FILE is the name of the image file that we compare all other files to. The -d option provides the name of the directory, where we search for similar images. This option is optional and the program will simply search the current working directory if it is not provided. When completed the program should look for images in all the sub-directories of DIR_NAME and find the one that is most similar to IMAGE_FILE. 

Example command to run the program with test files in the folder:

./one_process -d test/ to_be_compard.ppm 
