/* Date: 2019-2-5
 * Author: Jun Zhang, Jaakko Laiho
 * Email�� zhangjun32108@outlook.com 
 * Description: An image processing program that captures the required pattern in each frame of the input flow
 */

/*
 * Things have done: The single-core bare medal program is basically done. We have tested it on laptop but not on the board.
 * 
 * 
 * Todo list 1: Test the source file on the Board
 * Todo list 2: Optimize the code in terms of time complexity and space complexity
 */

#include <stdio.h>
#include <stdlib.h>
#include "system.h"
#include "altera_avalon_performance_counter.h"

#define SECTION_GRAYSDF 		1
#define SECTION_CALCCOORDSDF	2
#define SECTION_CROPSDF			3
#define SECTION_XCORR2SDF		4
#define SECTION_CALCPOSSDF		5

				

//Path:\il2212-lab-master\app\hello_image\src_0
//Contain the RGB value of tesing image. Using this test .h file can avoid reading data from the PPM file. 
#include "images.h"
#include "alt_types.h"

//Define two new datatypes to save memory. Since the maximum grayscale value is 255, INT8U (8 bits) should be big enough to hold most variables.
//Only when we call xcorr2 functions the dot product value is possible to be greater than 255. So we use INT16U to hold that value.  
typedef unsigned char INT8U;
typedef unsigned short INT16U;

#define dSPAN  15
#define cropSIZE  36
#define xPATTERN_length  5
#define offset_size_length 32

//INT8U dSPAN = 15;
//Here I use 33 instead of 31 because if we use 31, it may not capture the whole pattern when the pattern move 15 pixels in one frame
//INT8U cropSIZE = 36; //cropSIZE = 2 * dsPAN + 1 + 5 - 3; the value is supposed to be 33. Since we are going to use burst reading, we will find the next value which is the mutiple of 4.
// INT8U xPATTERN[5][5] = {{0,0,1,0,0},
// 					  {0,1,0,1,0},
// 					  {1,0,0,0,1},
// 					  {0,1,0,1,0},
// 					  {0,0,1,0,0}};					  
//INT8U xPATTERN_length = 5;
//INT8U offset_size_length = 32; // offset_size_length = cropSIZE(33,the real value of cropSIZE) - 4


INT8U INT8U_poINT8Uer_size = sizeof(INT8U*);
INT8U INT8U_size = sizeof(INT8U);

//Print the 2-D matrix with INT8U datatype
void printMatrix(INT8U *inputMatrix, INT8U matrix_h, INT8U matrix_w)
{
	INT8U i;
	INT8U j;
	printf("============================================================\n");
	printf("============================================================\n");
	printf("\n");
	printf("[");
	for(i = 0; i < matrix_h; i++)
	{
		printf("[	");
		for(j = 0; j < matrix_w; j++)
		{
			printf("  %d	",*(inputMatrix+matrix_w*i+j));
		    //printf("  %d	",inputMatrix[i][j]);
		}
		printf("]");
		printf("\n");
	}
	printf("]");
	printf("============================================================\n");
	printf("============================================================\n");
	printf("\n");
}

//Print the 2-D matrix with INT16U datatype
void printMatrix_INT16U(INT16U **inputMatrix, INT8U matrix_h, INT8U matrix_w)
{
	INT8U i;
	INT8U j;
	printf("============================================================\n");
	printf("============================================================\n");
	printf("\n");
	printf("[");
	for(i = 0; i < matrix_h; i++)
	{
		printf("[	");
		for(j = 0; j < matrix_w; j++)
		{
			printf("  %d	",inputMatrix[i][j]);
		}
		printf("]");
		printf("\n");
	}
	printf("]");
	printf("============================================================\n");
	printf("============================================================\n");
	printf("\n");
}


