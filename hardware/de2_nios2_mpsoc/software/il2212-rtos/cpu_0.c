// Skeleton for lab 2
//
// Task 1 writes periodically RGB-images to the shared memory
//
// No guarantees provided - if bugs are detected, report them in the Issue tracker of the github repository!

#include <stdio.h>
#include "altera_avalon_performance_counter.h"
#include "includes.h"
#include "ascii_gray.h"
#include "altera_avalon_pio_regs.h"
#include "sys/alt_irq.h"
#include "sys/alt_alarm.h"
#include "system.h"
#include "io.h"
#include "images.h"
#define PERFORMANCE 1
#define DEBUG 0
#define HW_TIMER_PERIOD 100 /* 100ms */

/* Definition of Task Stacks */
#define   TASK_STACKSIZE       2048
OS_STK    StartTask_Stack[TASK_STACKSIZE];
OS_STK    task1_stk[TASK_STACKSIZE];
OS_STK    GrayTask_stk[TASK_STACKSIZE];
OS_STK    CalcCoordTask_stk[TASK_STACKSIZE];
OS_STK    CropTask_stk[TASK_STACKSIZE];
OS_STK    xCorr2Task_stk[TASK_STACKSIZE];
OS_STK    PosMaxTask_stk[TASK_STACKSIZE];
OS_STK    ObjectPosTask_stk[TASK_STACKSIZE];

/* Definition of Task Priorities */

#define STARTTASK_PRIO	1
#define TASK1_PRIORITY	9
#define GrayTask_PRIORITY	10
#define CalcCoordTask_PRIORITY	11
#define CropTask_PRIORITY	12
#define xCorr2Task_PRIORITY	13
#define PosMaxTask_PRIORITY	14
#define ObjectPosTask_PRIORITY	15

/* Definition of Task Periods (ms) */

#define TASK1_PERIOD 10000
#define SECTION_1 1
#define SECTION_2 2
#define FinalResultLocation 64*64+3
#define ObjectLocation 64*64+3+2

/* Definition of dimensions gave by the project */




/*
 * Global variables
 */
unsigned char dSPAN=15;
unsigned char cropSIZE=(2*15)+1;
int delay;		// Delay of HW-timer
int current_image=0;
int counter=0;
INT8U err;

/*
 * ISR for HW Timer
 */
alt_u32 alarm_handler(void* context)
{
  OSTmrSignal(); /* Signals a 'tick' to the SW timers */

  return delay;
}

// Semaphores
OS_EVENT *Task1TmrSem;
OS_EVENT *Task1Sem;
OS_EVENT *GrayedSem;
OS_EVENT *CalcCoordSem;
OS_EVENT *CropSem;
OS_EVENT *xCorr2Sem;
OS_EVENT *PosMaxSem;
OS_EVENT *objectPosSem;

/*
Every task at the beginning block itself thanks to the utilization of semaphores
and once that the function has finished they increase the value of the semaphore
of the next task.
*/
void task1(void* pdata)
{
	INT8U err;
	INT8U value=0;
	char current_image=0;

	#if DEBUG == 1
	/* Sequence of images for testing if the system functions properly */
	char sequence_length = 4;
	unsigned char* image_sequence[4] = {test_ppm_1, test_ppm_2, test_ppm_3, test_ppm_4};

	#else
	/* Sequence of images for measuring performance */
	char sequence_length=3;
	unsigned char* image_sequence[3] = {test_ppm_1, test_ppm_2, test_ppm_3};
	#endif

	while (1)
	{
		/* Extract the x and y dimensions of the picture */
		unsigned char i = *image_sequence[current_image];
		unsigned char j = *(image_sequence[current_image]+1);
		OSSemPend(Task1Sem, 0, &err);

		PERF_RESET(PERFORMANCE_COUNTER_0_BASE);
		PERF_START_MEASURING (PERFORMANCE_COUNTER_0_BASE);
//		PERF_BEGIN(PERFORMANCE_COUNTER_0_BASE, SECTION_1);

		/* Measurement here */
   	Grayscale(image_sequence[current_image]);

		//IOWR_ALTERA_AVALON_PIO_DATA(LEDS_GREEN_BASE,value++);

		OSSemPost(GrayedSem);


		/* Increment the image pointer */
		current_image=(current_image+1) % sequence_length;

	}
}
void Grayscale(unsigned char* img){
	int maxX=img[0];
	int maxY=img[1];
	int total= maxY*maxX*3;
	int moving_index=3;
	int basic_index=0;
	unsigned char* shared = (unsigned char)SHARED_ONCHIP_BASE;
	*shared++=img[0];
	*shared++=img[1];
	*shared++=img[2];

	while(moving_index<total){

		shared[basic_index]=(unsigned char)(((img[moving_index])>>2)+((img[moving_index])>>4)+((img[moving_index+1])>>1)+((img[moving_index+1])>>4)+((img[moving_index+2])>>3));
		basic_index++;
		moving_index+=3;

	}

	if(DEBUG) {printf("\n INPUT \n");printAscii(shared, maxX,maxY); OSTimeDly(3000);}
}


