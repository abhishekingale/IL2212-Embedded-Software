#include "altera_avalon_performance_counter.h"
#include "system.h"
#include <stdio.h>
#include "images.h"
#include "ascii_gray.h"

#define dSPAN 15
#define cropSIZE ((2*dSPAN)+1) //the size of the image after cropping

//#define PERFORMANCE
#define DEBUG

unsigned char grayed[64][64]; //output matrix of the grayscale function
double cropResult[cropSIZE][cropSIZE]; //output matrix of the crop function
double xcorr2Result[cropSIZE-4][cropSIZE-4]; //output matrix of the xcorr2 function


typedef struct { //structure to map to coordinates
  int x, y;
} coordinate;

coordinate init; //coordinate representing the initial token

coordinate detect; //coordinate representing the output of the objectPos function

void grayscale(unsigned char matrix[64][192]){ //equivalent to grayscale
	  int j,i;
		for(i = 0; i < 64; i++) { //cycle through rows
	    for(j = 0; j < 192; j+=3) { //cycle through columns 3 at a time, combining every RGB to 1
	    	grayed[i][j/3] =  (unsigned char)( matrix[i][j] * 0.3125 + matrix[i][j+1] * 0.5625 + matrix[i][j+2] * 0.125); //apply multiplication to each element
	    }
  }
		#ifdef DEBUG
			printAscii(grayed, 64, 64);
			printf("\n\n");
		#endif
}

void getImage(int imageNumber){ //a function to get the image from a header file, convert it to a matrix and grayscale it
   //PERF_BEGIN(PERFORMANCE_COUNTER_0_BASE, 2);
	int columnSize = 192; //size of the column for the rgb matrix
	int rowSize = 64;
	unsigned char img[rowSize][columnSize]; //rgb matrix representing the image
	int i;
	int y;
	if(imageNumber == 0){ //imageNumber is the number of the image being scanned
		for(i=0;i<rowSize;i++){
				for(y=0;y<columnSize;y++){
					img[i][y] = test_ppm_1[i*columnSize+y+3]; //converts the image array to an image matrix
				}
			}
	}
	else if(imageNumber == 1){
		for(i=0;i<rowSize;i++){
				for(y=0;y<columnSize;y++){
					img[i][y] = test_ppm_2[i*columnSize+y+3];
				}
			}
	}
	else if(imageNumber == 2){
		for(i=0;i<rowSize;i++){
				for(y=0;y<columnSize;y++){
					img[i][y] = test_ppm_3[i*columnSize+y+3];
				}
			}
	}
	else if(imageNumber == 3){
		for(i=0;i<rowSize;i++){
				for(y=0;y<columnSize;y++){
					img[i][y] = test_ppm_4[i*columnSize+y+3];
				}
			}
	}
	grayscale(img);
  //PERF_END(PERFORMANCE_COUNTER_0_BASE, 2);
}

int getRowsMatrix(){ //returns the number of rows in the gray matrix
  return sizeof(grayed)/sizeof(grayed[0]);
}

int getColumnsMatrix(){ //returns the number of column in the gray matrix
  return sizeof(grayed[0])/sizeof(grayed[0][0]);
}

void crop(coordinate c){ //equivalent to crop
  int i, j;
   //PERF_BEGIN(PERFORMANCE_COUNTER_0_BASE, 3);
  for(i = c.y; i < c.y + cropSIZE; i++) { //cycle through rows
    for(j = c.x; j < c.x + cropSIZE; j++) { //cycle through columns
    cropResult[i-c.y][j-c.x] = (double) grayed[i][j]; //populate new cropped matrix of size cropSIZE by cropSIZE
    }
  }
   //PERF_END(PERFORMANCE_COUNTER_0_BASE, 3);
}

coordinate delay(){ //if its the first run, returns the initial token, otherwise returns the last coordinate
   //PERF_BEGIN(PERFORMANCE_COUNTER_0_BASE, 4);
	init.x = dSPAN;
	init.y = dSPAN;
   //PERF_END(PERFORMANCE_COUNTER_0_BASE, 4);
  if(detect.x==-1){
    return init; //init token
  }
  else return detect; //last coordinate
}

coordinate calcCoord(coordinate previous){ //equivalent to calcCoord
   //PERF_BEGIN(PERFORMANCE_COUNTER_0_BASE, 4);
    coordinate crop; //starting coordinate for cropping
    int numberRows = getRowsMatrix();
    int numberColumns = getColumnsMatrix();

    if(previous.x <= dSPAN) crop.x=0;
    else if(previous.x > numberColumns-dSPAN) crop.x= numberColumns-dSPAN*2-1;
    else crop.x = previous.x-dSPAN-1;

    if(previous.y <= dSPAN) crop.y=0;
    else if(previous.y > numberRows-dSPAN) crop.y= numberRows-dSPAN*2-1;
    else crop.y = previous.y-dSPAN-1;
   //PERF_END(PERFORMANCE_COUNTER_0_BASE, 4);
    return crop;
}