// ############################################################################################
// First crop then do the greyscale conversion
// transform RGB value to greyscale value
// Input: a 2D-matrix with the size of cropSIZE * (cropSIZE * 3), cropSIZE, cropSIZE
// Output: a 2D-matrix with the size of cropSIZE * cropSIZE
void groupV_3(INT8U *inputMatrix, INT8U matrix_row, INT8U matrix_col, INT8U* group)
{
	INT8U i;
	INT8U j;
	INT8U* row_add;

	for ( i = 0; i < matrix_row; i++)
	{
		//group[i] = group[0] + matrix_col * i;
		row_add = inputMatrix + i*matrix_col*3;
		for(j = 0; j < matrix_col*3; j = j+36)
		{
			//one algorithm to transform RGB value to grayscale value which is suitable for operation between 16-bit variables
			//the good balance between speed and precision
			//group[i][j/3] = ((inputMatrix[i][j]*38
			//			 +inputMatrix[i][j+1]*75
			//			 +inputMatrix[i][j+2]*15)>>7);

			//Another grayscale algorithm, lose some precison but increase the speed
			//using loop unrolling, assume that we know the cropSIZE is 36
			*(group+i*matrix_col+j/3)   = (((*(row_add +j ))>>2 )
						 +((*(row_add +j+ 1))>>1 )
						 +((*(row_add +j+ 2))>>3));
			*(group+i*matrix_col+j/3 + 1) = (((*(row_add +j + 3))>>2 )
						 +((*(row_add +j+ 4))>>1 )
						 +((*(row_add +j+ 5))>>3));
			*(group+i*matrix_col+j/3 + 2) = (((*(row_add +j + 6 ))>>2 )
						 +((*(row_add +j+ 7))>>1 )
						 +((*(row_add +j+ 8))>>3));
			*(group+i*matrix_col+j/3 + 3)= (((*(row_add +j + 9))>>2 )
						 +((*(row_add +j+ 10))>>1 )
						 +((*(row_add +j+ 11))>>3));
			*(group+i*matrix_col+j/3 + 4) = (((*(row_add +j + 12))>>2 )
						 +((*(row_add +j+ 13))>>1 )
						 +((*(row_add +j+ 14))>>3));
			*(group+i*matrix_col+j/3 + 5) = (((*(row_add +j + 15))>>2 )
						 +((*(row_add +j+ 16))>>1 )
						 +((*(row_add +j+ 17))>>3));
			*(group+i*matrix_col+j/3 + 6) = (((*(row_add +j + 18))>>2 )
						 +((*(row_add +j+ 19 ))>>1 )
						 +((*(row_add +j+ 20))>>3));
			*(group+i*matrix_col+j/3 + 7) = (((*(row_add +j + 21))>>2 )
						 +((*(row_add +j+ 22))>>1 )
						 +((*(row_add +j+ 23))>>3));
			*(group+i*matrix_col+j/3 + 8) = (((*(row_add +j + 24))>>2 )
						 +((*(row_add +j+ 25))>>1 )
						 +((*(row_add +j+ 26))>>3));
			*(group+i*matrix_col+j/3 + 9) = (((*(row_add +j + 27))>>2 )
						 +((*(row_add +j+ 28))>>1 )
						 +((*(row_add +j+ 29))>>3));
			*(group+i*matrix_col+j/3 + 10) = (((*(row_add +j + 30))>>2 )
						 +((*(row_add +j+ 31 ))>>1 )
						 +((*(row_add +j+ 32))>>3));
			*(group+i*matrix_col+j/3 + 11) = (((*(row_add +j + 33))>>2 )
						 +((*(row_add +j+ 34))>>1 )
						 +((*(row_add +j+ 35))>>3));


		}		
	}
	//printf("Grayed Matrix\n");
	//printMatrix(group, matrix_row, matrix_col);
}
// ############################################################################################