void GrayTask(void* pdata){

		PERF_RESET(PERFORMANCE_COUNTER_0_BASE);
		PERF_START_MEASURING (PERFORMANCE_COUNTER_0_BASE);
		PERF_BEGIN(PERFORMANCE_COUNTER_0_BASE, SECTION_1);

		if(PERFORMANCE){
			while(counter++<100){
					if(counter==0) PERF_BEGIN(PERFORMANCE_COUNTER_0_BASE, SECTION_2);
					Grayscale(image_sequence[current_image]);
					OSSemPost(CalcCoordSem);
					OSSemPend(GrayedSem,0,&err);
				}
			PERF_END(PERFORMANCE_COUNTER_0_BASE, SECTION_1);

			/* Print report */
			perf_print_formatted_report
			(PERFORMANCE_COUNTER_0_BASE,
			ALT_CPU_FREQ,        // defined in "system.h"
			2,                   // How many sections to print
			"Total time", "Time per iteration"        // Display-name of section(s).
			);

			double total_time =  (double)perf_get_section_time(PERFORMANCE_COUNTER_0_BASE, SECTION_2)/(double)alt_get_cpu_freq();
			    double normalized_time = total_time/4;
			    double throughput = 1/normalized_time;
			    printf("execution time: %f\nnormalized time: %f\nthroughput: %f\n", total_time, normalized_time, throughput);
			//printf("\nThroughtput=%d img/s", counter/perf_get_section_time(PERFORMANCE_COUNTER_0_BASE, SECTION_1) );
		}
		else{
				while(current_image<sequence_length){
					Grayscale(image_sequence[current_image]);
					OSSemPost(CalcCoordSem);
					OSSemPend(GrayedSem,0,&err);
				}
		}
}


void CalcCoord(){

	unsigned char* shared = (unsigned char)SHARED_ONCHIP_BASE;
	int imgW=(int)shared[0];	  	//pic size
	int imgH=(int)shared[1];
	int Globcoords[2];
	int coordX= shared[FinalResultLocation];
	int coordY=shared[FinalResultLocation+1];
	//equation used by the program below
	//printf("imgW=%d, imgH=%d, xvalue=%d, yvalue=%d\n",imgW,imgH,coordX,coordY);
	//OSTimeDly(2000);
	if(coordX <= dSPAN ){
		Globcoords[0] = 0;
	}
	else if ( coordX > imgW-dSPAN){
			Globcoords[0] =(imgW-cropSIZE-1);
		}
	else Globcoords[0] =(coordX-dSPAN-1);



	if( coordY <= dSPAN ){
		Globcoords[1] = 0;
	}
	else if ( coordY > imgH-dSPAN){
			Globcoords[1] =(imgH - cropSIZE -1);
		}
	else {
		Globcoords[1]=(coordY - dSPAN - 1);
	}
    shared[FinalResultLocation]=(unsigned char)Globcoords[0];
	shared[FinalResultLocation+1]=(unsigned char)Globcoords[1];
	//printf("\nCoords: %d,%d\n", Globcoords[0], Globcoords[1]); OSTimeDly(2000);
}

void CalcCoordTask(void *pdata){


	OSSemPend(CalcCoordSem,0,&err);
	unsigned char* shared = (unsigned char)SHARED_ONCHIP_BASE;
	if((current_image)==0){
		shared[FinalResultLocation]=dSPAN;
		shared[FinalResultLocation+1]=dSPAN;
	}
	while(1){
		CalcCoord();
		OSSemPost(CropSem);
		OSSemPend(CalcCoordSem,0,&err);
	}
}

void crop (int coordX,int coordY){

	unsigned char* shared = (unsigned char)SHARED_ONCHIP_BASE;
	int head;		//head to use as reference
	int jump;
	head=(shared[0]*coordY)+coordX+3;
		//Goes directly to the coords given (+3 because the first three values are x,y and maxvalue)
		//defines how many values we need to skip once that the cropsize is over
		//Jump defines the gap between values to skip to have a perfect cropping
	jump=shared[0]-cropSIZE;
	//printf("head: %d\njump:%d\n",head,jump); OSTimeDly(1000);
	int i,j;
	int following=0;
	for(i=0;i<cropSIZE;i++){
		for(j=0;j<cropSIZE;j++){
			shared[following]=(unsigned char)shared[head];
			//printf("%d,", shared[head]);
			head++; following++;
		}
		//printf("\n");
		head+=jump;		//when we reach the border of the matrix we goes to the next line
	}
	//OSTimeDly(5000);
}

