//#include <stdio.h>
#include "system.h"
//#include "altera_avalon_pio_regs.h"
//#include "io.h"
#include "sys/alt_stdio.h"
#include <altera_avalon_mutex.h>
#include "altera_avalon_performance_counter.h"
#include "images.h"

#include "alt_types.h"
//#define SECTION_WRITEIMAGETOSHAREDMEMORY 	1

//#define SECTION_OTHER_COMPUTING 		4




#define SECTION_DISTRIBUTED_PROCESSING 			1
#define SECTION_WRITE_IMAGE_TO_SHARED_MEMORY	2
#define SECTION_READ_IMAGE_FROM_SHARED_MEMORY	3
#define SECTION_WRITE_RESULT_TO_SHARED_MEMORY	4
#define SECTION_READ_RESULTS					5

typedef alt_u8 INT8U;
typedef alt_u16 INT16U;

#define img_w 64
#define img_h 64
#define img_w_0 192 // =img_w*2

#define dSPAN  15
#define cropSIZE  36
#define xPATTERN_length  5
#define offset_size_length 32
#define cropSIZE_int_RGB 27  // = 36*3/4 

extern void delay (int millisec);
int count = 0;
//int current_image = 0;
unsigned char* shared = (unsigned char*) SHARED_ONCHIP_BASE;

#define results_offset 4000 //results from other CPUs are stored at a place starting from shared+4000
#define DEBUG 1

#ifndef DEBUG
#include <stdio.h>
#endif
#include "ascii_gray.h"