// ############################################################################################
// First crop the image then do the grayscale convertion
// Cropped the image into a smaller piece with the size of  cropSIZE*cropSIZE*3, here img_w should be img_w_0
// here startingPoINT8U_x means the index of row, startingPoINT8U_y means the index of column
INT8U* crop(INT8U startingPoINT8U_x, INT8U startingPoINT8U_y, INT8U* inputMatrix, INT8U img_w)
{
	INT8U i;
	INT8U j;
	INT8U* staringPointAddress_forRow;
	INT8U* startingPoINT8UAddress = inputMatrix+img_w * startingPoINT8U_x + startingPoINT8U_y*3;
	// printf("Staring Point Address in memory: %d\n", startingPoINT8UAddress); 
	int** group = (int**)calloc(cropSIZE, sizeof(int*));
    if(group == NULL)
    {
    	printf("Calloc Error! Inside the crop function, location1\n");
    	//exit(0);
    }
    group[0] = (int*)calloc(cropSIZE*cropSIZE*3/4, sizeof(int));
    if(group[0] == NULL)
    {
    	printf("Calloc Error! Inside the crop function, location2\n");
    	//exit(0);
    }
	for ( i = 0; i < cropSIZE; i++)
	{
		group[i] = group[0] + i*cropSIZE*3/4;
		staringPointAddress_forRow = startingPoINT8UAddress + img_w*i;
		for( j = 0; j < cropSIZE*3/4; j++)
		{
			//group[i][j] = *(startingPoINT8UAddress + img_w*i + j); 
			group[i][j]   = *((int*)(staringPointAddress_forRow+j*4)); 
			// group[i][j+1] = *(staringPointAddress_forRow + j+1); 
			// group[i][j+2] = *(staringPointAddress_forRow + j+2); 

		}		
	}
	// printf("Cropped Matrix\n");
	// printMatrix(group, cropSIZE, cropSIZE*3);
	return (INT8U*)group[0];
}
// ############################################################################################




