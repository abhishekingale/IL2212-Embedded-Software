// Skeleton for lab 2
//
// Task 1 writes periodically RGB-images to the shared memory
//
// No guarantees provided - if bugs are detected, report them in the Issue tracker of the github repository!

#include <stdio.h>
#include "altera_avalon_performance_counter.h"
#include "includes.h"
#include "altera_avalon_pio_regs.h"
#include "sys/alt_irq.h"
#include "sys/alt_alarm.h"
#include "system.h"
#include "io.h"
#include "images.h"
#include "ascii_gray"

void grayscale();
void crop();
void xcorr2();


#define DEBUG 1

#define HW_TIMER_PERIOD 100 /* 100ms */

/* Definition of Task Stacks */
#define   TASK_STACKSIZE       2048
OS_STK    task1_stk[TASK_STACKSIZE];
OS_STK    task2_stk[TASK_STACKSIZE];
OS_STK    task3_stk[TASK_STACKSIZE];

OS_STK    StartTask_Stack[TASK_STACKSIZE];

/* Definition of Task Priorities */

#define STARTTASK_PRIO      1
#define TASK1_PRIORITY      10
#define TASK2_PRIORITY      11
#define TASK3_PRIORITY      12


/* Definition of Task Periods (ms) */
#define TASK1_PERIOD 10000

#define grayscale1 1
#define getImage2 2
#define crop3 3
#define xcorr24 4
#define resetArray5 5


#define dSPAN 15
#define cropSIZE ((2*dSPAN)+1) //the size of the image after cropping

unsigned char grayed[64][64]; //output matrix of the grayscale function
double cropResult[cropSIZE][cropSIZE]; //output matrix of the crop function
double xcorr2Result[cropSIZE-4][cropSIZE-4]; //output matrix of the xcorr2 function

coordinate init; //coordinate representing the initial token

coordinate detect; //coordinate representing the output of the objectPos function

typedef struct { //structure to map to coordinates
  int x, y;
} coordinate;

coordinate init; //coordinate representing the initial token

coordinate detect; //coordinate representing the output of the objectPos function


void grayscale(unsigned char* orig)
{
	PERF_BEGIN(PERFORMANCE_COUNTER_0_BASE, grayscale1);

	int sizeX=orig[0];
	int sizeY=orig[1];
	int fullsize=sizeX*sizeY;
	int i,j;
	unsigned char grayscale_img[fullsize+3];
	unsigned char* imgP;
	unsigned char* share;

	grayscale_img[0]=sizeX;
	grayscale_img[1]=sizeY;
	grayscale_img[2]=orig[2];
	for(i=0;i<fullsize;i++)
	{
		grayscale_img[i + 3]= 0.3125 * orig[3 * i + 3 ] + 0.5625 * orig[3 * i + 4] + 0.125  * orig[3 * i + 5];
	}

	 imgP = grayscale_img;
	share = (unsigned char*) SHARED_ONCHIP_BASE;
	for (i=0;i<fullsize+3;i++)
	{
		*share++ = *imgP++;
	}
	PERF_END(PERFORMANCE_COUNTER_0_BASE, grayscale1);
//	resize (grayscale_img);

}

int getRowsMatrix(){ //returns the number of rows in the gray matrix
  return sizeof(grayed)/sizeof(grayed[0]);
}

