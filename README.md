# MandelBrot-Set-Computation-Visualization
## What is Mandelbrot Set
The Mandelbrot set is a set of complex number: 
the value of such a complex number C makes the quadratic recurrence equation
                           
                           Z_(k+1)=Z_k^2+C                              (1)
                           
fail to diverge after times of iteration and remain bounded within certain range. 
The plot of the Mandelbrot set generates magical images. Below is a sample plot of Mandelbrot Set.

## Contents of this Repository 
By increasing the number of iteration and the resolution of the image, 
one can produce Mandelbrot images with an incredible amount of details. 
The cost of generating fine Mandelbrot images is also high: 
million times of computation can be involved in the program. 
The improvement of efficiency is of great importance in the computation of the Mandelbrot set. 
Several programs, implemented in MPI, Pthread, or sequential, are used 
to compute and generate Mandelbrot set images.

![image](https://user-images.githubusercontent.com/64362092/196091004-3d31f158-1198-4903-8df1-09500255b52a.png)

