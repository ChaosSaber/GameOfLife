#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <endian.h>
#include <omp.h>
#include <math.h>
#include "pictureloader.h"

#define calcIndex(width, x,y)  ((y)*(width) + (x))

struct Marginals
{
  int ystart, yend, xstart, xend;
};

int max(int a, int b)
{
  if(a > b) return a;
  else return b; 
}


void show(unsigned* currentfield, int w, int h) 
{
  printf("\033[H");
  for (int y = 0; y < h; y++) 
  {
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

void writeVTK(unsigned* currentfield, int w, struct Marginals marginals, int t, char* prefix) 
{
    int blockWidth = marginals.xend - marginals.xstart, blockHeight = marginals.yend - marginals.ystart; 
    char name[1024] = "\0";
    int threadnum = omp_get_thread_num();
    sprintf(name, "%s_%d_%d.vtk", prefix, threadnum, t);
    FILE* outfile = fopen(name, "w");

    /*Write vtk header */                                                           
    fprintf(outfile,"# vtk DataFile Version 3.0\n");       
    fprintf(outfile,"FRAME %d\n", t);     
    fprintf(outfile,"BINARY\n");     
    fprintf(outfile,"DATASET STRUCTURED_POINTS\n");     
    fprintf(outfile,"DIMENSIONS %d %d %d \n",  blockWidth, blockHeight, 1);        
    fprintf(outfile,"SPACING 1.0 1.0 1.0\n");//or ASPECT_RATIO                            
    fprintf(outfile,"ORIGIN %d %d 0\n", marginals.xstart-marginals.xstart/blockWidth, marginals.ystart - marginals.ystart/blockHeight);                                              
    fprintf(outfile,"POINT_DATA %d\n", blockHeight*blockWidth);
    fprintf(outfile,"SCALARS data float 1\n");
    fprintf(outfile,"LOOKUP_TABLE default\n");
   
    for (int y = marginals.ystart; y < marginals.yend; y++) {
      for (int x = marginals.xstart; x < marginals.xend; x++) {
        float value = currentfield[calcIndex(w, x,y)]; // != 0.0 ? 1.0:0.0;
        value = convert2BigEndian(value);
        fwrite(&value, 1, sizeof(float), outfile);
      }
    }
    fclose(outfile);
}

int coutLifingsPeriodic(unsigned* currentfield, int x , int y, int w, int h) 
{
   int n = 0;
   for (int y1 = y - 1; y1 <= y + 1; y1++) 
     for (int x1 = x - 1; x1 <= x + 1; x1++) 
       if (currentfield[calcIndex(w, (x1 + w) % w, (y1 + h) % h)]) 
         n++;
   return n;
}
 
struct Marginals* getMarginals(int w, int h, int horizontalBlocks, int verticalBlocks)
{
  struct Marginals *result = (struct Marginals*)malloc(horizontalBlocks*verticalBlocks * sizeof(struct Marginals)); 
  double blockHeight = 1.0 * h / verticalBlocks, blockWidth = 1.0 * w / horizontalBlocks;

  for (int vertical = 0; vertical < verticalBlocks; vertical++)
  {
    for (int horizontal = 0; horizontal < horizontalBlocks; horizontal++)
    {
      struct Marginals marginals =
      {
        round(vertical * blockHeight),
        round((vertical + 1) * blockHeight),
        round(horizontal * blockWidth), 
        round((horizontal + 1) * blockWidth)     
      };

      result[horizontal + vertical * horizontalBlocks] = marginals;
      //{round(vertical * blockHeight), round((vertical + 1) * blockHeight), round(horizontal * blockWidth), round((horizontal + 1) * horizontalBlocks)};
    }
  }

  return result;
}

int evolve(unsigned* currentfield, unsigned* newfield, int w, int h, struct Marginals marginals) 
{
  int changes = 0;

  for (int y = marginals.ystart; y < marginals.yend; y++) 
  {
    for (int x = marginals.xstart; x < marginals.xend; x++) 
    {
      int n = coutLifingsPeriodic(currentfield, x , y, w, h);

      if (currentfield[calcIndex(w, x,y)]) 
        n--;
      newfield[calcIndex(w, x,y)] = (n == 3 || (n == 2 && currentfield[calcIndex(w, x,y)]));   
      if(newfield[calcIndex(w, x,y)] != currentfield[calcIndex(w, x,y)])
        changes++;
    }
  }

  return changes;
}
 
void game(int w, int h, int timesteps, int horizontalBlocks, int verticalBlocks, char* inputFile) 
{
  unsigned *currentfield = calloc(w*h, sizeof(unsigned));
  unsigned *newfield     = calloc(w*h, sizeof(unsigned));
  
  fillFromFile(currentfield, w, h, inputFile);

  struct Marginals *marginals = getMarginals(w, h, horizontalBlocks, verticalBlocks);

  for (int t = 0; t < timesteps; t++) 
  {
    //show(currentfield, w, h);
    int changes=0;
    #pragma omp parallel
    {
      writeVTK(currentfield, w, marginals[omp_get_thread_num()], t, "out/output");
      #pragma omp atomic
      changes += evolve(currentfield, newfield, w, h, marginals[omp_get_thread_num()]);
    }
      if (changes == 0) {
    	sleep(3);
    	break;
    }

    //SWAP
    unsigned *temp = currentfield;
    currentfield = newfield;
    newfield = temp;
  }
  
  free(currentfield);
  currentfield = NULL;
  free(newfield);
  newfield = NULL;
  free(marginals);
  marginals = NULL;
}
 
int main(int argc, char **argv) 
{
  int w = 0, h = 0, timesteps = 10, horizontalBlocks = 2, verticalBlocks = 2;
  char* inputFile = NULL;

  if (argc > 1) w = atoi(argv[1]); ///< read width
  if (argc > 2) h = atoi(argv[2]); ///< read height
  if (argc > 3) timesteps = atoi(argv[3]);
  if (argc > 4) horizontalBlocks = atoi(argv[4]);
  if (argc > 5) verticalBlocks = atoi(argv[5]);
  if (argc > 6) inputFile = argv[6];
  if (w <= 0) w = 30; ///< default width
  if (h <= 0) h = 30; ///< default height

  omp_set_num_threads(horizontalBlocks * verticalBlocks);
  game(w, h, timesteps, horizontalBlocks, verticalBlocks, inputFile);
}