int getColumnsMatrix(){ //returns the number of column in the gray matrix
  return sizeof(grayed[0])/sizeof(grayed[0][0]);
}

	void crop(coordinate c){
{
	PERF_BEGIN(PERFORMANCE_COUNTER_0_BASE, resize2);

	//int sizeX=orig[0];
//	int sizeY=orig[1];
	int fullsize=(c.y + cropSIZE)*(c.x + cropSIZE);
	//int i,j;
//	int s=1;
	//unsigned char resize_img[fullsize/4+3];

	unsigned double* imgP;
	unsigned char* share;

 //equivalent to crop
  int i, j;
   PERF_BEGIN(PERFORMANCE_COUNTER_0_BASE, crop3);
  for(i = c.y; i < c.y + cropSIZE; i++) { //cycle through rows
    for(j = c.x; j < c.x + cropSIZE; j++) { //cycle through columns
    cropResult[i-c.y][j-c.x] = (double) grayed[i][j]; //populate new cropped matrix of size cropSIZE by cropSIZE
    }
  }
	 	imgP = cropResult;
	share = (unsigned char*) SHARED_ONCHIP_BASE;
	for (i=0;i<fullsize+3;i++)
	{
		*share++ = *imgP++;
	}

	PERF_END(PERFORMANCE_COUNTER_0_BASE, resize2);
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

void xcorr2 ()
{
	unsigned double* imgP;
	unsigned char* share;

	int i, j;
 PERF_BEGIN(PERFORMANCE_COUNTER_0_BASE, xcorr24);
  for(i = 0; i < cropSIZE-4; i++) {
    for(j = 0; j < cropSIZE-4; j++) {
          xcorr2Result[i][j] = cropResult[i][j+2] + cropResult[i+1][j+1] +cropResult[i+1][j+3] + cropResult[i+2][j] + cropResult[i+2][j+4] +
		  cropResult[i+3][j+1] +cropResult[i+3][j+3] +cropResult[i+4][j+2]; //calculate the cross correllation
    }
  }
 PERF_END(PERFORMANCE_COUNTER_0_BASE, xcorr24);
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




int delay; // Delay of HW-timer

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
OS_EVENT *Task2Sem;
OS_EVENT *Task3Sem;

// SW-Timer
OS_TMR *Task1Tmr;

/* Timer Callback Functions */
void Task1TmrCallback (void *ptmr, void *callback_arg){
  OSSemPost(Task1TmrSem);
}
void task1(void* pdata)
{
	INT8U err;
	INT8U value=0;
	char current_image=0;

	#if DEBUG == 1
	/* Sequence of images for testing if the system functions properly */
	char number_of_images=4;
	unsigned char* img_array[4] = {test_ppm_1, test_ppm_2, test_ppm_3, test_ppm_4 };
	#else
	/* Sequence of images for measuring performance */
	char number_of_images=4;
	unsigned char* img_array[4] = {test_ppm_1, test_ppm_2, test_ppm_3, test_ppm_4};
	#endif

	while (1)
	{
		/* Extract the x and y dimensions of the picture */
		unsigned char i = *img_array[current_image];
		unsigned char j = *(img_array[current_image]+1);
		OSSemPend(Task1Sem, 0, &err);

		PERF_RESET(PERFORMANCE_COUNTER_0_BASE);
		PERF_START_MEASURING (PERFORMANCE_COUNTER_0_BASE);
//		PERF_BEGIN(PERFORMANCE_COUNTER_0_BASE, SECTION_1);

		/* Measurement here */
   	grayscale (img_array[current_image]);

		IOWR_ALTERA_AVALON_PIO_DATA(LEDS_GREEN_BASE,value++);

		OSSemPost(Task2Sem);


		/* Increment the image pointer */
		current_image=(current_image+1) % number_of_images;

	}
}
void task2(void* pdata) {
	INT8U err;
	unsigned char* share;
	share = (unsigned char*) SHARED_ONCHIP_BASE;
	while(1)
	{
	OSSemPend(Task2Sem, 0, &err);
//	PERF_BEGIN(PERFORMANCE_COUNTER_0_BASE, resizeImg2);
		/* Measurement here */
	crop(share);
//	PERF_END(PERFORMANCE_COUNTER_0_BASE, resizeImg2);
	// printf("hello from task2\n");

	OSSemPost(Task3Sem);
	}
}
void task3(void* pdata) {
	INT8U err;
	unsigned char* share;
	share = (unsigned char*) SHARED_ONCHIP_BASE;
	while(1)
	{

	OSSemPend(Task3Sem, 0, &err);
//	PERF_BEGIN(PERFORMANCE_COUNTER_0_BASE, brightCorrect3);
		/* Measurement here */
	xcorr2(share);
//	PERF_END(PERFORMANCE_COUNTER_0_BASE, brightCorrect3);

	// printf("hello from task3\n");

	OSSemPost(Task1Sem);
	}
}


void StartTask(void* pdata)
{
  INT8U err;
  void* context;

  static alt_alarm alarm;     /* Is needed for timer ISR function */

  /* Base resolution for SW timer : HW_TIMER_PERIOD ms */
  delay = alt_ticks_per_second() * HW_TIMER_PERIOD / 1000;
  printf("delay in ticks %d\n", delay);


  if (alt_alarm_start (&alarm,
      delay,
      alarm_handler,
      context) < 0)
      {
          printf("No system clock available!n");
      }


   Task1Tmr = OSTmrCreate(0, //delay
                            TASK1_PERIOD/HW_TIMER_PERIOD, //period
                            OS_TMR_OPT_PERIODIC,
                            Task1TmrCallback, //OS_TMR_CALLBACK
                            (void *)0,
                            "Task1Tmr",
                            &err);

   if (DEBUG) {
    if (err == OS_ERR_NONE) { //if creation successful
      printf("Task1Tmr created\n");
    }
   }


   /*
    * Start timers
    */

   //start Task1 Timer
   OSTmrStart(Task1Tmr, &err);

   if (DEBUG) {
    if (err == OS_ERR_NONE) { //if start successful
      printf("Task1Tmr started\n");
    }
   }


   /*
   * Creation of Kernel Objects
   */

  Task1TmrSem = OSSemCreate(1);
  Task1Sem = OSSemCreate(1);
  Task2Sem = OSSemCreate(0);
  Task3Sem = OSSemCreate(0);

  /*
   * Create statistics task
   */

  OSStatInit();

  /*
   * Creating Tasks in the system
   */

  err=OSTaskCreateExt(task1,
                  NULL,
                  (void *)&task1_stk[TASK_STACKSIZE-1],
                  TASK1_PRIORITY,
                  TASK1_PRIORITY,
                  task1_stk,
                  TASK_STACKSIZE,
                  NULL,
                  0);

  if (DEBUG) {
     if (err == OS_ERR_NONE) { //if start successful
      printf("Task1 created\n");
    }
   }
   err=OSTaskCreateExt(task2,
                  NULL,
                  (void *)&task2_stk[TASK_STACKSIZE-1],
                  TASK2_PRIORITY,
                  TASK2_PRIORITY,
                  task2_stk,
                  TASK_STACKSIZE,
                  NULL,
                  0);

  if (DEBUG) {
     if (err == OS_ERR_NONE) { //if start successful
      printf("Task2 created\n");
    }
   }
   err=OSTaskCreateExt(task3,
                  NULL,
                  (void *)&task3_stk[TASK_STACKSIZE-1],
                  TASK3_PRIORITY,
                  TASK3_PRIORITY,
                  task3_stk,
                  TASK_STACKSIZE,
                  NULL,
                  0);

  if (DEBUG) {
     if (err == OS_ERR_NONE) { //if start successful
      printf("Task3 created\n");
    }
   }

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
