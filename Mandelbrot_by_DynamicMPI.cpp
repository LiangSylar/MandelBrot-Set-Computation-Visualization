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
#define Y_RESN  200      /* y resolution */

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

    int CHUNK = 200;

    int numOfEntries = 2+CHUNK; //recored number of elements in the recv_Array from slaves

	if (taskid == MASTER) {

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
	    timeval t_start, t_end;

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

	    //printf("show screen\n");

		int max_index = X_RESN*Y_RESN;

		static int result_array[X_RESN*Y_RESN]; //
		int index, //used indicate the number send to slaves;
			// index starts from 1
			next_core,
			recv_index,
			recv_id,
			i_,  //used for drawing point
			j_;  //used for drawing point


		int avail_core[numtasks];
		int head = 0;
		int tail = 0;

		index = 1;

		gettimeofday(&t_start, NULL); //computations start

		for(int i = 1; i < numtasks; i++) { //push all slaves into avail_core first
			avail_core[i-1] = i;
			tail = (tail+1)%numtasks;
		}

		// for(int i = 1; i<numtasks; i++){ //initialize cores
		// 	MPI_Send(&index, 1, MPI_INT, i, 1, MPI_COMM_WORLD);
		// 	//printf("Master sends index %d to slave %d\n", index, i);
		// 	index++;
		// }		

		while (head != tail && index <= max_index) { //initialize all slaves first
			next_core = avail_core[head];
			head = (head+1)%numtasks;
			// printf("next_core is %d\n", next_core);
			MPI_Send(&index, 1, MPI_INT, next_core, 1, MPI_COMM_WORLD);
			index = index + CHUNK;
		}


		while (index <= max_index+CHUNK) { //dynamic process starts

			int recv_array[numOfEntries];
			MPI_Status status; 
			MPI_Recv(recv_array, numOfEntries, MPI_INT, MPI_ANY_SOURCE, 1, MPI_COMM_WORLD, &status); 
			recv_id = recv_array[0];
			recv_index = recv_array[1];
			// printf("Master successfully receives index %d from slave\n",recv_index );
			// printf("Master knows that slave %d is available now\n", recv_id);

			avail_core[tail] = recv_id;
			tail = (tail+1)%numtasks;
			//printf("Slave %d rests: (head, tail) is (%d, %d)\n",recv_id, head, tail);

			int currentIdx = recv_index;
			for(int i = 0; i < CHUNK; i++) {
				if (recv_array[2+i]==1) {
					//printf("Master receives a Mandelbrot number %d\n", currentIdx);
					//XDrawPoint (display, win, gc, j_, i_);
					result_array[currentIdx-1] = 1;
					//printf("");
					//draw the point
				} else{
					result_array[currentIdx-1] = 0;
				}
				currentIdx++;
			}


			if(tail != head) {
				next_core = avail_core[head];
				//printf("next_core is %d\n", next_core);
				head = (head+1)%numtasks;
				//printf("Slave %d activates: (head, tail) is (%d, %d)\n",next_core, head, tail);
				MPI_Send(&index, 1, MPI_INT, next_core, 1, MPI_COMM_WORLD);
				//printf("Master sends index %d to slave %d\n", index, next_core);
				index = index + CHUNK;
			}		
		}

		int stop_signal = -1;
		for(int p=1; p < numtasks; p++) {
			MPI_Send(&stop_signal, 1, MPI_INT, p, 1, MPI_COMM_WORLD);
			//printf("Master sends ending signals to slave %d\n", p);
		}
		
		gettimeofday(&t_end, NULL);
    	double runningTime = (t_end.tv_sec - t_start.tv_sec) + 
                         (t_end.tv_usec - t_start.tv_usec) / 1000000.0;
    	printInfo(runningTime);
		//printf("running time is %f \n", float(end-start)/CLOCKS_PER_SEC);
		
		for(int i = 0; i < max_index; i++) {
			if (result_array[i] == 1) {
				//printf("Master receives a Mandelbrot number %d\n", i);
				j_ = (i) % X_RESN;
				i_ = (i) / X_RESN;
				XDrawPoint (display, win, gc, j_, i_);
				usleep(1)
			}
		}

		XFlush (display);
		//printf("end of computation\n");
    	sleep (30);
	
	}

	else if (taskid != MASTER) {

		int Srecv_index;
		int Ssend_array[numOfEntries];

		MPI_Status status;
		while(1) {

			MPI_Recv(&Srecv_index, 1, MPI_INT, MASTER, 1, MPI_COMM_WORLD, &status);

			if(Srecv_index == -1) {

				break;
			};

			for (int count = 0; count < CHUNK; count ++) {
				/*computation*/
				int i, j, k;
				Compl   z, c;
				float   lengthsq, temp;

				j = (Srecv_index+count -1) % X_RESN;
				i = (Srecv_index+count -1) / X_RESN;

				z.real = z.imag = 0.0;
		        c.real = ((float) j - X_RESN*.5)/(X_RESN*.25);               /* scale factors for 800 x 800 window */
		        c.imag = ((float) i - Y_RESN*.5)/(Y_RESN*.25);

		        k = 0; //k is the # iterations

		        do  {

		            temp = z.real*z.real - z.imag*z.imag + c.real;
		            
		            z.imag = 2.0*z.real*z.imag + c.imag;
		            z.real = temp;
		            lengthsq = z.real*z.real+z.imag*z.imag;

		            k++;
		        } while (lengthsq < 4 && k < 100);
				
				Ssend_array[2+count] = (k==100) ? 1 : 0;

			}

			Ssend_array[0] = taskid;
			Ssend_array[1] = Srecv_index;

	        MPI_Send(Ssend_array, numOfEntries, MPI_INT, MASTER, 1, MPI_COMM_WORLD);

		}
 	}

	MPI_Finalize();
	return 0;
}

void printInfo(double runningTime) {
    printf("\nName: %s\n", "Liang Jialu");
    printf("Student ID: %d\n", 118010164);
    printf("Assignment 2, Mandelbrot Set, Dynamic MPI implementation.\n");
    printf("runTime is %f\n", runningTime);
}