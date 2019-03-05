//#include <stdio.h>
#include "system.h"
//#include "io.h"
#include "sys/alt_stdio.h"
#include <altera_avalon_mutex.h>

#define shared_address_offset_for_cpu4 2592 // = 3*8*36*3
#define cropSIZE  36
#define img_h 64
#define img_w 64
#define img_w_0 192 // = 64*3
#define image_piece_h 12
#define offset_size_length 32
#define offset_size_height 8  // = 12 - 4
#define cropSIZE_int_RGB 27  // = 36*3/4 
#define cropSIZE_TIMES_3 108
#define results_offset 4000 //results from other CPUs are stored at a place starting from shared+4000
typedef unsigned char INT8U;
typedef unsigned short INT16U;

extern void delay (int millisec);
char count = 0;

unsigned char* shared = (unsigned char*) SHARED_ONCHIP_BASE;

//Print the 2-D matrix with INT8U datatype
void printMatrix(INT8U *inputMatrix, INT8U matrix_h, INT8U matrix_w)
{
	INT8U i;
	INT8U j;
	alt_putstr("============================================================\n");
	alt_putstr("============================================================\n");
	alt_putstr("\n");
	alt_putstr("[");
	for(i = 0; i < matrix_h; i++)
	{
		alt_putstr("[	");
		for(j = 0; j < matrix_w; j++)
		{
			alt_printf("  %x	",*(inputMatrix+matrix_w*i+j));
		    //alt_printf("  %d	",inputMatrix[i][j]);
		}
		alt_putstr("]");
		alt_putstr("\n");
	}
	alt_putstr("]");
	alt_putstr("============================================================\n");
	alt_putstr("============================================================\n");
	alt_putstr("\n");
}

//Print the 2-D matrix with INT8U datatype
void printMatrix_INT16U(INT16U *inputMatrix, INT8U matrix_h, INT8U matrix_w)
{
	INT8U i;
	INT8U j;
	alt_putstr("============================================================\n");
	alt_putstr("============================================================\n");
	alt_putstr("\n");
	alt_putstr("[");
	for(i = 0; i < matrix_h; i++)
	{
		alt_putstr("[	");
		for(j = 0; j < matrix_w; j++)
		{
			alt_printf("  %x	",*(inputMatrix+matrix_w*i+j));
		    //alt_printf("  %d	",inputMatrix[i][j]);
		}
		alt_putstr("]");
		alt_putstr("\n");
	}
	alt_putstr("]");
	alt_putstr("============================================================\n");
	alt_putstr("============================================================\n");
	alt_putstr("\n");
}

