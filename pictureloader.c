
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <opencv/cv.h>
#include <opencv/highgui.h>
#include <math.h>
#include <omp.h>

void filling(unsigned* currentfield, int w, int h) {
  for (int i = 0; i < h*w; i++) {
    currentfield[i] = (rand() < RAND_MAX / 10) ? 1 : 0; ///< init domain randomly
  }
}

//weil ich davon ausging die Funktion existiert in math.h und ich die funktionen nicht zu pow(x,2) oder x*x umändern wollte
int sqr(uchar value)
{
	return value * value;
}


void fillFromFile(unsigned* currentfield, int w, int h, char* filePath)
{


	IplImage* img = 0; 
	/*
	* 2.Inputparameter cvLoadImage
	* > 1 3-Farbenbild (BGR)  	CV_LOAD_IMAGE_COLOR
	* = 1 Graustufenbild		CV_LOAD_IMAGE_GRAYSCALE 
	* < 1 wie dargestellt laden (mit alphakanal)	CV_LOAD_IMAGE_ANYDEPTH 
	*/
	img=cvLoadImage(filePath, CV_LOAD_IMAGE_COLOR);
	if(!img)
	{
		filling(currentfield, w, h);
		return;
	}

	int height    = img->height;
	int width     = img->width;
	int step      = img->widthStep; //Anzahl Farbkanäle * Bildbreite
	//int channels  = img->nChannels;
	// für uns 3 channel in GBR-Code
	uchar *imgData      = (uchar *)img->imageData;
	#pragma omp parallel for collapse(2)
		for (int row = 0; row<h; row++)
  		for(int col = 0; col<w; col++)
  		{
  			if((row >= height) || (col >= width))
  				currentfield[row*h+col] = 0;
  			else
  			{
  				uchar b = imgData[step * row + col * 3];
         	uchar g = imgData[step * row + col * 3 + 1];
         	uchar r = imgData[step * row + col * 3 + 2];

  	    	//dunkle Pixel finden und diese als 1 interpretieren
        	int intensity = sqrt((sqr(b) + sqr(g) + sqr(r)) / 3);
  				currentfield[row*h+col] = intensity < 128? 1 : 0;
  			}
  		}
}