void xcorr2(){ //equivalent to xcorr2
  int i, j;
//PERF_BEGIN(PERFORMANCE_COUNTER_0_BASE, 5);
  for(i = 0; i < cropSIZE-4; i++) {
    for(j = 0; j < cropSIZE-4; j++) {
          xcorr2Result[i][j] = cropResult[i][j+2] + cropResult[i+1][j+1] +cropResult[i+1][j+3] + cropResult[i+2][j] + cropResult[i+2][j+4] +
		  cropResult[i+3][j+1] +cropResult[i+3][j+3] +cropResult[i+4][j+2]; //calculate the cross correllation
    }
  }
//PERF_END(PERFORMANCE_COUNTER_0_BASE, 5);
}
coordinate posMax(){ //equivalent to getOffset
   //PERF_BEGIN(PERFORMANCE_COUNTER_0_BASE, 2);
  double currentNumber = 0; //number used to store largest number in matrix
  coordinate currentCoordinate; //the current largest coordinate
  currentCoordinate.x = 0;
  currentCoordinate.y = 0;
  int i,j;
  for(i = 0; i < cropSIZE-4; i++) {
    for(j = 0; j < cropSIZE-4; j++) {
      if(currentNumber < xcorr2Result[i][j]) { //if we find a number larger than the current number
        currentNumber = xcorr2Result[i][j]; //the current number becomes the number we found
        currentCoordinate.x = j; //the current coordinate is the coordinate of that number
        currentCoordinate.y = i;
      }
    }
  }
   //PERF_END(PERFORMANCE_COUNTER_0_BASE, 2);
  return currentCoordinate;
}

coordinate objectPos(coordinate coords, coordinate offset){ //equivalent to objectPos
   //PERF_BEGIN(PERFORMANCE_COUNTER_0_BASE, 3);
  coordinate result;
  result.x = coords.x+offset.x+2; //finds the absolute position of the object
  result.y = coords.y+offset.y+2;
#ifdef DEBUG
  printAsciiHidden(&grayed, 64, 64, result.x, result.y, 4, 10);
#endif
   //PERF_END(PERFORMANCE_COUNTER_0_BASE, 3);
  return result;
}
void resetArray(){ //used for resetting the xcorr2 array after every cycle
	 int i, j;
	 for(i = 0; i < cropSIZE-4; i++) {
	     for(j = 0; j < cropSIZE-4; j++) {
	    	 xcorr2Result[i][j] = 0;
	     }
	 }
}

int main()
{
	detect.x=-1; //initialize to a value outside the values of detect
  #ifdef PERFORMANCE
	  /* Reset Performance Counter */
 	 PERF_RESET(PERFORMANCE_COUNTER_0_BASE);
 	/* Start Measuring */
  	PERF_START_MEASURING (PERFORMANCE_COUNTER_0_BASE);
    #endif

	int i;
  int j;
  #ifdef PERFORMANCE
  PERF_BEGIN(PERFORMANCE_COUNTER_0_BASE, 1);
  for(j = 0; j<2; j++){
    #endif
    for(i=0; i<4;i++){
    getImage(i); //gets image and converts it gray scale i.e. 64 x 64 Matrix
    coordinate previous = delay(); //get previous coordinate
    coordinate coords = calcCoord(previous);
    crop(coords); //crops 31 x 31 Matrix from image
    xcorr2(); //
    coordinate offset = posMax();//calculate coordinate for the maximum value
    detect = objectPos(coords,offset); //find the absolute coordinate of the object

    //printf("previous : %d %d\n",previous.x,previous.y);
    //printf("coords : %d %d\n",coords.x,coords.y);
    //printf("offset : %d %d\n",offset.x,offset.y);
    #ifdef DEBUG
      printf("@coordinate [%d,%d]\n",detect.x,detect.y);
      printf("\n");
    #endif

    resetArray();
  }
  #ifdef PERFORMANCE
  i=0;
}
  #endif
    #ifdef PERFORMANCE
    PERF_END(PERFORMANCE_COUNTER_0_BASE, i+1);
  /* End Measuring */
  PERF_STOP_MEASURING(PERFORMANCE_COUNTER_0_BASE);
  /* Print measurement report */
  perf_print_formatted_report
	 (PERFORMANCE_COUNTER_0_BASE,
	  ALT_CPU_FREQ,                 // defined in "system.h"
	  1,                            // How many sections to print
	  "counter 1");    // Display-name of section(s).

    double total_time =  (double)perf_get_section_time(PERFORMANCE_COUNTER_0_BASE, 1)/(double)alt_get_cpu_freq();
    double normalized_time = total_time/4;
    double throughput = 1/normalized_time;
    printf("execution time: %f\nnormalized time: %f\nthroughput: %f\n", total_time, normalized_time, throughput);
#endif
/**
  printf("\n");
  printf("factorial(5) = %x (hexadecimal)\n",fac5);
  printf("factorial(10) = %x (hexadecimal)\n",fac10);
  printf("factorial(15) = %x (hexadecimal)\n",fac15);
*/
  /* Event loop never exits. */
  while (1);

  return 0;
}