void CropTask(void *pdata){

	OSSemPend(CropSem,0,&err);
	unsigned char* shared = (unsigned char)SHARED_ONCHIP_BASE;
	while(1){

		crop(shared[FinalResultLocation],shared[FinalResultLocation+1]);
		OSSemPost(xCorr2Sem);
		OSSemPend(CropSem,0,&err);
	}
}


unsigned int stencilmat(int i, int j,unsigned int cropped_matrix[31][31]){ //x,y,mat
	unsigned int sum;
	sum=(cropped_matrix[j][i+2] + cropped_matrix[j+1][i+1] + cropped_matrix[j+1][i+3] + cropped_matrix[j+2][i] + cropped_matrix[j+2][i+4] + cropped_matrix[j+3][i+1] + cropped_matrix[j+3][i+3] + cropped_matrix[j+4][i+2]);

	return sum;

}

void xCorr2(){

	int i,j;
	unsigned int matrix[31][31];
	unsigned char* shared = (unsigned char)SHARED_ONCHIP_BASE;
	for(i=0;i<cropSIZE;i++){
		for(j=0;j<cropSIZE;j++){
			matrix[i][j]=(unsigned int)*shared;
			shared++;

			}

	}
 	unsigned int* intshared = (unsigned int)SHARED_ONCHIP_BASE;
	for(i=0; i<cropSIZE-4;i++){
		for(j=0; j<cropSIZE-4;j++){
			*intshared++= stencilmat(j,i,matrix);

		}

	}

}


void xCorr2Task(void *pdata){

		OSSemPend(xCorr2Sem,0,&err);
		while(1){
			xCorr2();
			OSSemPost(PosMaxSem);
			OSSemPend(xCorr2Sem,0,&err);
		}

}

void posMax(){
	unsigned int max=0;		//Variable used for the max
	unsigned int maybemax;		//Variable that is a max of it's own matrix but not the biggest

	//The following finds the highest value between all the different matrix
	int i,j;
	unsigned int* shared = (unsigned int)SHARED_ONCHIP_BASE;
	int matrix[27][27];
	for(i=0;i<cropSIZE-4;i++){
		for(j=0;j<cropSIZE-4;j++){
			matrix[i][j]=*shared++;
			//shared++;
		}
	}
	unsigned char* charshared = (unsigned char)SHARED_ONCHIP_BASE;
	for(i=0; i<cropSIZE-4;i++){
		for(j=0; j<cropSIZE-4;j++){
			maybemax=matrix[i][j];

			if(maybemax> max){
				 max=maybemax;
				 charshared[ObjectLocation]=j;
				 charshared[ObjectLocation+1]=i;
			}
		}

	}
	if(DEBUG){printf("\nCROPPED OUTPUT\n");printAsciiHidden(charshared,cropSIZE-4,cropSIZE-4,charshared[ObjectLocation],charshared[ObjectLocation+1],3,max); OSTimeDly(3000);}
}


void PosMaxTask(void *pdata){
	OSSemPend(PosMaxSem,0,&err);
	while(1){
	posMax();
	OSSemPost(objectPosSem);
	OSSemPend(PosMaxSem,0,&err);
	}
}

void objectPos(){
	unsigned char* shared = (unsigned char)SHARED_ONCHIP_BASE;
	//Equation given by the haskell code
	shared[FinalResultLocation]= shared[FinalResultLocation]+shared[ObjectLocation]+2;
	shared[FinalResultLocation+1]= shared[FinalResultLocation+1]+shared[ObjectLocation+1]+2;
	if(!PERFORMANCE) printf("\n[%d,%d]\n", shared[FinalResultLocation], shared[FinalResultLocation+1]);

}

void ObjectPosTask(void *pdata){

	OSSemPend(objectPosSem,0,&err);
	while(1){
		objectPos();
		current_image++;
		if(PERFORMANCE){
			 current_image%=sequence_length;
			 if(counter==0)PERF_END(PERFORMANCE_COUNTER_0_BASE, SECTION_2);
		}
		else{
			PERF_END(PERFORMANCE_COUNTER_0_BASE, SECTION_1);

			/* Print report */
			perf_print_formatted_report
			(PERFORMANCE_COUNTER_0_BASE,
			ALT_CPU_FREQ,        // defined in "system.h"
			1,                   // How many sections to print
			"Section 1"        // Display-name of section(s).
			);
		}
		OSSemPost(GrayedSem);
		OSSemPend(objectPosSem,0,&err);
	}
}


