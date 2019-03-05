#include <stdio.h>
#include "system.h"
#include "io.h"

#define FLAG1 SHARED_ONCHIP_BASE             //used to indicate if data has been written or not
#define FLAG2 SHARED_ONCHIP_BASE+1           //used to indicate if data has been read or not
#define FLAG3 SHARED_ONCHIP_BASE+2           //used to indicate if data has been read or not
#define LOC_crod SHARED_ONCHIP_BASE+6
\\#define LOC_crodY SHARED_ONCHIP_BASE+10
#define LOC_grayscale  SHARED_ONCHIP_BASE+15           //address where grayscale data is written
#define LOC_crop  SHARED_ONCHIP_BASE+1032           //address where crop data is written

unsigned char dSPAN=15;
unsigned char cropSIZE=31

void CalcCoord(){

	unsigned char* shared = (unsigned char)SHARED_ONCHIP_BASE+15;
	int imgW=(int)shared[0];	  	//pic size
	int imgH=(int)shared[1];
	int Globcoords[2];
	int coordX= shared[LOC_crod];
	int coordY=shared[LOC_crod+1];
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
    shared[LOC_crod]=(unsigned char)Globcoords[0];
	shared[LOC_crod+1]=(unsigned char)Globcoords[1];
	//printf("\nCoords: %d,%d\n", Globcoords[0], Globcoords[1]); OSTimeDly(2000);
}

void crop (int coordX,int coordY){

	unsigned char* shared = (unsigned char)SHARED_ONCHIP_BASE+1032;
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

unsigned int stencilmat(int i, int j,unsigned int cropped_matrix[31][31]){ //x,y,mat
	unsigned int sum;
	sum=(cropped_matrix[j][i+2] + cropped_matrix[j+1][i+1] + cropped_matrix[j+1][i+3] + cropped_matrix[j+2][i] + cropped_matrix[j+2][i+4] + cropped_matrix[j+3][i+1] + cropped_matrix[j+3][i+3] + cropped_matrix[j+4][i+2]);

	return sum;

}
