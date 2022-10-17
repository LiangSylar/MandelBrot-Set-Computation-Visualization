#include "mpi.h"
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xos.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <time.h>

#define MASTER 0
#define X_RESN  200       /* x resolution */
#define Y_RESN  200       /* y resolution */

void printInfo(double runningTime);

typedef struct complextype
        {
        float real, imag;
        } Compl;

int main(int argc, char* argv[]) {  

	MPI_Init(&argc, & argv);
	int taskid, 
		numtasks, 
		len;
	char hostname[MPI_MAX_PROCESSOR_NAME];
	MPI_Get_processor_name(hostname, &len);
	MPI_Comm_size(MPI_COMM_WORLD, &numtasks);
	MPI_Comm_rank(MPI_COMM_WORLD, &taskid);

    timeval t_start, t_end;

    int max_index = X_RESN*Y_RESN;
    int average_lines = int( floor(Y_RESN/(numtasks-1)) );

    //computation process 
    if (taskid != MASTER) {

    	int recv_start_signal;
    	MPI_Status status;
	    MPI_Recv(&recv_start_signal, 1, MPI_INT, MASTER, 1, MPI_COMM_WORLD, &status); 

	    int start_line = (taskid-1)*average_lines; //starts from 0

	    int core_lines = average_lines;


	    if (taskid == numtasks-1) {
	 
	    	core_lines = Y_RESN - (numtasks-2)*average_lines;
	    	start_line = (numtasks-2)*average_lines;
	 
	    }

	 	int 	i, j, k;
	    Compl   z, c;
		float   lengthsq, temp;

		int current_line = start_line;
		int countOfLines = 0;
		int sendArray[X_RESN+1];

		while(countOfLines < core_lines) {

			int currentIdx = current_line*X_RESN;
			sendArray[0] = current_line;

			for(int idx = 0; idx<X_RESN; idx++) {

				j = currentIdx % X_RESN;
				i = currentIdx / X_RESN;

				z.real = z.imag = 0.0;
			    c.real = ((float) j - X_RESN*.5)/(X_RESN*.25);               /* scale factors for 800 x 800 window */
			    c.imag = ((float) i - Y_RESN*.5)/(Y_RESN*.25);
			    k = 0; //k is the # iterations

			    do  {                                             /* iterate for pixel color */
			        
			        temp = z.real*z.real - z.imag*z.imag + c.real;
			        z.imag = 2.0*z.real*z.imag + c.imag;
			        z.real = temp;
			        lengthsq = z.real*z.real+z.imag*z.imag;
			        k++;

			    } while (lengthsq < 4 && k < 100);

				sendArray[idx+1] = (k==100) ? 1 : 0;

			    currentIdx++;
			}

			MPI_Send(sendArray, X_RESN+1, MPI_INT, MASTER, 1, MPI_COMM_WORLD);

			countOfLines++;
			current_line++;
		}

    }   
    else if(taskid == MASTER) {

		Window          win;                            /* initialization for a window */
	    unsigned
	    int             width, height,                  /* window size */
	                    x, y,                           /* window position */
	                    border_width,                   /*border width in pixels */
	                    display_width, display_height,  /* size of screen */
	                    screen;                         /* which screen */
	    char            *window_name = "Mandelbrot Set", *display_name = NULL;
	    GC              gc;
	    unsigned
	    long            valuemask = 0;
	    XGCValues       values;
	    Display         *display;
	    XSizeHints      size_hints;
	    Pixmap          bitmap;
	    XPoint          points[800];
	    FILE            *fp, *fopen ();
	    char            str[100];
	    XSetWindowAttributes attr[1];

	    if (  (display = XOpenDisplay (display_name)) == NULL ) {
        fprintf (stderr, "drawon: cannot connect to X server %s\n",
                    XDisplayName (display_name) );
    	exit (-1);
    	}
	    
	    screen = DefaultScreen (display);
	    display_width = DisplayWidth (display, screen);
	    display_height = DisplayHeight (display, screen);
	    
	    /* set window size */
	    width = X_RESN;
	    height = Y_RESN;

	    /* set window position */
	    x = 0;
	    y = 0;

	    /* create opaque window */
	    border_width = 4;
	    win = XCreateSimpleWindow (display, RootWindow (display, screen),
	                            x, y, width, height, border_width, 
	                            BlackPixel (display, screen), WhitePixel (display, screen));

	    size_hints.flags = USPosition|USSize;
	    size_hints.x = x;
	    size_hints.y = y;
	    size_hints.width = width;
	    size_hints.height = height;
	    size_hints.min_width = 300;
	    size_hints.min_height = 300;
	    
	    XSetNormalHints (display, win, &size_hints);
	    XStoreName(display, win, window_name);

	    /* create graphics context */
	    gc = XCreateGC (display, win, valuemask, &values);

	    XSetBackground (display, gc, WhitePixel (display, screen));
	    XSetForeground (display, gc, BlackPixel (display, screen));
	    XSetLineAttributes (display, gc, 1, LineSolid, CapRound, JoinRound);

	    attr[0].backing_store = Always;
	    attr[0].backing_planes = 1;
	    attr[0].backing_pixel = BlackPixel(display, screen);

	    XChangeWindowAttributes(display, win, CWBackingStore | CWBackingPlanes | CWBackingPixel, attr);

	    XMapWindow (display, win);
	    XSync(display, 0);

		static int result_array[X_RESN*Y_RESN];

		/*let slaves start computation*/
		int start_signal = 1;
		for(int i=1; i < numtasks; i++) {
			MPI_Send(&start_signal, 1, MPI_INT, i, 1, MPI_COMM_WORLD);
		}
		gettimeofday(&t_start, NULL); //starts recording time

		MPI_Status status;

		int recvArray[X_RESN+1];
		int recvLine;

		for(int i = 0; i < Y_RESN; i++) {
		
			MPI_Recv(recvArray, X_RESN+1, MPI_INT, MPI_ANY_SOURCE, 1, MPI_COMM_WORLD, &status);

			recvLine = recvArray[0];
			//printf("Master receives line %d\n", recvLine);
			int current_index = recvLine*X_RESN;
			for(int j = 1; j <= X_RESN; j++) {
				//printf("Master receives index %d from line %d\n", current_index, recvLine);
				result_array[current_index] = recvArray[j];
				current_index++;
			}
		}

		gettimeofday(&t_end, NULL);
    	double runningTime = (t_end.tv_sec - t_start.tv_sec) + 
                        	 (t_end.tv_usec - t_start.tv_usec) / 1000000.0;
    	printInfo(runningTime);
		//printf("running time is %f \n", float(end-start)/CLOCKS_PER_SEC);

		int i_, j_;
		for(int i = 0; i < max_index; i++) {
			if (result_array[i] == 1) {
				j_ = i % X_RESN;
				i_ = i / X_RESN;
				XDrawPoint (display, win, gc, j_, i_);
				usleep(1);
			}
		}

		XFlush (display);
    	sleep (30);
	
	}


	MPI_Finalize();
	return 0;
}

void printInfo(double runningTime) {
    printf("\nName: %s\n", "Liang Jialu");
    printf("Student ID: %d\n", 118010164);
    printf("Assignment 2, Mandelbrot Set, Static MPI implementation.\n");
    printf("runTime is %f\n", runningTime);
}