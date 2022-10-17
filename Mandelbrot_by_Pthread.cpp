#include <stdio.h>
#include <pthread.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xos.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <time.h>

#define         X_RESN  200       /* x resolution */
#define         Y_RESN  200       /* y resolution */
#define         no_threads 3

void printInfo(double runningTime);

typedef struct complextype
        {
        float real, imag;
        } Compl;

int global_index = 1;
int max_index = X_RESN*Y_RESN;
static int mandelbrot_set[X_RESN*Y_RESN];
int set_index = 0;

int chunk = 200; //chunk must smaller than max_index

pthread_mutex_t mutex1;

void *slave(void *ignored){

	int local_index, i, j, k;
	Compl   z, c;
	float   lengthsq, temp;

	do{
		pthread_mutex_lock(&mutex1);
		
        if (global_index > max_index+1) {
			pthread_mutex_unlock(&mutex1);
			break;
		}

        local_index = global_index;

        //printf("global_index is %d\n", global_index);

		global_index = global_index + chunk;
		//if (local_index < max_index) printf("hahaha %d\n", max_index);
		pthread_mutex_unlock(&mutex1);
		//if (i == 500 && j == 9) printf("%d\n", global_index);

		//printf("%d, %d\n", i, j);

        int times = 0;
        while(local_index < max_index+1 && times < chunk)    {

            //printf("local_index is %d\n", local_index);

            j = (local_index-1) % X_RESN;
            i = (local_index-1) / X_RESN;
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

    		if (k == 100) {
    			pthread_mutex_lock(&mutex1);
                mandelbrot_set[set_index] = local_index;
                set_index++;
                pthread_mutex_unlock(&mutex1);
            }

            local_index++;
            times++;
        }

	} while (local_index < (max_index+1));
	
    pthread_exit(NULL);
}

int main() {

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
   
    /* connect to Xserver */
    if (  (display = XOpenDisplay (display_name)) == NULL ) {
        fprintf (stderr, "drawon: cannot connect to X server %s\n",
                    XDisplayName (display_name) );
    exit (-1);
    }
        
    /* get screen size */

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

    /*create slaves */
	int i;
	pthread_t thread[no_threads];
	pthread_mutex_init(&mutex1, NULL);

    gettimeofday(&t_start, NULL);

	for (i = 0; i < no_threads; i++) {
		if (pthread_create(&thread[i], NULL, slave, NULL) != 0)
			perror("Pthread_create fails");
	}
	for (i = 0; i < no_threads; i++) {
		if (pthread_join(thread[i], NULL) != 0)
			perror("Pthread_join fails");
	}
	
    gettimeofday(&t_end, NULL);
    double runningTime = (t_end.tv_sec - t_start.tv_sec) + 
                         (t_end.tv_usec - t_start.tv_usec) / 1000000.0;

    printInfo(runningTime);
	
	int h = 0;
	int j_, i_;
	while(mandelbrot_set[h] != 0) {
		j_ = (mandelbrot_set[h]-1) % X_RESN;
		i_ = (mandelbrot_set[h]-1) / X_RESN;
		//printf("%d, %d\n",i_, j_ );
		XDrawPoint (display, win, gc, j_, i_);
        usleep(1);
        h++;
	}
	XFlush (display);
    sleep (30);
}

void printInfo(double runningTime) {
    printf("\nName: %s\n", "Liang Jialu");
    printf("Student ID: %d\n", 118010164);
    printf("Assignment 2, Mandelbrot Set, Pthread implementation.\n");
    printf("runTime is %f\n", runningTime);
}