//Optimization poINT8U： store the INT8Uermediate value to speed up the program
void xcorr2(INT8U *inputMatrix, INT8U inputMatrix_w, INT16U *inputPointer)
{
	int i;
	INT16U j;
	INT8U output_size_length = offset_size_length;
	INT8U *startingPoINT8UAddress;
	for ( i = 0; i < output_size_length; i++)
	{
		for( j = 0; j < output_size_length; j=j+8)
		{

			// startingPoINT8UAddress = *inputMatrix + inputMatrix_w*i + j;

			// // unroll the loop partially and assume the pattern we want to detected is fixed
			// *(inputPointer + i*output_size_length + j) = *(startingPoINT8UAddress + 2) + *(startingPoINT8UAddress + inputMatrix_w +  1) + *(startingPoINT8UAddress + inputMatrix_w +  3) + *(startingPoINT8UAddress + inputMatrix_w*2) + *(startingPoINT8UAddress + inputMatrix_w*2 +  4)
			// 		+ *(startingPoINT8UAddress + inputMatrix_w*3 +  1) + *(startingPoINT8UAddress + inputMatrix_w*3 +  3) + *(startingPoINT8UAddress + inputMatrix_w*4 +  2);
			// //product = (*(startingPoINT8UAddress + inputMatrix_w*m + n));

			// startingPoINT8UAddress = startingPoINT8UAddress+1;
			// *(inputPointer + i*output_size_length + j + 1) = *(startingPoINT8UAddress + 2) + *(startingPoINT8UAddress + inputMatrix_w +  1) + *(startingPoINT8UAddress + inputMatrix_w +  3) + *(startingPoINT8UAddress + inputMatrix_w*2) + *(startingPoINT8UAddress + inputMatrix_w*2 +  4)
			// 		+ *(startingPoINT8UAddress + inputMatrix_w*3 +  1) + *(startingPoINT8UAddress + inputMatrix_w*3 +  3) + *(startingPoINT8UAddress + inputMatrix_w*4 +  2);

			// startingPoINT8UAddress = startingPoINT8UAddress+1;
			// *(inputPointer + i*output_size_length + j + 2) = *(startingPoINT8UAddress + 2) + *(startingPoINT8UAddress + inputMatrix_w +  1) + *(startingPoINT8UAddress + inputMatrix_w +  3) + *(startingPoINT8UAddress + inputMatrix_w*2) + *(startingPoINT8UAddress + inputMatrix_w*2 +  4)
			// 		+ *(startingPoINT8UAddress + inputMatrix_w*3 +  1) + *(startingPoINT8UAddress + inputMatrix_w*3 +  3) + *(startingPoINT8UAddress + inputMatrix_w*4 +  2);

			// startingPoINT8UAddress = startingPoINT8UAddress+1;
			// *(inputPointer + i*output_size_length + j + 3) = *(startingPoINT8UAddress + 2) + *(startingPoINT8UAddress + inputMatrix_w +  1) + *(startingPoINT8UAddress + inputMatrix_w +  3) + *(startingPoINT8UAddress + inputMatrix_w*2) + *(startingPoINT8UAddress + inputMatrix_w*2 +  4)
			// 		+ *(startingPoINT8UAddress + inputMatrix_w*3 +  1) + *(startingPoINT8UAddress + inputMatrix_w*3 +  3) + *(startingPoINT8UAddress + inputMatrix_w*4 +  2);

			// startingPoINT8UAddress = startingPoINT8UAddress+1;
			// *(inputPointer + i*output_size_length + j + 4) = *(startingPoINT8UAddress + 2) + *(startingPoINT8UAddress + inputMatrix_w +  1) + *(startingPoINT8UAddress + inputMatrix_w +  3) + *(startingPoINT8UAddress + inputMatrix_w*2) + *(startingPoINT8UAddress + inputMatrix_w*2 +  4)
			// 		+ *(startingPoINT8UAddress + inputMatrix_w*3 +  1) + *(startingPoINT8UAddress + inputMatrix_w*3 +  3) + *(startingPoINT8UAddress + inputMatrix_w*4 +  2);

			// startingPoINT8UAddress = startingPoINT8UAddress+1;
			// *(inputPointer + i*output_size_length + j + 5) = *(startingPoINT8UAddress + 2) + *(startingPoINT8UAddress + inputMatrix_w +  1) + *(startingPoINT8UAddress + inputMatrix_w +  3) + *(startingPoINT8UAddress + inputMatrix_w*2) + *(startingPoINT8UAddress + inputMatrix_w*2 +  4)
			// 		+ *(startingPoINT8UAddress + inputMatrix_w*3 +  1) + *(startingPoINT8UAddress + inputMatrix_w*3 +  3) + *(startingPoINT8UAddress + inputMatrix_w*4 +  2);

			// startingPoINT8UAddress = startingPoINT8UAddress+1;
			// *(inputPointer + i*output_size_length + j + 6) = *(startingPoINT8UAddress + 2) + *(startingPoINT8UAddress + inputMatrix_w +  1) + *(startingPoINT8UAddress + inputMatrix_w +  3) + *(startingPoINT8UAddress + inputMatrix_w*2) + *(startingPoINT8UAddress + inputMatrix_w*2 +  4)
			// 		+ *(startingPoINT8UAddress + inputMatrix_w*3 +  1) + *(startingPoINT8UAddress + inputMatrix_w*3 +  3) + *(startingPoINT8UAddress + inputMatrix_w*4 +  2);

			// startingPoINT8UAddress = startingPoINT8UAddress+1;
			// *(inputPointer + i*output_size_length + j + 7) = *(startingPoINT8UAddress + 2) + *(startingPoINT8UAddress + inputMatrix_w +  1) + *(startingPoINT8UAddress + inputMatrix_w +  3) + *(startingPoINT8UAddress + inputMatrix_w*2) + *(startingPoINT8UAddress + inputMatrix_w*2 +  4)
			// 		+ *(startingPoINT8UAddress + inputMatrix_w*3 +  1) + *(startingPoINT8UAddress + inputMatrix_w*3 +  3) + *(startingPoINT8UAddress + inputMatrix_w*4 +  2);



			startingPoINT8UAddress = inputMatrix + inputMatrix_w*i + j;
			*(inputPointer + (i<<5) + j ) = *(startingPoINT8UAddress + 2) + *(startingPoINT8UAddress + inputMatrix_w +  1) + *(startingPoINT8UAddress + inputMatrix_w +  3) + *(startingPoINT8UAddress + (inputMatrix_w<<1)) + *(startingPoINT8UAddress + (inputMatrix_w<<1) +  4)
					+ *(startingPoINT8UAddress + inputMatrix_w*3 +  1) + *(startingPoINT8UAddress + inputMatrix_w*3 +  3) + *(startingPoINT8UAddress + (inputMatrix_w<<2) +  2);

			startingPoINT8UAddress = startingPoINT8UAddress+1;
			*(inputPointer + (i<<5) + j + 1 ) = *(startingPoINT8UAddress + 2) + *(startingPoINT8UAddress + inputMatrix_w +  1) + *(startingPoINT8UAddress + inputMatrix_w +  3) + *(startingPoINT8UAddress + (inputMatrix_w<<1)) + *(startingPoINT8UAddress + (inputMatrix_w<<1) +  4)
					+ *(startingPoINT8UAddress + inputMatrix_w*3 +  1) + *(startingPoINT8UAddress + inputMatrix_w*3 +  3) + *(startingPoINT8UAddress + (inputMatrix_w<<2) +  2);

			startingPoINT8UAddress = startingPoINT8UAddress+1;
			*(inputPointer + (i<<5) + j + 2) = *(startingPoINT8UAddress + 2) + *(startingPoINT8UAddress + inputMatrix_w +  1) + *(startingPoINT8UAddress + inputMatrix_w +  3) + *(startingPoINT8UAddress + (inputMatrix_w<<1)) + *(startingPoINT8UAddress + (inputMatrix_w<<1) +  4)
					+ *(startingPoINT8UAddress + inputMatrix_w*3 +  1) + *(startingPoINT8UAddress + inputMatrix_w*3 +  3) + *(startingPoINT8UAddress + (inputMatrix_w<<2) +  2);

			startingPoINT8UAddress = startingPoINT8UAddress+1;
			*(inputPointer + (i<<5) + j + 3) = *(startingPoINT8UAddress + 2) + *(startingPoINT8UAddress + inputMatrix_w +  1) + *(startingPoINT8UAddress + inputMatrix_w +  3) + *(startingPoINT8UAddress + (inputMatrix_w<<1)) + *(startingPoINT8UAddress + (inputMatrix_w<<1) +  4)
					+ *(startingPoINT8UAddress + inputMatrix_w*3 +  1) + *(startingPoINT8UAddress + inputMatrix_w*3 +  3) + *(startingPoINT8UAddress + (inputMatrix_w<<2) +  2);

			startingPoINT8UAddress = startingPoINT8UAddress+1;
			*(inputPointer + (i<<5) + j + 4) = *(startingPoINT8UAddress + 2) + *(startingPoINT8UAddress + inputMatrix_w +  1) + *(startingPoINT8UAddress + inputMatrix_w +  3) + *(startingPoINT8UAddress + (inputMatrix_w<<1)) + *(startingPoINT8UAddress + (inputMatrix_w<<1) +  4)
					+ *(startingPoINT8UAddress + inputMatrix_w*3 +  1) + *(startingPoINT8UAddress + inputMatrix_w*3 +  3) + *(startingPoINT8UAddress + (inputMatrix_w<<2) +  2);

			startingPoINT8UAddress = startingPoINT8UAddress+1;
			*(inputPointer + (i<<5) + j + 5) = *(startingPoINT8UAddress + 2) + *(startingPoINT8UAddress + inputMatrix_w +  1) + *(startingPoINT8UAddress + inputMatrix_w +  3) + *(startingPoINT8UAddress + (inputMatrix_w<<1)) + *(startingPoINT8UAddress + (inputMatrix_w<<1) +  4)
					+ *(startingPoINT8UAddress + inputMatrix_w*3 +  1) + *(startingPoINT8UAddress + inputMatrix_w*3 +  3) + *(startingPoINT8UAddress + (inputMatrix_w<<2) +  2);

			startingPoINT8UAddress = startingPoINT8UAddress+1;
			*(inputPointer + (i<<5) + j + 6) = *(startingPoINT8UAddress + 2) + *(startingPoINT8UAddress + inputMatrix_w +  1) + *(startingPoINT8UAddress + inputMatrix_w +  3) + *(startingPoINT8UAddress + (inputMatrix_w<<1)) + *(startingPoINT8UAddress + (inputMatrix_w<<1) +  4)
					+ *(startingPoINT8UAddress + inputMatrix_w*3 +  1) + *(startingPoINT8UAddress + inputMatrix_w*3 +  3) + *(startingPoINT8UAddress + (inputMatrix_w<<2) +  2);

			startingPoINT8UAddress = startingPoINT8UAddress+1;
			*(inputPointer + (i<<5) + j + 7) = *(startingPoINT8UAddress + 2) + *(startingPoINT8UAddress + inputMatrix_w +  1) + *(startingPoINT8UAddress + inputMatrix_w +  3) + *(startingPoINT8UAddress + (inputMatrix_w<<1)) + *(startingPoINT8UAddress + (inputMatrix_w<<1) +  4)
					+ *(startingPoINT8UAddress + inputMatrix_w*3 +  1) + *(startingPoINT8UAddress + inputMatrix_w*3 +  3) + *(startingPoINT8UAddress + (inputMatrix_w<<2) +  2);

		}		
	}
	//return_pointer = xcropp2ed;
	//printf("Xcropp2ed Matrix\n");
	//printMatrix_INT16U(return_pointer, output_size_length,output_size_length);
	//return return_pointer;
}


