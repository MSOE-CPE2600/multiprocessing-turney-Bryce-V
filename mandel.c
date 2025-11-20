/// 
//  mandel.c
//  Based on example code found here:
//  https://users.cs.fiu.edu/~cpoellab/teaching/cop4610_fall22/project3.html
//
//  Converted to use jpg instead of BMP and other minor changes
//  
///
#define _POSIX_C_SOURCE 200809L

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <signal.h>
#include <wait.h>
#include <string.h>
#include <math.h>
#include "jpegrw.h"
#define _USE_MATH_DEFINES

// local routines
static int iteration_to_color( int i, int max );
static int iterations_at_point( double x, double y, int max );
static void compute_image( imgRawImage *img, double xmin, double xmax,
									double ymin, double ymax, int max );
static void show_help();

// NEW FUNCTION PROTOTYPE
static int run_movie_mode(char *progpath, int num_children,
                          double xcenter, double ycenter,
                          double xscale, int image_width,
                          int image_height, int max_iter,
                          const char *outfile_prefix);

int main( int argc, char *argv[] )
{
	char c;

	// These are the default configuration values used
	// if no command line arguments are given.
	const char *outfile = "mandel.jpg";
	double xcenter = 0;
	double ycenter = 0;
	double xscale = 4;
	double yscale = 0; // calc later
	int    image_width = 1000;
	int    image_height = 1000;
	int    max = 1000;

    // NEW: number of processes for movie mode
    int num_children = 0;

	// For each command line argument given,
	// override the appropriate configuration value.

	while((c = getopt(argc,argv,"x:y:s:W:H:m:o:hp:"))!=-1) {
		switch(c) 
		{
			case 'x':
				xcenter = atof(optarg);
				break;
			case 'y':
				ycenter = atof(optarg);
				break;
			case 's':
				xscale = atof(optarg);
				break;
			case 'W':
				image_width = atoi(optarg);
				break;
			case 'H':
				image_height = atoi(optarg);
				break;
			case 'm':
				max = atoi(optarg);
				break;
			case 'o':
				outfile = optarg;
				break;

            // NEW OPTION
            case 'p':
                num_children = atoi(optarg);
                break;

			case 'h':
				show_help();
				exit(1);
				break;
		}
	}

    // NEW: If -p was supplied, run movie mode and exit
    if (num_children > 0) {
        return run_movie_mode(argv[0], num_children, xcenter, ycenter,
                              xscale, image_width, image_height,
                              max, outfile);
    }

	// Calculate y scale based on x scale (settable) and image sizes in X and Y (settable)
	yscale = xscale / image_width * image_height;

	// Display the configuration of the image.
	printf("mandel: x=%lf y=%lf xscale=%lf yscale=%1f max=%d outfile=%s\n",xcenter,ycenter,xscale,yscale,max,outfile);

	// Create a raw image of the appropriate size.
	imgRawImage* img = initRawImage(image_width,image_height);

	// Fill it with a black
	setImageCOLOR(img,0);

	// Compute the Mandelbrot image
	compute_image(img,xcenter-xscale/2,xcenter+xscale/2,ycenter-yscale/2,ycenter+yscale/2,max);

	// Save the image in the stated file.
	storeJpegImageFile(img,outfile);

	// free the mallocs
	freeRawImage(img);

	return 0;
}

/*
Return the number of iterations at point x, y
in the Mandelbrot space, up to a maximum of max.
*/

int iterations_at_point( double x, double y, int max )
{
	double x0 = x;
	double y0 = y;

	int iter = 0;

	while( (x*x + y*y <= 4) && iter < max ) {

		double xt = x*x - y*y + x0;
		double yt = 2*x*y + y0;

		x = xt;
		y = yt;

		iter++;
	}

	return iter;
}

/*
Compute an entire Mandelbrot image, writing each point to the given bitmap.
Scale the image to the range (xmin-xmax,ymin-ymax), limiting iterations to "max"
*/

