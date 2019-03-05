#include <stdio.h>
#include "altera_avalon_performance_counter.h"
#include "altera_avalon_pio_regs.h"
#include "sys/alt_irq.h"
#include "sys/alt_alarm.h"
#include "system.h"
#include "io.h"
#include "images.h"

#define DEBUG 0
#define FLAG1 SHARED_ONCHIP_BASE             //used to indicate if data has been read or not
#define FLAG6 SHARED_ONCHIP_BASE+5             //used to indicate if data has been written or not
#define LOC_grayscale  SHARED_ONCHIP_BASE+15           //address where grayscale data is written
#define LOC_toASCII  SHARED_ONCHIP_BASE+1686           //address where toASCII data is written

void grayscale();
void toASCII();
extern void delay (int millisec);

unsigned char *out;         //pointer to PGM output image
unsigned char symbols[16] = {32, 46, 59, 42, 115, 116, 73, 40, 106, 117, 86, 119, 98, 82, 103, 64};
//unsigned char symbols[]={' ', '.', ';', '*', 's', 't', 'I', '(', 'j', 'u', 'V', 'w', 'b', 'R', 'g', '@'};

void grayscale(unsigned char* orig)
{

	int sizeX=*orig++;//orig[0];
	int sizeY=*orig++;//orig[1];
	int maxval=*orig++;
	int fullsize=sizeX*sizeY;
	int i;
	unsigned char* grayscale_pointer = (unsigned char*) LOC_grayscale;
	unsigned char r, g, b;

	*grayscale_pointer++=sizeX;//grayscale_img[0]=sizeX;
	*grayscale_pointer++=sizeY;//grayscale_img[1]=sizeY;

	*grayscale_pointer++=*orig++;//grayscale_img[2]=orig[2];
	for( i=0;i<fullsize;i++)
	{
		r=*orig++;
		g=*orig++;
		b=*orig++;
		*grayscale_pointer++ = (((r<<2)+ r)>>4)+(((g<<3)+ g)>>4)+(b>>3);//orig[3 * i + 3 ] + 0.5625 * orig[3 * i + 4] + 0.125  * orig[3 * i + 5];
	}
}

void toASCII(unsigned char* orig)
{
  int sizeX=*orig++;//orig[0];
	int sizeY=*orig++;
	int i,j;

	*out++ = sizeX ;
	*out++ = sizeY ;
	*out++ = 15 ;

	for( i = 0; i < sizeX * sizeY; i++)
	{
		*out++ = symbols[(*orig++)];
	}
  IOWR_8DIRECT(FLAG6, 0, 0);      //indicate data write
	#if DEBUG == 1
	out-=sizeX*sizeY;
	printf ("ASCII:\n");
	for( i = 0; i < sizeY; i++)
	{
		for( j = 0; j < sizeX; j++)
		{
			printf("%c ",*out++);
		}
		printf("\n");
	}
	#endif
}

int main()
{
  printf("Hello from cpu_0!\n");
  char sequence_length = 4;
  unsigned char* image_sequence[4] = {test_ppm_1, test_ppm_2, test_ppm_3, test_ppm_4};
	char current_image=0;

	PERF_RESET(PERFORMANCE_COUNTER_0_BASE);
	PERF_START_MEASURING (PERFORMANCE_COUNTER_0_BASE);

	int count = 0;
	PERF_BEGIN(PERFORMANCE_COUNTER_0_BASE, 1);
	while(count < 1000)
	{
    if(IORD_8DIRECT(FLAG1, 0)==0);     //wait for data to be read
    {
      grayscale (image_sequence[current_image]);    //transfer image to shared memory
      IOWR_8DIRECT(FLAG1, 0, 1);      //indicate data write
      current_image=(current_image+1) % sequence_length;
		}

    if(IORD_8DIRECT(FLAG6, 0)==1);     //wait for data to be written
    {
      toASCII(LOC_toASCII);
      count++;
			#if DEBUG == 1
			PERF_END(PERFORMANCE_COUNTER_0_BASE, 1);
			/* Print report */
			perf_print_formatted_report(PERFORMANCE_COUNTER_0_BASE,
									 ALT_CPU_FREQ,        // defined in "system.h"
									 1,                   // How many sections to print
									 "Section 1"        // Display-name of section(s).
									 );
			printf("Number of images processed: %d\n",count);
			PERF_BEGIN(PERFORMANCE_COUNTER_0_BASE, 1);
			delay(100);
			#endif

	}

	}
	PERF_END(PERFORMANCE_COUNTER_0_BASE, 1);

	/* Print report */
	perf_print_formatted_report(PERFORMANCE_COUNTER_0_BASE,
							 ALT_CPU_FREQ,        // defined in "system.h"
							 1,                   // How many sections to print
							 "Section 1"        // Display-name of section(s).
							 );
	printf("Number of images processed: %d\n",count);

  return 0;
}