// The inputMatrix here is the xcorred Matrix
// Calculate the offset with largest value and their coords
void posMax_coords(INT16U* inputMatrix, INT8U cropCoord_x, INT8U cropCoord_y, INT16U* returnVar)
{
	INT16U output[3];
	INT8U i;
	INT8U j;
	INT16U max = 0;
	for(i = 0; i<offset_size_length; i++)
		for(j = 0; j <offset_size_length; j++)
		{
			if(max < *(inputMatrix+offset_size_length*i+j))
			{
				max = *(inputMatrix+offset_size_length*i+j);
				output[0] = i;
				output[1] = j;
				output[2] = max;
			}
		}
	*returnVar 	   = output[0] + cropCoord_x + 2;
	*(returnVar+1) = output[1] + cropCoord_y + 2;
	*(returnVar+2) = output[2];
	// printf("CropPoints:	%d, %d\n",cropCoord_x,cropCoord_y);
 // 	printf("Offset:	    %d, %d, %d\n",output[0],output[1], output[2]);
 // 	printf("returnVar:  %d, %d, %d\n",returnVar[0],returnVar[1], returnVar[2]);
}




//Calculate the next cropped point according to the previous detected pattern position ( the middle point of the detected pattern)
void calcCoord(INT16U *previousPoINT8U, INT8U img_h, INT8U img_w, INT8U* returnVar )
{
	INT8U output[2];
	if(previousPoINT8U[0] <= dSPAN)
		output[0] = 0;
	else if (previousPoINT8U[0] > (img_h - dSPAN))
		output[0] = img_h - cropSIZE;
	else
		output[0] = previousPoINT8U[0] - dSPAN ;
	if(previousPoINT8U[1] <= dSPAN)
		output[1] = 0;
	else if (previousPoINT8U[1] > (img_w - dSPAN))
		output[1] = img_w - cropSIZE;
	else
		output[1] = previousPoINT8U[1] - dSPAN ;
	//printf("Next cropped point: %d, %d . \n",output[0],output[1]);
	*returnVar 		 = output[0];
	*(returnVar + 1) = output[1];
	//printf("Return point: %d, %d . \n",returnVar[0],returnVar[1]);
}