void compute_image(imgRawImage* img, double xmin, double xmax, double ymin, double ymax, int max )
{
	int i,j;

	int width = img->width;
	int height = img->height;

	// For every pixel in the image...

	for(j=0;j<height;j++) {

		for(i=0;i<width;i++) {

			// Determine the point in x,y space for that pixel.
			double x = xmin + i*(xmax-xmin)/width;
			double y = ymin + j*(ymax-ymin)/height;

			// Compute the iterations at that point.
			int iters = iterations_at_point(x,y,max);

			// Set the pixel in the bitmap.
			setPixelCOLOR(img,i,j,iteration_to_color(iters,max));
		}
	}
}


/*
Convert a iteration number to a color.
Here, we just scale to gray with a maximum of imax.
Modify this function to make more interesting colors.
*/
int iteration_to_color( int iters, int max )
{
	int color = 0xFFFFFF*iters/(double)max;
	return color;
}


// Show help message
void show_help()
{
	printf("Use: mandel [options]\n");
	printf("Where options are:\n");
	printf("-m <max>    The maximum number of iterations per point. (default=1000)\n");
	printf("-x <coord>  X coordinate of image center point. (default=0)\n");
	printf("-y <coord>  Y coordinate of image center point. (default=0)\n");
	printf("-s <scale>  Scale of the image in Mandlebrot coordinates (X-axis). (default=4)\n");
	printf("-W <pixels> Width of the image in pixels. (default=1000)\n");
	printf("-H <pixels> Height of the image in pixels. (default=1000)\n");
	printf("-o <file>   Set output file. (default=mandel.bmp)\n");
	printf("-h          Show this help text.\n");

    // NEW: appended line
	printf("-p <proc>   Generate 50-frame Mandelbrot movie using <proc> child processes.\n");
	printf("\nSome examples are:\n");
	printf("mandel -x -0.5 -y -0.5 -s 0.2\n");
	printf("mandel -x -.38 -y -.665 -s .05 -m 100\n");
	printf("mandel -x 0.286932 -y 0.014287 -s .0005 -m 1000\n\n");
}

static int run_movie_mode(char *progpath, int num_children,
                          double xcenter, double ycenter,
                          double xscale, int image_width,
                          int image_height, int max_iter,
                          const char *outfile_prefix)
{
    const int FRAMES = 50;
    const double zoom_factor = 0.97; // shrink scale per frame
    int active = 0;
    int frame;

    printf("Movie mode: %d frames, %d processes\n", FRAMES, num_children);

    for (frame=0; frame<FRAMES; frame++){
        while(active >= num_children){
            wait(NULL);
            active--;
        }

        pid_t pid = fork();

        if (pid == 0){
            /* CHILD PROCESS */

            double frame_scale = xscale * pow(zoom_factor, frame);

            char xs[64], ys[64], ss[64], ws[32], hs[32], ms[32], out[128];

            snprintf(xs, sizeof(xs), "%f", xcenter);
            snprintf(ys, sizeof(ys), "%f", ycenter);
            snprintf(ss, sizeof(ss), "%f", frame_scale);
            snprintf(ws, sizeof(ws), "%d", image_width);
            snprintf(hs, sizeof(hs), "%d", image_height);
            snprintf(ms, sizeof(ms), "%d", max_iter);

            snprintf(out, sizeof(out), "%s%02d.jpg", outfile_prefix, frame);

            char *newargv[] = {
                progpath,
                "-x", xs,
                "-y", ys,
                "-s", ss,
                "-W", ws,
                "-H", hs,
                "-m", ms,
                "-o", out,
                NULL
            };

            execv(progpath, newargv);
            perror("execv failed");
            exit(1);
        }else{
            /* PARENT PROCESS */
            active++;
        }
    }

    /* Wait for all children to finish */
    while(active > 0){
        wait(NULL);
        active--;
    }

    printf("Movie generation complete.\n");
    return 0;
}