int main()
{
	// alt_putstr("Hello cpu_0!\n");

	alt_mutex_dev* mutex_0 = altera_avalon_mutex_open( "/dev/mutex_0" );
	alt_mutex_dev* mutex_1 = altera_avalon_mutex_open( "/dev/mutex_1" );
	alt_mutex_dev* mutex_2 = altera_avalon_mutex_open( "/dev/mutex_2" );
	alt_mutex_dev* mutex_3 = altera_avalon_mutex_open( "/dev/mutex_3" );
	alt_mutex_dev* mutex_4 = altera_avalon_mutex_open( "/dev/mutex_4" );
	
	
	/*
	 * Handshaking
	 * */
	
	// alt_putstr("cpu_0 is waiting for all CPUs to wake up\n");
	
	char cpu1_sleeping = 1;
	char cpu2_sleeping = 1;
	char cpu3_sleeping = 1;
	char cpu4_sleeping = 1;
	
	
	altera_avalon_mutex_trylock(mutex_1, 1);
	altera_avalon_mutex_trylock(mutex_2, 1);
	altera_avalon_mutex_trylock(mutex_3, 1);
	altera_avalon_mutex_trylock(mutex_4, 1);
	
	while(cpu1_sleeping || cpu2_sleeping || cpu3_sleeping || cpu4_sleeping) {
		if(cpu1_sleeping) {
			altera_avalon_mutex_trylock(mutex_1, 1);
			cpu1_sleeping = altera_avalon_mutex_is_mine(mutex_1);
			altera_avalon_mutex_unlock(mutex_1);
			if(!cpu1_sleeping) {//just woke up
				alt_putstr("CPU 1 awake\n");
				altera_avalon_mutex_lock(mutex_1, 1);
			}
		}
		if(cpu2_sleeping) {
			altera_avalon_mutex_trylock(mutex_2, 1);
			cpu2_sleeping = altera_avalon_mutex_is_mine(mutex_2);
			altera_avalon_mutex_unlock(mutex_2);
			if(!cpu2_sleeping) {//just woke up
				alt_putstr("CPU 2 awake\n");
				altera_avalon_mutex_lock(mutex_2, 1);
			}
		}
		if(cpu3_sleeping) {
			altera_avalon_mutex_trylock(mutex_3, 1);
			cpu3_sleeping = altera_avalon_mutex_is_mine(mutex_3);
			altera_avalon_mutex_unlock(mutex_3);
			if(!cpu3_sleeping) {//just woke up
				alt_putstr("CPU 3 awake\n");
				altera_avalon_mutex_lock(mutex_3, 1);
			}
		}
		if(cpu4_sleeping) {
			altera_avalon_mutex_trylock(mutex_4, 1);
			cpu4_sleeping = altera_avalon_mutex_is_mine(mutex_4);
			altera_avalon_mutex_unlock(mutex_4);
			if(!cpu4_sleeping) {//just woke up
				alt_putstr("CPU 4 awake\n");
				altera_avalon_mutex_lock(mutex_4, 1);
			}
		}
	}
	
	// alt_putstr("All CPUs are awake\n");
	/*
	 * Handshake finished
	 * */
	
	
	
	
	
	
	INT8U *image;
	INT16U detected[3];
	INT8U coords[2];
	int *writePointer;
	INT16U *readPointer;
	INT16U valueFromCPUs[4];
	INT16U detectedCoords[2];

	detected[0] = 0;
	detected[1] = 0;	
	
	INT8U* startingPoINT8UAddress;
	int* staringPointAddress_forRow;
	INT8U i;
	INT8U j;
		
	PERF_RESET(PERFORMANCE_COUNTER_0_BASE);
	PERF_START_MEASURING (PERFORMANCE_COUNTER_0_BASE);
	while (1) {

		if(count % 4 == 0)
		{
			//alt_putstr("all the four images has been processed!\n");

			detected[0] = 0;
			detected[1] = 0;
			detected[2] = 0;
			if( DEBUG && count == 4) {
				altera_avalon_mutex_lock(mutex_1, 1);
				altera_avalon_mutex_lock(mutex_2, 1);
				altera_avalon_mutex_lock(mutex_3, 1);
				altera_avalon_mutex_lock(mutex_4, 1);
				exit(0);
			} else
		 	if( count == 100)
		 	{
			while(1)
			{
				altera_avalon_mutex_lock(mutex_1, 1);
				altera_avalon_mutex_lock(mutex_2, 1);
				altera_avalon_mutex_lock(mutex_3, 1);
				altera_avalon_mutex_lock(mutex_4, 1);
			    PERF_STOP_MEASURING (PERFORMANCE_COUNTER_0_BASE);

			    perf_print_formatted_report
					(PERFORMANCE_COUNTER_0_BASE,
					ALT_CPU_FREQ,        // defined in "system.h"
					5,                   // How many sections to print
					"Slave CPU Workload",
					"Write Image",
					"Read Image",
					"Write Result",
					"Read Results"
					);
				exit(0);
			}		 		
		 	}

		}

		image = image_sequence[count % 4] + 4;

		
		
		
		/*
		 * calculating the cropping point, cropping function and transmit the image to shared onship memory
		 * */

		PERF_BEGIN(PERFORMANCE_COUNTER_0_BASE, SECTION_WRITE_IMAGE_TO_SHARED_MEMORY);
		if(detected[0] <= dSPAN)
			coords[0] = 0;
		else if (detected[0] > (img_h - dSPAN))
			coords[0] = img_h - cropSIZE;
		else
			coords[0] = detected[0] - dSPAN ;
		if(detected[1] <= dSPAN)
			coords[1] = 0;
		else if (detected[1] > (img_w - dSPAN))
			coords[1] = img_w - cropSIZE;
		else
			coords[1] = detected[1] - dSPAN ;
				
		startingPoINT8UAddress = image + img_w_0 * coords[0] + coords[1]*3; 
		writePointer = (int*)shared - cropSIZE_int_RGB;
		for ( i = 0; i < cropSIZE; i++)
		{
			staringPointAddress_forRow =(int*)(startingPoINT8UAddress + img_w_0*i);
			writePointer += cropSIZE_int_RGB;
			for( j = 0; j < cropSIZE_int_RGB; j = j + 9)
			{
				*(writePointer+j)     = *(staringPointAddress_forRow+j);
				*(writePointer+j + 1) = *(staringPointAddress_forRow+j + 1); 
				*(writePointer+j + 2) = *(staringPointAddress_forRow+j + 2); 
				*(writePointer+j + 3) = *(staringPointAddress_forRow+j + 3); 
				*(writePointer+j + 4) = *(staringPointAddress_forRow+j + 4); 
				*(writePointer+j + 5) = *(staringPointAddress_forRow+j + 5); 
				*(writePointer+j + 6) = *(staringPointAddress_forRow+j + 6); 
				*(writePointer+j + 7) = *(staringPointAddress_forRow+j + 7); 
				*(writePointer+j + 8) = *(staringPointAddress_forRow+j + 8);  
			}		
		}
		PERF_END(PERFORMANCE_COUNTER_0_BASE, SECTION_WRITE_IMAGE_TO_SHARED_MEMORY);
		
		
		
		
		
		/*
		 * schedule reading from memory
		 * */
		PERF_BEGIN(PERFORMANCE_COUNTER_0_BASE, SECTION_READ_IMAGE_FROM_SHARED_MEMORY);
		// alt_putstr("cpu_0 scheduling reading\n");
		if(altera_avalon_mutex_is_mine(mutex_1)) {
			// alt_putstr("cpu_0 giving control to cpu 1\n");
			altera_avalon_mutex_unlock(mutex_1);
			delay(1);
			altera_avalon_mutex_lock(mutex_1, 1);

		} else {
			alt_putstr("cpu_0 cant open mutex 1 because does not own mutex 1\n");
		}
		if(altera_avalon_mutex_is_mine(mutex_2)) {
			// alt_putstr("cpu_0 giving control to cpu 2\n");
			altera_avalon_mutex_unlock(mutex_2);
			delay(1);
			altera_avalon_mutex_lock(mutex_2, 1);
		} else {
			alt_putstr("cpu_0 cant open mutex 2 because does not own mutex 2\n");
		}
		if(altera_avalon_mutex_is_mine(mutex_3)) {
			// alt_putstr("cpu_0 giving control to cpu 3\n");
			altera_avalon_mutex_unlock(mutex_3);
			delay(1);
			altera_avalon_mutex_lock(mutex_3, 1);
		} else {
			alt_putstr("cpu_0 cant open mutex 3 because does not own mutex 3\n");
		}
		if(altera_avalon_mutex_is_mine(mutex_4)) {
			// alt_putstr("cpu_0 giving control to cpu 4\n");
			altera_avalon_mutex_unlock(mutex_4);
			delay(1);
			altera_avalon_mutex_lock(mutex_4, 1);
		} else {
			alt_putstr("cpu_0 cant open mutex 4 because does not own mutex 4\n");
		}
		PERF_END(PERFORMANCE_COUNTER_0_BASE, SECTION_READ_IMAGE_FROM_SHARED_MEMORY);
		
		
		
		
		
		PERF_BEGIN(PERFORMANCE_COUNTER_0_BASE, SECTION_WRITE_RESULT_TO_SHARED_MEMORY);
		/*
		 * schedule writing result to memory
		 * */
		// alt_putstr("cpu_0 scheduling result writing from other cpu's\n");
		
		if(altera_avalon_mutex_is_mine(mutex_1)) {
			// alt_putstr("cpu_0 giving control to cpu 1\n");
			altera_avalon_mutex_unlock(mutex_1);
			delay(1);
			altera_avalon_mutex_lock(mutex_1, 1);

		} else {
			alt_putstr("cpu_0 cant open mutex 1 because does not own mutex 1\n");
		}

		if(altera_avalon_mutex_is_mine(mutex_2)) {
			// alt_putstr("cpu_0 giving control to cpu 2\n");
			altera_avalon_mutex_unlock(mutex_2);
			delay(1);
			altera_avalon_mutex_lock(mutex_2, 1);
		} else {
			alt_putstr("cpu_0 cant open mutex 2 because does not own mutex 2\n");
		}
		
		if(altera_avalon_mutex_is_mine(mutex_3)) {
			// alt_putstr("cpu_0 giving control to cpu 3\n");
			altera_avalon_mutex_unlock(mutex_3);
			delay(1);
			altera_avalon_mutex_lock(mutex_3, 1);
		} else {
			alt_putstr("cpu_0 cant open mutex 3 because does not own mutex 3\n");
		}
		
		if(altera_avalon_mutex_is_mine(mutex_4)) {
			// alt_putstr("cpu_0 giving control to cpu 4\n");
			altera_avalon_mutex_unlock(mutex_4);
			delay(1);
			altera_avalon_mutex_lock(mutex_4, 1);
		} else {
			alt_putstr("cpu_0 cant open mutex 4 because does not own mutex 4\n");
		}
		PERF_END(PERFORMANCE_COUNTER_0_BASE, SECTION_WRITE_RESULT_TO_SHARED_MEMORY);

		// alt_putstr("results written\n");
		
		/*
		 * Read results 
		 * */
		
		// alt_putstr("cpu_0 reading results from memory and compare them in order to get the final result\n");
		PERF_BEGIN(PERFORMANCE_COUNTER_0_BASE, SECTION_READ_RESULTS);
		// results are stored at a consecutive memory space starting from shared + 4000, the order is x1,y1,value1, x2, y2, value2 ... 
		// Datatype is INT16U
		readPointer = (INT16U*)(shared + results_offset);

		valueFromCPUs[0] = *(readPointer + 2);

		detected[2]		 = valueFromCPUs[0];
		detected[0]		 = *(readPointer );
		detected[1]		 = *(readPointer + 1);
		//alt_printf("cpu_0 reading results from CPU1: %x \n",detected[0]);
		// alt_printf("cpu_0 reading results from CPU1: %x \n",detected[1]);
		// alt_printf("cpu_0 reading results from CPU1: %x \n",detected[2]);
		valueFromCPUs[1] = *(readPointer + 5);
		if(detected[2] < valueFromCPUs[1])
			{
				detected[2]		 = valueFromCPUs[1];
				detected[0]		 = *(readPointer + 3);
				detected[1]		 = *(readPointer + 4);
			}
		
		valueFromCPUs[2] = *(readPointer + 8);
		if(detected[2] < valueFromCPUs[2])
			{
				detected[2]		 = valueFromCPUs[2];
				detected[0]		 = *(readPointer + 6);
				detected[1]		 = *(readPointer + 7);
			}
		
		valueFromCPUs[3] = *(readPointer + 11);
		if(detected[2] < valueFromCPUs[3])
			{
				detected[2]		 = valueFromCPUs[3];
				detected[0]		 = *(readPointer + 9);
				detected[1]		 = *(readPointer + 10);
			}

		//calculating the positions by adding the offset to the coords

		detected[0] = detected[0] + coords[0] + 2;
		detected[1] = detected[1] + coords[1] + 2;

		PERF_END(PERFORMANCE_COUNTER_0_BASE, SECTION_READ_RESULTS);
		alt_printf("Count:	Final result: %x: %x, %x, %x\n", count, (count % 4),detected[0],detected[1],detected[2]);
		

		count ++;
		// alt_printf("image number %x processed\n", count);
		
		if(DEBUG) {
			printAscii(image, image_sequence[count % 4][0], image_sequence[count % 4][1]);
			alt_printf("------------------------------------\n\n");
			printAsciiHidden(image, image_sequence[count % 4][0], image_sequence[count % 4][1], detected[0], detected[1], 5, 0);
		}
		
//			    	printf("------------------------------------\n\n");
//			    	printAsciiHidden(grayed[0], image_sequence[counter][0], image_sequence[counter][1],
//			    			detected[0], detected[1], 5, 0);
//			    }
		
		

	    
	    //delay(500);	//nice wait
  }
  return 0;
}