INT8U main()
{
	//each image is represented by a list
	//First value of the list is the value of image height
	//Second value of the list is the value of the real value of image weight
	//Third value of the list is the value of maximum color
	//Later is the R, G, B values of each pixel point, respectively, all the RGB values are oganized in an consecutive order accroding to C language convention

    INT8U counter;
	INT8U *image;
	INT8U img_h = 0;
	INT8U img_w = 0;
	INT8U img_w_0 = 0;
	INT8U grayed[cropSIZE*cropSIZE];
	INT16U detected[3];
	INT8U coords[2];
	INT8U *croped;


	INT16U xcropp2ed[offset_size_length*offset_size_length];
	// alt_u64 xcropp2ed[offset_size_length*offset_size_length/4];

    img_w = image_sequence[0][0];
	img_h = image_sequence[0][1];
	//Extended by the RGB value
	img_w_0 = img_w * 3;

	
	detected[0] = 0;
	detected[1] = 0;


    for( counter = 0; counter < sequence_length; counter++)
    {
    	
		image = image_sequence[counter] + 3;

	    PERF_RESET(PERFORMANCE_COUNTER_0_BASE);
		PERF_START_MEASURING (PERFORMANCE_COUNTER_0_BASE);
		
	    PERF_BEGIN(PERFORMANCE_COUNTER_0_BASE, SECTION_CALCCOORDSDF);
	    calcCoord(detected, img_h, img_w, coords );
	    PERF_END(PERFORMANCE_COUNTER_0_BASE, SECTION_CALCCOORDSDF);

	    PERF_BEGIN(PERFORMANCE_COUNTER_0_BASE, SECTION_CROPSDF);

	    croped = crop(coords[0],coords[1],image, img_w_0);

		PERF_END(PERFORMANCE_COUNTER_0_BASE, SECTION_CROPSDF);

	    PERF_BEGIN(PERFORMANCE_COUNTER_0_BASE, SECTION_GRAYSDF);
	    groupV_3(croped, cropSIZE, cropSIZE, grayed);

	   	free(croped);
		PERF_END(PERFORMANCE_COUNTER_0_BASE, SECTION_GRAYSDF);

		PERF_BEGIN(PERFORMANCE_COUNTER_0_BASE, SECTION_XCORR2SDF);

	   	xcorr2(grayed, cropSIZE, xcropp2ed);
	   	

	   	PERF_END(PERFORMANCE_COUNTER_0_BASE, SECTION_XCORR2SDF);


	    PERF_BEGIN(PERFORMANCE_COUNTER_0_BASE, SECTION_CALCPOSSDF);

	    posMax_coords(xcropp2ed, coords[0], coords[1], detected);
	    

	    PERF_END(PERFORMANCE_COUNTER_0_BASE, SECTION_CALCPOSSDF);
	    
	    printf("Test image %d : %d , %d \n",counter,detected[0],detected[1]);
	    
	    PERF_STOP_MEASURING (PERFORMANCE_COUNTER_0_BASE);

	    perf_print_formatted_report
	    		(PERFORMANCE_COUNTER_0_BASE,
	    		ALT_CPU_FREQ,        // defined in "system.h"
	    		5,                   // How many sections to print
	    		"graySDF",
				"calcCoordSDF",
				"cropSDF",
				"xcorr2SDF",
				"calcPosSDF"
	    		);
    }


	return 0;
}