void StartTask(void* pdata)
{

  void* context=0;

  static alt_alarm alarm;     /* Is needed for timer ISR function */

  /* Base resolution for SW timer : HW_TIMER_PERIOD ms */
  delay = alt_ticks_per_second() * HW_TIMER_PERIOD / 1000;
  printf("delay in ticks %d\n", delay);

  /*
   * Create Hardware Timer with a period of 'delay'
   */
  if (alt_alarm_start (&alarm,
      delay,
      alarm_handler,
      context) < 0)
      {
          printf("No system clock available!n");
      }

   /*
   * Creation of Kernel Objects
   */
  Task1Sem = OSSemCreate(1);
  GrayedSem= OSSemCreate(0);
  CalcCoordSem= OSSemCreate(0);
  CropSem= OSSemCreate(0);
  xCorr2Sem = OSSemCreate(0);
  PosMaxSem = OSSemCreate(0);
  objectPosSem = OSSemCreate(0);
  /*
   * Create statistics task
   */

  OSStatInit();

  /*
   * Creating Tasks in the system
   */


	err=OSTaskCreateExt(CalcCoordTask,
                  NULL,
                  (void *)&CalcCoordTask_stk[TASK_STACKSIZE-1],
                  CalcCoordTask_PRIORITY,
                  CalcCoordTask_PRIORITY,
                  CalcCoordTask_stk,
                  TASK_STACKSIZE,
                  NULL,
                  0);

	 if (DEBUG) {
     if (err == OS_ERR_NONE) { //if start successful
      printf("CalcCoordTask created\n");
    }}

	err=OSTaskCreateExt(xCorr2Task,
                  NULL,
                  (void *)&xCorr2Task_stk[TASK_STACKSIZE-1],
                  xCorr2Task_PRIORITY,
                  xCorr2Task_PRIORITY,
                  xCorr2Task_stk,
                  TASK_STACKSIZE,
                  NULL,
                  0);

	 if (DEBUG) {
     if (err == OS_ERR_NONE) { //if start successful
      printf("xCorr2Task created\n");
    }}

	err=OSTaskCreateExt(CropTask,
                  NULL,
                  (void *)&CropTask_stk[TASK_STACKSIZE-1],
                  CropTask_PRIORITY,
                  CropTask_PRIORITY,
                  CropTask_stk,
                  TASK_STACKSIZE,
                  NULL,
                  0);

	 if (DEBUG) {
		 if (err == OS_ERR_NONE) { //if start successful
		  printf("CropTask created\n");
   		 }
	}

	err=OSTaskCreateExt(PosMaxTask,
                  NULL,
                  (void *)&PosMaxTask_stk[TASK_STACKSIZE-1],
                  PosMaxTask_PRIORITY,
                  PosMaxTask_PRIORITY,
                  PosMaxTask_stk,
                  TASK_STACKSIZE,
                  NULL,
                  0);

	 if (DEBUG) {
     if (err == OS_ERR_NONE) { //if start successful
      printf("PosMaxTask created\n");
    }else printf("\n%d\n",err);}


	err=OSTaskCreateExt(GrayTask,
                  NULL,
                  (void *)&GrayTask_stk[TASK_STACKSIZE-1],
                  GrayTask_PRIORITY,
                  GrayTask_PRIORITY,
                  GrayTask_stk,
                  TASK_STACKSIZE,
                  NULL,
                  0);

	 if (DEBUG) {
     if (err == OS_ERR_NONE) { //if start successful
      printf("GrayTask created\n");
    }}
	err=OSTaskCreateExt(ObjectPosTask,
                  NULL,
                  (void *)&ObjectPosTask_stk[TASK_STACKSIZE-1],
                  ObjectPosTask_PRIORITY,
                  ObjectPosTask_PRIORITY,
                  ObjectPosTask_stk,
                  TASK_STACKSIZE,
                  NULL,
                  0);

	 if (DEBUG) {
     if (err == OS_ERR_NONE) { //if start successful
      printf("ObjectPosTask created\n");
    }}

  printf("All Tasks and Kernel Objects generated!\n");

  /* Task deletes itself */

  OSTaskDel(OS_PRIO_SELF);
}


int main(void) {

  printf("MicroC/OS-II-Vesion: %1.2f\n", (double) OSVersion()/100.0);

  OSTaskCreateExt(
	 StartTask, // Pointer to task code
         NULL,      // Pointer to argument that is
                    // passed to task
         (void *)&StartTask_Stack[TASK_STACKSIZE-1], // Pointer to top
						     // of task stack
         STARTTASK_PRIO,
         STARTTASK_PRIO,
         (void *)&StartTask_Stack[0],
         TASK_STACKSIZE,
         (void *) 0,
         OS_TASK_OPT_STK_CHK | OS_TASK_OPT_STK_CLR);

  OSStart();

  return 0;
}
