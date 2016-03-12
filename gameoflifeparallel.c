#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <endian.h>
#include <omp.h>
#include "pictureloader.h"

#define calcIndex(width, x,y)  ((y)*(width) + (x))

int max(int a, int b)
{
  if(a > b) return a;
  else return b; 
}


void show(unsigned* currentfield, int w, int h) {
  printf("\033[H");
  for (int y = 0; y < h; y++) {
    for (int x = 0; x < w; x++) printf(currentfield[calcIndex(w, x,y)] ? "\033[07m  \033[m" : "  ");
    printf("\033[E");
  }
  fflush(stdout);
}


float convert2BigEndian( const float inFloat )
{
   float retVal;
   char *floatToConvert = ( char* ) & inFloat;
   char *returnFloat    = ( char* ) & retVal;

   // swap the bytes into a temporary buffer
   returnFloat[0] = floatToConvert[3];
   returnFloat[1] = floatToConvert[2];
   returnFloat[2] = floatToConvert[1];
   returnFloat[3] = floatToConvert[0];

   return retVal;
}

void writeVTK(unsigned* currentfield, int w, int h, int t, char* prefix) {

    int blockHeight = h / omp_get_num_threads();
    char name[1024] = "\0";
    int threadnum = omp_get_thread_num();
    sprintf(name, "%s_%d_%d.vtk", prefix,  threadnum, t);
    FILE* outfile = fopen(name, "w");

    /*Write vtk header */                                                           
    fprintf(outfile,"# vtk DataFile Version 3.0\n");       
    fprintf(outfile,"FRAME %d\n", t);     
    fprintf(outfile,"BINARY\n");     
    fprintf(outfile,"DATASET STRUCTURED_POINTS\n");     
    fprintf(outfile,"DIMENSIONS %d %d %d \n",  w, blockHeight, 1);        
    fprintf(outfile,"SPACING 1.0 1.0 1.0\n");//or ASPECT_RATIO                            
    fprintf(outfile,"ORIGIN 0 %d 0\n",max(threadnum  * (blockHeight - 1), 0));                                              
    fprintf(outfile,"POINT_DATA %d\n", blockHeight*w);                                    
    fprintf(outfile,"SCALARS data float 1\n");                              
    fprintf(outfile,"LOOKUP_TABLE default\n");         
   
    for (int y = threadnum*blockHeight; y < (threadnum + 1) * blockHeight; y++) {
      for (int x = 0; x < w; x++) {
        float value = currentfield[calcIndex(w, x,y)]; // != 0.0 ? 1.0:0.0;
        value = convert2BigEndian(value);
        fwrite(&value, 1, sizeof(float), outfile);
      }
    }
    fclose(outfile);
}
int coutLifingsPeriodic(unsigned* currentfield, int x , int y, int w, int h) {
   int n = 0;
   for (int y1 = y - 1; y1 <= y + 1; y1++) 
     for (int x1 = x - 1; x1 <= x + 1; x1++) 
       if (currentfield[calcIndex(w, (x1 + w) % w, (y1 + h) % h)]) 
         n++;
   return n;
}
 
int evolve(unsigned* currentfield, unsigned* newfield, int w, int h) {
int changes = 0;
int blockHeight = h / omp_get_num_threads();
  int threadnum = omp_get_thread_num();
  for (int y = threadnum*blockHeight; y < (threadnum + 1) * blockHeight; y++) {
    for (int x = 0; x < w; x++) {
      int n = coutLifingsPeriodic(currentfield, x , y, w, h);
      if (currentfield[calcIndex(w, x,y)]) 
        n--;
      newfield[calcIndex(w, x,y)] = (n == 3 || (n == 2 && currentfield[calcIndex(w, x,y)]));   
      if(newfield[calcIndex(w, x,y)] != currentfield[calcIndex(w, x,y)])
        changes++;
    }
  }
//TODO if changes == 0, the time loop will not run! 
  return changes;
}
 

 
void game(int w, int h, int timesteps) {
  unsigned *currentfield = calloc(w*h, sizeof(unsigned));
  unsigned *newfield     = calloc(w*h, sizeof(unsigned));
  
  //filling(currentfield, w, h);
  fillFromFile(currentfield, w, h, "test.png");
  for (int t = 0; t < timesteps; t++) {
    show(currentfield, w, h);
    int changes=0;
    #pragma omp parallel
    {
      writeVTK(currentfield, w, h, t, "out/output");
      #pragma omp atomic
      changes += evolve(currentfield, newfield, w, h);
    }
      if (changes == 0) {
    	sleep(3);
    	break;
    }
    
    //usleep(200000);

    //SWAP
    unsigned *temp = currentfield;
    currentfield = newfield;
    newfield = temp;
  }
  
  free(currentfield);
  free(newfield);
}
 
int main(int c, char **v) {
  omp_set_num_threads(3);
  int w = 0, h = 0, timesteps = 10;
  if (c > 1) w = atoi(v[1]); ///< read width
  if (c > 2) h = atoi(v[2]); ///< read height
  if (c > 3) timesteps = atoi(v[3]);
  if (w <= 0) w = 30; ///< default width
  if (h <= 0) h = 30; ///< default height
  game(w, h, timesteps);

}
