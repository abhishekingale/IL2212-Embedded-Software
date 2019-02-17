#include <stdio.h>
#include "altera_avalon_performance_counter.h"
#include "includes.h"
#include "altera_avalon_pio_regs.h"
#include "sys/alt_irq.h"
#include "sys/alt_alarm.h"
#include "system.h"
#include "io.h"
#include "images.h"
void grayF();
void cropF();

void toASCII();

#define DEBUG 1

#define HW_TIMER_PERIOD 100 /* 100ms */

/* Definition of Task Stacks */
#define   TASK_STACKSIZE       2048
OS_STK    task1_stk[TASK_STACKSIZE];
OS_STK    StartTask_Stack[TASK_STACKSIZE]; 

/* Definition of Task Priorities */

#define STARTTASK_PRIO      1
#define TASK1_PRIORITY      10

/* Definition of Task Periods (ms) */
#define TASK1_PERIOD 10000

#define grayF1 1
#define cropF2 2

#define toASCII5 5

/*
 * Example function for copying a ppm image from sram to the shared on-chip mempry
 */
void sram2sm_p3(unsigned char* base)
{
	int x, y;
	unsigned char* shared;

	shared = (unsigned char*) SHARED_ONCHIP_BASE;

	int size_x = *base++;
	int size_y = *base++;
	int max_col= *base++;
	*shared++  = size_x;
	*shared++  = size_y;
	*shared++  = max_col;
	printf("The image is: %d x %d!! \n", size_x, size_y);
	for(y = 0; y < size_y; y++)
	for(x = 0; x < size_x; x++)
	{
		*shared++ = *base++; 	// R
		*shared++ = *base++;	// G
		*shared++ = *base++;	// B
	}
}

void grayF(unsigned char* orig)
{	
	PERF_BEGIN(PERFORMANCE_COUNTER_0_BASE, grayF1);

	int sizeX=orig[0];
	int sizeY=orig[1];
	int fullsize=sizeX*sizeY;
	int i,j;
	unsigned char grayscale_img[fullsize+3];

	grayscale_img[0]=sizeX;
	grayscale_img[1]=sizeY;
	grayscale_img[2]=orig[2];
	for( i=0;i<fullsize;i++)
	{
		grayscale_img[i + 3]= 0.3125 * orig[3 * i + 3 ] + 0.5625 * orig[3 * i + 4] + 0.125  * orig[3 * i + 5];
	}
	PERF_END(PERFORMANCE_COUNTER_0_BASE, grayscale1);  
	cropF (grayscale_img);

}

void cropF(unsigned char* orig)
{	
	PERF_BEGIN(PERFORMANCE_COUNTER_0_BASE, resize2);

	int sizeX=orig[0];
	int sizeY=orig[1];
	int fullsize=sizeX*sizeY;
	int i,j;
	unsigned char resize_img[fullsize/4+3];

	resize_img[0]=sizeX/2;
	resize_img[1]=sizeY/2;
	resize_img[2]=orig[2];
	
	for ( i=0;i<sizeY;i=i+2)
	{
		for ( j=0;j<sizeX;j=j+2)
		{
		resize_img[j/2+i*sizeX/4+3]=(orig[i*sizeX+j+3]+orig[i*sizeX+j+4]+orig[(i+1)*sizeX+j+3]+orig[(i+1)*sizeX+j+4])/4;
		}
	}
	PERF_END(PERFORMANCE_COUNTER_0_BASE, resize2);  
//brightness(resize_img);
}

void toASCII(unsigned char* orig)
{
PERF_BEGIN(PERFORMANCE_COUNTER_0_BASE, toASCII5);

int sizeX = orig[0], sizeY = orig[1];
int i,j;
unsigned char ASCII[sizeX * sizeY+3];
unsigned char symbols[16] = {32, 46, 59, 42, 115, 116, 73, 40, 106, 117, 86, 119, 98, 82, 103, 64};

ASCII[0] = sizeX ;
ASCII[1] = sizeY ;
ASCII[2] = orig[2] ;
	printf ("ASCII:\n");

for( i = 0; i < sizeX * sizeY; i++)
{
ASCII[i+3] = symbols[((orig[i+3])/16)];
}
for( i = 0; i < sizeY; i++)
	{
	for( j=0; j< sizeX;j++)
		{
		printf("%c ",ASCII[j+(i*sizeX)+3]);
		}
	printf("\n");
	}
	PERF_END(PERFORMANCE_COUNTER_0_BASE, toASCII5);  
}





int main(void) {

  printf("MicroC/OS-II-Vesion: %1.2f\n", (double) OSVersion()/100.0);
  	char number_of_images=10;
	unsigned char* img_array[10] = {img1_24_24, img1_32_22, img1_32_32, img1_40_28, img1_40_40, 
			img2_24_24, img2_32_22, img2_32_32, img2_40_28, img2_40_40 };
	char current_image=0;

	while (1)
	{ 	
		/* Extract the x and y dimensions of the picture */
		unsigned char i = *img_array[current_image];
		unsigned char j = *(img_array[current_image]+1);

		PERF_RESET(PERFORMANCE_COUNTER_0_BASE);
		PERF_START_MEASURING (PERFORMANCE_COUNTER_0_BASE);

		/* Measurement here */
	grayscale (img_array[current_image]);


		/* Print report */
		perf_print_formatted_report
		(PERFORMANCE_COUNTER_0_BASE,            
		ALT_CPU_FREQ,        // defined in "system.h"
		5,                   // How many sections to print
		"grayscale",        // Display-name of section(s).
  	    "resize", 
   		"brightness", 
  	  	"sobel", 
    	"toASCII" 
		);   

		/* Increment the image pointer */
		current_image=(current_image+1) % number_of_images;

	}

  
  return 0;
}