int main()
{ 
	int i;
	INT8U j;
	INT8U image_piece[1296];  // = 12*36*3
	INT8U grayed[432];		  // = 12*36
	INT16U xcropp2ed[offset_size_height*offset_size_length];
	int* writePointer;
	INT16U* writeBackPointer;
	INT8U *row_add;
	INT8U *startingPoINT8UAddress;
	INT16U output[3];
	INT16U max=0;
	// image piece is stored from shared ~ shared + 12*36*3 -1
	int* readAddress;



	// alt_putstr("Hello cpu_4!\n");
	alt_mutex_dev* mutex_4 = altera_avalon_mutex_open( "/dev/mutex_4" );
  	if(mutex_4) {
  		// alt_putstr("cpu_4 found mutex_4 handle\n");
  	}

  	/*
	 * NEW PART - START FROM THE SAME LINE
	 * */
  	
  	altera_avalon_mutex_lock(mutex_4, 1);
	delay(1);
	altera_avalon_mutex_unlock(mutex_4);
	delay(10);
  	
	/*
	 * END 
  	*/
  	while (1) {
		//if(!altera_avalon_mutex_is_mine(mutex_4)) {
			altera_avalon_mutex_lock(mutex_4, 1);
			// alt_putstr("cpu_4 reading image\n");
			
			/*
			* TODO: read image from shared memory here
			* */
			//delay(100); 	//simulating read time
			readAddress = (int*)(shared + shared_address_offset_for_cpu4 )- cropSIZE_int_RGB;
			writePointer = (int*)image_piece - cropSIZE_int_RGB;
			for( i = 0; i < image_piece_h; i++)
			{
				writePointer += cropSIZE_int_RGB;
				readAddress  += cropSIZE_int_RGB;
				for ( j = 0; j < cropSIZE_int_RGB; j = j +9)
				{
					*(writePointer+j)     = *(readAddress +j);
					*(writePointer+j + 1) = *(readAddress +j + 1); 
					*(writePointer+j + 2) = *(readAddress +j + 2); 
					*(writePointer+j + 3) = *(readAddress +j + 3); 
					*(writePointer+j + 4) = *(readAddress +j + 4); 
					*(writePointer+j + 5) = *(readAddress +j + 5); 
					*(writePointer+j + 6) = *(readAddress +j + 6); 
					*(writePointer+j + 7) = *(readAddress +j + 7); 
					*(writePointer+j + 8) = *(readAddress +j + 8); 
				}
			}					




			// alt_printf("cpu_4 read %x\n", *shared);
			
			//signal read completion
			altera_avalon_mutex_unlock(mutex_4);
			//delay(1);
			//altera_avalon_mutex_lock(mutex_4, 1);
			
			/*
			* TODO: put grayscaling here
			* */
			// alt_putstr("cpu_4 grayscaling\n");
			//delay(100); 	//simulating read time

			INT16U j_by_3;
									INT16U i_cropSIZE;
									INT16U j_by_3_plus_i_cropSIZE;
									
									for ( i = 0; i < image_piece_h; i++)
									{
										i_cropSIZE = i * cropSIZE; 
										//grayed[i] = grayed[0] + cropSIZE * i;
										row_add = image_piece + i*cropSIZE_TIMES_3;
										for(j = 0; j < cropSIZE_TIMES_3; j = j+36)
										{
											j_by_3 = j / 3;
											j_by_3_plus_i_cropSIZE = j_by_3 + i_cropSIZE;
											
											//one algorithm to transform RGB value to grayscale value which is suitable for operation between 16-bit variables
											//the good balance between speed and precision
											//grayed[i][j/3] = ((inputMatrix[i][j]*38
											//			 +inputMatrix[i][j+1]*75
											//			 +inputMatrix[i][j+2]*15)>>7);

											//Another grayscale algorithm, lose some precison but increase the speed
											//using loop unrolling, assume that we know the cropSIZE is 36
											*(grayed+j_by_3_plus_i_cropSIZE)   = (((*(row_add +j ))>>2 )
														 +((*(row_add +j+ 1))>>1 )
														 +((*(row_add +j+ 2))>>3));
											*(grayed+j_by_3_plus_i_cropSIZE + 1) = (((*(row_add +j + 3))>>2 )
														 +((*(row_add +j+ 4))>>1 )
														 +((*(row_add +j+ 5))>>3));
											*(grayed+j_by_3_plus_i_cropSIZE + 2) = (((*(row_add +j + 6 ))>>2 )
														 +((*(row_add +j+ 7))>>1 )
														 +((*(row_add +j+ 8))>>3));
											*(grayed+j_by_3_plus_i_cropSIZE + 3)= (((*(row_add +j + 9))>>2 )
														 +((*(row_add +j+ 10))>>1 )
														 +((*(row_add +j+ 11))>>3));
											*(grayed+j_by_3_plus_i_cropSIZE + 4) = (((*(row_add +j + 12))>>2 )
														 +((*(row_add +j+ 13))>>1 )
														 +((*(row_add +j+ 14))>>3));
											*(grayed+j_by_3_plus_i_cropSIZE + 5) = (((*(row_add +j + 15))>>2 )
														 +((*(row_add +j+ 16))>>1 )
														 +((*(row_add +j+ 17))>>3));
											*(grayed+j_by_3_plus_i_cropSIZE + 6) = (((*(row_add +j + 18))>>2 )
														 +((*(row_add +j+ 19 ))>>1 )
														 +((*(row_add +j+ 20))>>3));
											*(grayed+j_by_3_plus_i_cropSIZE + 7) = (((*(row_add +j + 21))>>2 )
														 +((*(row_add +j+ 22))>>1 )
														 +((*(row_add +j+ 23))>>3));
											*(grayed+j_by_3_plus_i_cropSIZE + 8) = (((*(row_add +j + 24))>>2 )
														 +((*(row_add +j+ 25))>>1 )
														 +((*(row_add +j+ 26))>>3));
											*(grayed+j_by_3_plus_i_cropSIZE + 9) = (((*(row_add +j + 27))>>2 )
														 +((*(row_add +j+ 28))>>1 )
														 +((*(row_add +j+ 29))>>3));
											*(grayed+j_by_3_plus_i_cropSIZE + 10) = (((*(row_add +j + 30))>>2 )
														 +((*(row_add +j+ 31 ))>>1 )
														 +((*(row_add +j+ 32))>>3));
											*(grayed+j_by_3_plus_i_cropSIZE + 11) = (((*(row_add +j + 33))>>2 )
														 +((*(row_add +j+ 34))>>1 )
														 +((*(row_add +j+ 35))>>3));

										}		
									}

									/*
									* TODO: xcorr2 here
									* */
									//alt_putstr("cpu_1 xcorr2'ing image\n");
									//delay(100); 	//simulate xcorr time
									INT16U i_times_32;
									INT16U i_times_32_plus_j;
									INT16U cropSIZE_TIMES_3_plus_1 = cropSIZE_TIMES_3 + 1;
									INT16U cropSIZE_TIMES_3_plus_3 = cropSIZE_TIMES_3 + 3;
									INT16U cropSIZE_plus_1 = cropSIZE + 1;
									INT16U cropSIZE_plus_3 = cropSIZE + 3;
									INT16U cropSIZE_times_2 = cropSIZE<<1;
									INT16U cropSIZE_times_4 = cropSIZE<<2;
									INT16U cropSIZE_times_4_plus_2 = cropSIZE_times_4 + 2; 
									INT16U cropSIZE_times_2_plus_4 = cropSIZE_times_2 + 4;
									max = 0;
									
									for ( i = 0; i < offset_size_height; i++)
									{
										i_times_32 = i<<5;
										for( j = 0; j < offset_size_length; j=j+8)
										{

											// startingPoINT8UAddress = *inputMatrix + cropSIZE*i + j;
											i_times_32_plus_j = i_times_32 + j;
											
											
											startingPoINT8UAddress = grayed + cropSIZE*i + j;
											*(xcropp2ed + i_times_32_plus_j )= 
													*(startingPoINT8UAddress + 2)
													+ *(startingPoINT8UAddress + cropSIZE_plus_1)
													+ *(startingPoINT8UAddress + cropSIZE_plus_3)
													+ *(startingPoINT8UAddress + (cropSIZE_times_2))
													+ *(startingPoINT8UAddress + cropSIZE_times_2_plus_4)
													+ *(startingPoINT8UAddress + cropSIZE_TIMES_3_plus_1) 
													+ *(startingPoINT8UAddress + cropSIZE_TIMES_3_plus_3) 
													+ *(startingPoINT8UAddress + cropSIZE_times_4_plus_2);

											if(max < *(xcropp2ed + i_times_32_plus_j ))
												{
													max = *(xcropp2ed + i_times_32_plus_j );
													output[0] = i;
													output[1] = j;
													output[2] = max;
												}
											startingPoINT8UAddress++;
											*(xcropp2ed + i_times_32_plus_j + 1 ) = *(startingPoINT8UAddress + 2) + *(startingPoINT8UAddress + cropSIZE_plus_1) + *(startingPoINT8UAddress + cropSIZE_plus_3) + *(startingPoINT8UAddress + (cropSIZE_times_2)) + *(startingPoINT8UAddress + cropSIZE_times_2_plus_4)
													+ *(startingPoINT8UAddress + cropSIZE_TIMES_3_plus_1) + *(startingPoINT8UAddress + cropSIZE_TIMES_3_plus_3) + *(startingPoINT8UAddress + cropSIZE_times_4_plus_2);

											if(max < *(xcropp2ed + i_times_32_plus_j + 1 ))
												{
													max = *(xcropp2ed + i_times_32_plus_j + 1 );
													output[0] = i;
													output[1] = j + 1;
													output[2] = max;
												}
											startingPoINT8UAddress++;
											*(xcropp2ed + i_times_32_plus_j + 2) = *(startingPoINT8UAddress + 2) + *(startingPoINT8UAddress + cropSIZE_plus_1) + *(startingPoINT8UAddress + cropSIZE_plus_3) + *(startingPoINT8UAddress + (cropSIZE_times_2)) + *(startingPoINT8UAddress + cropSIZE_times_2_plus_4)
													+ *(startingPoINT8UAddress + cropSIZE_TIMES_3_plus_1) + *(startingPoINT8UAddress + cropSIZE_TIMES_3_plus_3) + *(startingPoINT8UAddress + cropSIZE_times_4_plus_2);

											if(max < *(xcropp2ed + i_times_32_plus_j + 2))
												{
													max = *(xcropp2ed + i_times_32_plus_j + 2);
													output[0] = i;
													output[1] = j + 2;
													output[2] = max;
												}
											startingPoINT8UAddress++;
											*(xcropp2ed + i_times_32_plus_j + 3) = *(startingPoINT8UAddress + 2) + *(startingPoINT8UAddress + cropSIZE_plus_1) + *(startingPoINT8UAddress + cropSIZE_plus_3) + *(startingPoINT8UAddress + (cropSIZE_times_2)) + *(startingPoINT8UAddress + cropSIZE_times_2_plus_4)
													+ *(startingPoINT8UAddress + cropSIZE_TIMES_3_plus_1) + *(startingPoINT8UAddress + cropSIZE_TIMES_3_plus_3) + *(startingPoINT8UAddress + cropSIZE_times_4_plus_2);

											if(max < *(xcropp2ed + i_times_32_plus_j + 3))
												{
													max = *(xcropp2ed + i_times_32_plus_j + 3);
													output[0] = i;
													output[1] = j + 3;
													output[2] = max;
												}
											startingPoINT8UAddress++;
											*(xcropp2ed + i_times_32_plus_j + 4) = *(startingPoINT8UAddress + 2) + *(startingPoINT8UAddress + cropSIZE_plus_1) + *(startingPoINT8UAddress + cropSIZE_plus_3) + *(startingPoINT8UAddress + (cropSIZE_times_2)) + *(startingPoINT8UAddress + cropSIZE_times_2_plus_4)
													+ *(startingPoINT8UAddress + cropSIZE_TIMES_3_plus_1) + *(startingPoINT8UAddress + cropSIZE_TIMES_3_plus_3) + *(startingPoINT8UAddress + cropSIZE_times_4_plus_2);

											if(max < *(xcropp2ed + i_times_32_plus_j + 4))
												{
													max = *(xcropp2ed + i_times_32_plus_j + 4);
													output[0] = i;
													output[1] = j + 4;
													output[2] = max;
												}
											startingPoINT8UAddress++;
											*(xcropp2ed + i_times_32_plus_j + 5) = *(startingPoINT8UAddress + 2) + *(startingPoINT8UAddress + cropSIZE_plus_1) + *(startingPoINT8UAddress + cropSIZE_plus_3) + *(startingPoINT8UAddress + (cropSIZE_times_2)) + *(startingPoINT8UAddress + cropSIZE_times_2_plus_4)
													+ *(startingPoINT8UAddress + cropSIZE_TIMES_3_plus_1) + *(startingPoINT8UAddress + cropSIZE_TIMES_3_plus_3) + *(startingPoINT8UAddress + cropSIZE_times_4_plus_2);

											if(max < *(xcropp2ed + i_times_32_plus_j + 5))
												{
													max = *(xcropp2ed + i_times_32_plus_j + 5);
													output[0] = i;
													output[1] = j + 5;
													output[2] = max;
												}
											startingPoINT8UAddress++;
											*(xcropp2ed + i_times_32_plus_j + 6) = *(startingPoINT8UAddress + 2) + *(startingPoINT8UAddress + cropSIZE_plus_1) + *(startingPoINT8UAddress + cropSIZE_plus_3) + *(startingPoINT8UAddress + (cropSIZE_times_2)) + *(startingPoINT8UAddress + cropSIZE_times_2_plus_4)
													+ *(startingPoINT8UAddress + cropSIZE_TIMES_3_plus_1) + *(startingPoINT8UAddress + cropSIZE_TIMES_3_plus_3) + *(startingPoINT8UAddress + cropSIZE_times_4_plus_2);

											if(max < *(xcropp2ed + i_times_32_plus_j + 6))
												{
													max = *(xcropp2ed + i_times_32_plus_j + 6);
													output[0] = i;
													output[1] = j + 6;
													output[2] = max;
												}
											
											startingPoINT8UAddress++;
											*(xcropp2ed + i_times_32_plus_j + 7) = *(startingPoINT8UAddress + 2) + *(startingPoINT8UAddress + cropSIZE_plus_1) + *(startingPoINT8UAddress + cropSIZE_plus_3) + *(startingPoINT8UAddress + (cropSIZE_times_2)) + *(startingPoINT8UAddress + cropSIZE_times_2_plus_4)
													+ *(startingPoINT8UAddress + cropSIZE_TIMES_3_plus_1) + *(startingPoINT8UAddress + cropSIZE_TIMES_3_plus_3) + *(startingPoINT8UAddress + cropSIZE_times_4_plus_2);

											if(max < *(xcropp2ed + i_times_32_plus_j + 7))
												{
													max = *(xcropp2ed + i_times_32_plus_j + 7);
													output[0] = i;
													output[1] = j + 7;
													output[2] = max;
												}
										}		
									}
			

			output[0] += 24;
			/*
			 * TODO: calculate position
			 * */
			// alt_putstr("cpu_4 calculating position\n");
			// delay(10);	//simulating position time
			
			
			
			//signal computation completion
			//altera_avalon_mutex_unlock(mutex_4);  
			//delay(1);
			//wait until granted access to write to shared memory
			
			
			
			
			
			/*
			* TODO: write result to shared memory here
			* */
			// alt_putstr("cpu_4 writing result\n");
			//delay(100); 	//simulating write time
//			*shared = 24;
			altera_avalon_mutex_lock(mutex_4, 1);
			writeBackPointer = (INT16U*)(shared + results_offset) + 9;
			*(writeBackPointer) 	= output[0] ; 
			*(writeBackPointer + 1) = output[1] ;
			*(writeBackPointer + 2)	= output[2]; 
			// alt_printf("value: %x\n",output[0]);
			// alt_printf("value: %x\n",output[1]);
			// alt_printf("value: %x\n",output[2]);			
			//signal write completion
			altera_avalon_mutex_unlock(mutex_4);
		// } else {
		// 	alt_putstr("ERROR DETECTED: cpu_4 not supposed to own mutex at begin of loop\n");
		// 	altera_avalon_mutex_unlock(mutex_4);
		// }
		count++;
		// alt_printf("cpu_4 count = %x\n", count);
	}
  	return 0;
}
