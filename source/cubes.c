#include "globals.h"
#include "models.h"
#include "cubes.h"

u8 cnt1=0; // Terrible name, frame counter to manage the rotation of the camera in the FIRST part of the animation when the small cube hits the big translucent cube

// Small rolling cube
f32 rot[17]={0.0f};		// Rotation angle for each stage
f32 size=52.906584f;	// Needed for further translations/rotations
f32 posx=-83.0f;		// Position
f32 posy=-403.0f;
f32 posz=75.613168f;
f32 fall=41.5f;			// Initial fall distance is 166, divided by 4
f32 ssX=2.900002f;		// These 7 values are used for texture projection
f32 nn=0.031800f;
f32 oo=0.011370f;
f32 pp=-0.242000f;
f32 qq=0.739994f;
f32 ffovy=13.390054f;
f32 aspect=0.740000f;
f32 delta=1.0f;			// To scale the cube as it moves up in the final stage, final value at 1.6f
u8 count=0;				// Little counter to count 3 frames before starting to move the cube up in the final stage
u8 freeze=8;			// Amount of freezed frames before the last fall of the cube
f32 cntr=0;				// Counter for texture projection, float type to allow flexibility to handle 50 Hz correctly

//Big translucent cube
f32 posxB=-1.0f;	// Position
f32 posyB=-62.9f;
f32 poszB=-1.0f;
f32 rotxB=0.0f;		// Rotation
f32 rotyB=90.0f;
f32 rotzB=0.0f;
f32 sX=6.799998f;
f32 n=-0.006560f;
f32 o=0.011090f;
f32 p=0.188000f;
f32 q=-0.289730f;
f32 fovy=4.590037f;
Mtx mr,mv,rx,ry,rz,m;

// RGB color for both cubes (white, texture will be colored)
u8 cr=255;
u8 cg=255;
u8 cb=255;

void prepsCubeView(f32 speed, f32 fspeed, u8* stage, f32 tv1, f32 tv2, f32* alpha, f32 alphaT[]){
	GRRLIB_ObjectViewBegin();
	switch(*stage){
		case 0:
			posy+=26.5f*tv1;
			if(posy>=-195.0f){
				(*stage)++;
				posy=-195.0f;
			}
			GRRLIB_ObjectViewTrans(posx,posy,posz);
			break;
		case 1:
			*alpha=255.0f;
			switch (cnt1){
				case 0:
					lookX-=0.03f;
					camY-=7.0f;
					ty-=7.0f;
					fovy+=0.292f;
					break;
				case 1:
					lookX-=0.03f;
					camY-=4.0f;
					ty-=4.0f;
					fovy+=0.167f;
					break;
				case 2:
					lookX-=0.03f;
					camY-=4.0f;
					ty-=4.0f;
					fovy+=0.167f;
					break;
				case 3:
					lookX-=0.02f;
					camY-=3.0f;
					ty-=3.0f;
					fovy+=0.125f;
					(*alpha)-=10.0f;
					break;
				case 4:
					lookX-=0.02f;
					camY-=2.0f;
					ty-=2.0f;
					fovy+=0.083f;
					(*alpha)-=15.0f;
					break;
				case 5:
					lookX-=0.01f;
					camY-=1.0f;
					ty-=1.0f;
					fovy+=0.042f;
					(*alpha)-=20.0f;
					break;
				case 6:
					lookX-=0.01f;
					camY-=1.0f;
					ty-=1.0f;
					fovy+=0.042f;
					(*alpha)-=25.0f;
					break;
				case 7:
					lookX-=0.005f;
					camY-=1.0f;
					ty-=1.0f;
					fovy+=0.042f;
					(*alpha)-=30.0f;
					break;
				case 8:
					lookX-=0.005f;
					camY-=1.0f;
					ty-=1.0f;
					fovy+=0.042f;
					(*alpha)-=35.0f;
					break;
				case 9:
					(*alpha)-=40.0f;
					break;
				case 10:
					lookX+=0.005f;
					camY+=1.0f;
					ty+=1.0f;
					fovy-=0.042f;
					(*alpha)-=50.0f;
					break;
				case 11:
					lookX+=0.005f;
					camY+=1.0f;
					ty+=1.0f;
					fovy-=0.042f;
					(*alpha)-=60.0f;
					break;
				case 12:
					lookX+=0.005f;
					camY+=1.0f;
					ty+=1.0f;
					fovy-=0.042f;
					(*alpha)-=70.0f;
					break;
				case 13:
					lookX+=0.005f;
					camY+=1.0f;
					ty+=1.0f;
					fovy-=0.042f;
					(*alpha)-=80.0f;
					break;
				case 14:
					lookX+=0.01f;
					camY+=1.0f;
					ty+=1.0f;
					fovy-=0.042f;
					(*alpha)-=90.0f;
					break;
				case 15:
					lookX+=0.01f;
					camY+=2.0f;
					ty+=2.0f;
					fovy-=0.083f;
					(*alpha)-=100.0f;
					break;
				case 16:
					lookX+=0.01f;
					camY+=3.0f;
					ty+=3.0f;
					fovy-=0.125f;
					(*alpha)-=110.0f;
					break;
				case 17:
					lookX+=0.01f;
					camY+=2.0f;
					ty+=2.0f;
					fovy-=0.083f;
					(*alpha)-=120.0f;
					(*stage)++;
					break;
			}
			cnt1++;
			GRRLIB_ObjectViewTrans(posx,posy,posz);
			break;
		case 2:
			rot[0]+=speed;
			if(rot[0]>90.0f){
				(*stage)++;
				rot[0]=90.0f;
			}
			lookX+=0.008f*tv1;
			camY+=1.0f*tv1;
			ty+=1.0f*tv1;
			fovy-=0.042f*tv1;
			(*alpha)-=10.0f*tv1;
			if(cntr<10) cntr+=tv1;
			GRRLIB_ObjectViewTrans(0,-size,size);
			GRRLIB_ObjectViewRotate(rot[0],0,0);
			GRRLIB_ObjectViewTrans(0,size,-size);
			GRRLIB_ObjectViewTrans(posx,posy,posz);
			break;
		case 3:
			rot[1]+=speed;
			if(rot[1]>90.0f){
				(*stage)++;
				rot[1]=90.0f;
			}
			lookX+=0.003f*tv1;
			camY+=0.4f*tv1;
			ty+=0.4f*tv1;
			fovy-=0.017f*tv1;
			(*alpha)-=25.0f/11.0f*tv1;
			if(alphaT[0]<254.0f) alphaT[0]+=255.0f*speed/(90.0f*tv2);
			if(cntr<20) cntr+=tv1;
			GRRLIB_ObjectViewTrans(0,-size,size);
			GRRLIB_ObjectViewRotate(rot[0],0,0);
			GRRLIB_ObjectViewTrans(0,0,size);
			GRRLIB_ObjectViewRotate(rot[1],0,0);
			GRRLIB_ObjectViewTrans(0,size,-2*size);
			GRRLIB_ObjectViewTrans(posx,posy,posz);
			break;
		case 4:
			rot[2]+=speed;
			if(rot[2]>90.0f){
				(*stage)++;
				rot[2]=90.0f;
			}
			if(*alpha>0.0f) (*alpha)-=1.0f;
			if(rot[2]>=18.0f){
				lookX-=0.001f*tv1;
				camY-=0.2f*tv1;
				ty-=0.2f*tv1;
				fovy+=0.008f*tv1;
			}
			if(alphaT[1]<254.0f) alphaT[1]+=255.0f*speed/(90.0f*tv2);
			if(cntr<30) cntr+=tv1;
			GRRLIB_ObjectViewTrans(0,-size,size);
			GRRLIB_ObjectViewRotate(rot[0],0,0);
			GRRLIB_ObjectViewTrans(0,0,size);
			GRRLIB_ObjectViewRotate(rot[1],0,0);
			GRRLIB_ObjectViewTrans(-size,0,size);
			GRRLIB_ObjectViewRotate(0,0,rot[2]);
			GRRLIB_ObjectViewTrans(size,size,-3*size);
			GRRLIB_ObjectViewTrans(posx,posy,posz);
			break;
		case 5:
			rot[3]+=speed;
			if(rot[3]>90.0f){
				(*stage)++;
				rot[3]=90.0f;
			}
			if(lookX>0) lookX-=0.001f*tv1;
			if(camY>-199.8f){
				camY-=0.2f*tv1;
				ty-=0.2f*tv1;
			}
			else{
				lookX=0.0f;
				camY=-200.0f;
				ty=0.0f;
			}
			if(cntr<40) cntr+=tv1;
			if(alphaT[2]<254.0f) alphaT[2]+=255.0f*speed/(90.0f*tv2);
			GRRLIB_ObjectViewTrans(0,-size,size);
			GRRLIB_ObjectViewRotate(rot[0],0,0);
			GRRLIB_ObjectViewTrans(0,0,size);
			GRRLIB_ObjectViewRotate(rot[1],0,0);
			GRRLIB_ObjectViewTrans(-size,0,size);
			GRRLIB_ObjectViewRotate(0,0,rot[2]);
			GRRLIB_ObjectViewTrans(-size,0,0);
			GRRLIB_ObjectViewRotate(0,0,rot[3]);
			GRRLIB_ObjectViewTrans(2*size,size,-3*size);
			GRRLIB_ObjectViewTrans(posx,posy,posz);
			break;
		case 6:
			rot[4]+=speed*1.25f; // We go sligtly faster where the angle is bigger
			if(rot[4]>180.0f){
				(*stage)++;
				rot[4]=180.0f;
			}
			if(cntr<56) cntr+=tv1;
			if(alphaT[3]<254.0f) alphaT[3]+=255.0f*speed/(90.0f*tv2);
			GRRLIB_ObjectViewTrans(0,-size,size);
			GRRLIB_ObjectViewRotate(rot[0],0,0);
			GRRLIB_ObjectViewTrans(0,0,size);
			GRRLIB_ObjectViewRotate(rot[1],0,0);
			GRRLIB_ObjectViewTrans(-size,0,size);
			GRRLIB_ObjectViewRotate(0,0,rot[2]);
			GRRLIB_ObjectViewTrans(-size,0,0);
			GRRLIB_ObjectViewRotate(0,0,rot[3]);
			GRRLIB_ObjectViewTrans(-size,0,0);
			GRRLIB_ObjectViewRotate(0,0,rot[4]);
			GRRLIB_ObjectViewTrans(3*size,size,-3*size);
			GRRLIB_ObjectViewTrans(posx,posy,posz);
			break;
		case 7:
			rot[5]+=speed;
			if(rot[5]>90.0f){
				(*stage)++;
				rot[5]=90.0f;
			}
			if(cntr<66) cntr+=tv1;
			if(alphaT[4]<254.0f) alphaT[4]+=255.0f*speed/(90.0f*tv2);
			GRRLIB_ObjectViewTrans(0,-size,size);
			GRRLIB_ObjectViewRotate(rot[0],0,0);
			GRRLIB_ObjectViewTrans(0,0,size);
			GRRLIB_ObjectViewRotate(rot[1],0,0);
			GRRLIB_ObjectViewTrans(-size,0,size);
			GRRLIB_ObjectViewRotate(0,0,rot[2]);
			GRRLIB_ObjectViewTrans(-size,0,0);
			GRRLIB_ObjectViewRotate(0,0,rot[3]);
			GRRLIB_ObjectViewTrans(-size,0,0);
			GRRLIB_ObjectViewRotate(0,0,rot[4]);
			GRRLIB_ObjectViewTrans(0,-size,0);
			GRRLIB_ObjectViewRotate(0,0,rot[5]);
			GRRLIB_ObjectViewTrans(3*size,2*size,-3*size);
			GRRLIB_ObjectViewTrans(posx,posy,posz);
			break;
		case 8:
			rot[6]+=speed;
			if(rot[6]>90.0f){
				(*stage)++;
				rot[6]=90.0f;
			}
			if(cntr<76) cntr+=tv1;
			if(alphaT[5]<254.0f) alphaT[5]+=255.0f*speed/(90.0f*tv2);
			GRRLIB_ObjectViewTrans(0,-size,size);
			GRRLIB_ObjectViewRotate(rot[0],0,0);
			GRRLIB_ObjectViewTrans(0,0,size);
			GRRLIB_ObjectViewRotate(rot[1],0,0);
			GRRLIB_ObjectViewTrans(-size,0,size);
			GRRLIB_ObjectViewRotate(0,0,rot[2]);
			GRRLIB_ObjectViewTrans(-size,0,0);
			GRRLIB_ObjectViewRotate(0,0,rot[3]);
			GRRLIB_ObjectViewTrans(-size,0,0);
			GRRLIB_ObjectViewRotate(0,0,rot[4]);
			GRRLIB_ObjectViewTrans(0,-size,0);
			GRRLIB_ObjectViewRotate(0,0,rot[5]);
			GRRLIB_ObjectViewTrans(0,-size,0);
			GRRLIB_ObjectViewRotate(0,0,rot[6]);			
			GRRLIB_ObjectViewTrans(3*size,3*size,-3*size);
			GRRLIB_ObjectViewTrans(posx,posy,posz);
			break;
		case 9:
			rot[7]+=speed;
			if(rot[7]>90.0f){
				(*stage)++;
				rot[7]=90.0f;
			}
			if(cntr<86.0f) cntr+=tv1;
			if(rot[7]>=43.0f && rot[7]<54.0f) aspect=0.53f;
			if(rot[7]==54.0f) aspect=-0.849999f;
			if(rot[7]>=63.0f) aspect=0.74f;
			if(alphaT[6]<254.0f) alphaT[6]+=255.0f*speed/(90.0f*tv2);
			GRRLIB_ObjectViewTrans(0,-size,size);
			GRRLIB_ObjectViewRotate(rot[0],0,0);
			GRRLIB_ObjectViewTrans(0,0,size);
			GRRLIB_ObjectViewRotate(rot[1],0,0);
			GRRLIB_ObjectViewTrans(-size,0,size);
			GRRLIB_ObjectViewRotate(0,0,rot[2]);
			GRRLIB_ObjectViewTrans(-size,0,0);
			GRRLIB_ObjectViewRotate(0,0,rot[3]);
			GRRLIB_ObjectViewTrans(-size,0,0);
			GRRLIB_ObjectViewRotate(0,0,rot[4]);
			GRRLIB_ObjectViewTrans(0,-size,0);
			GRRLIB_ObjectViewRotate(0,0,rot[5]);
			GRRLIB_ObjectViewTrans(0,-size,0);
			GRRLIB_ObjectViewRotate(0,0,rot[6]);
			GRRLIB_ObjectViewTrans(0,-size,-size);
			GRRLIB_ObjectViewRotate(0,-rot[7],0);			
			GRRLIB_ObjectViewTrans(3*size,4*size,-2*size);
			GRRLIB_ObjectViewTrans(posx,posy,posz);
			break;
		case 10:
			rot[8]+=speed;
			if(rot[8]>90.0f){
				(*stage)++;
				rot[8]=90.0f;
			}
			if(cntr<96) cntr+=tv1;
			if(rot[8]==54.0f) aspect=-0.849999f;
			if(rot[8]>54.0f) aspect=0.74f;
			if(alphaT[7]<254.0f) alphaT[7]+=255.0f*speed/(90.0f*tv2);
			GRRLIB_ObjectViewTrans(0,-size,size);
			GRRLIB_ObjectViewRotate(rot[0],0,0);
			GRRLIB_ObjectViewTrans(0,0,size);
			GRRLIB_ObjectViewRotate(rot[1],0,0);
			GRRLIB_ObjectViewTrans(-size,0,size);
			GRRLIB_ObjectViewRotate(0,0,rot[2]);
			GRRLIB_ObjectViewTrans(-size,0,0);
			GRRLIB_ObjectViewRotate(0,0,rot[3]);
			GRRLIB_ObjectViewTrans(-size,0,0);
			GRRLIB_ObjectViewRotate(0,0,rot[4]);
			GRRLIB_ObjectViewTrans(0,-size,0);
			GRRLIB_ObjectViewRotate(0,0,rot[5]);
			GRRLIB_ObjectViewTrans(0,-size,0);
			GRRLIB_ObjectViewRotate(0,0,rot[6]);
			GRRLIB_ObjectViewTrans(0,-size,-size);
			GRRLIB_ObjectViewRotate(0,-rot[7],0);	
			GRRLIB_ObjectViewTrans(0,0,-size);
			GRRLIB_ObjectViewRotate(0,-rot[8],0);		
			GRRLIB_ObjectViewTrans(3*size,4*size,-size);
			GRRLIB_ObjectViewTrans(posx,posy,posz);
			break;
		case 11:
			rot[9]+=speed*1.25f;
			if(rot[9]>180.0f){
				(*stage)++;
				rot[9]=180.0f;
			}
			if(cntr<112) cntr+=tv1;
			if(alphaT[8]<254.0f) alphaT[8]+=255.0f*speed/(90.0f*tv2);
			GRRLIB_ObjectViewTrans(0,-size,size);
			GRRLIB_ObjectViewRotate(rot[0],0,0);
			GRRLIB_ObjectViewTrans(0,0,size);
			GRRLIB_ObjectViewRotate(rot[1],0,0);
			GRRLIB_ObjectViewTrans(-size,0,size);
			GRRLIB_ObjectViewRotate(0,0,rot[2]);
			GRRLIB_ObjectViewTrans(-size,0,0);
			GRRLIB_ObjectViewRotate(0,0,rot[3]);
			GRRLIB_ObjectViewTrans(-size,0,0);
			GRRLIB_ObjectViewRotate(0,0,rot[4]);
			GRRLIB_ObjectViewTrans(0,-size,0);
			GRRLIB_ObjectViewRotate(0,0,rot[5]);
			GRRLIB_ObjectViewTrans(0,-size,0);
			GRRLIB_ObjectViewRotate(0,0,rot[6]);
			GRRLIB_ObjectViewTrans(0,-size,-size);
			GRRLIB_ObjectViewRotate(0,-rot[7],0);	
			GRRLIB_ObjectViewTrans(0,0,-size);
			GRRLIB_ObjectViewRotate(0,-rot[8],0);
			GRRLIB_ObjectViewTrans(0,0,-size);
			GRRLIB_ObjectViewRotate(0,-rot[9],0);	
			GRRLIB_ObjectViewTrans(3*size,4*size,0);
			GRRLIB_ObjectViewTrans(posx,posy,posz);
			break;
		case 12:
			rot[10]+=speed;
			if(rot[10]>90.0f){
				(*stage)++;
				rot[10]=90.0f;
			}
			if(cntr<122) cntr+=tv1;
			if(alphaT[9]<254.0f) alphaT[9]+=255.0f*speed/(90.0f*tv2);
			GRRLIB_ObjectViewTrans(0,-size,size);
			GRRLIB_ObjectViewRotate(rot[0],0,0);
			GRRLIB_ObjectViewTrans(0,0,size);
			GRRLIB_ObjectViewRotate(rot[1],0,0);
			GRRLIB_ObjectViewTrans(-size,0,size);
			GRRLIB_ObjectViewRotate(0,0,rot[2]);
			GRRLIB_ObjectViewTrans(-size,0,0);
			GRRLIB_ObjectViewRotate(0,0,rot[3]);
			GRRLIB_ObjectViewTrans(-size,0,0);
			GRRLIB_ObjectViewRotate(0,0,rot[4]);
			GRRLIB_ObjectViewTrans(0,-size,0);
			GRRLIB_ObjectViewRotate(0,0,rot[5]);
			GRRLIB_ObjectViewTrans(0,-size,0);
			GRRLIB_ObjectViewRotate(0,0,rot[6]);
			GRRLIB_ObjectViewTrans(0,-size,-size);
			GRRLIB_ObjectViewRotate(0,-rot[7],0);	
			GRRLIB_ObjectViewTrans(0,0,-size);
			GRRLIB_ObjectViewRotate(0,-rot[8],0);
			GRRLIB_ObjectViewTrans(0,0,-size);
			GRRLIB_ObjectViewRotate(0,-rot[9],0);
			GRRLIB_ObjectViewTrans(size,0,0);
			GRRLIB_ObjectViewRotate(0,-rot[10],0);
			GRRLIB_ObjectViewTrans(2*size,4*size,0);
			GRRLIB_ObjectViewTrans(posx,posy,posz);
			break;
		case 13:
			rot[11]+=speed;
			if(rot[11]>90.0f){
				(*stage)++;
				rot[11]=90.0f;
			}
			if(cntr<132) cntr+=tv1;
			if(alphaT[10]<254.0f) alphaT[10]+=255.0f*speed/(90.0f*tv2);
			GRRLIB_ObjectViewTrans(0,-size,size);
			GRRLIB_ObjectViewRotate(rot[0],0,0);
			GRRLIB_ObjectViewTrans(0,0,size);
			GRRLIB_ObjectViewRotate(rot[1],0,0);
			GRRLIB_ObjectViewTrans(-size,0,size);
			GRRLIB_ObjectViewRotate(0,0,rot[2]);
			GRRLIB_ObjectViewTrans(-size,0,0);
			GRRLIB_ObjectViewRotate(0,0,rot[3]);
			GRRLIB_ObjectViewTrans(-size,0,0);
			GRRLIB_ObjectViewRotate(0,0,rot[4]);
			GRRLIB_ObjectViewTrans(0,-size,0);
			GRRLIB_ObjectViewRotate(0,0,rot[5]);
			GRRLIB_ObjectViewTrans(0,-size,0);
			GRRLIB_ObjectViewRotate(0,0,rot[6]);
			GRRLIB_ObjectViewTrans(0,-size,-size);
			GRRLIB_ObjectViewRotate(0,-rot[7],0);	
			GRRLIB_ObjectViewTrans(0,0,-size);
			GRRLIB_ObjectViewRotate(0,-rot[8],0);
			GRRLIB_ObjectViewTrans(0,0,-size);
			GRRLIB_ObjectViewRotate(0,-rot[9],0);
			GRRLIB_ObjectViewTrans(size,0,0);
			GRRLIB_ObjectViewRotate(0,-rot[10],0);
			GRRLIB_ObjectViewTrans(size,0,0);
			GRRLIB_ObjectViewRotate(0,-rot[11],0);
			GRRLIB_ObjectViewTrans(size,4*size,0);
			GRRLIB_ObjectViewTrans(posx,posy,posz);
			break;
		case 14:
			rot[12]+=speed;
			if(rot[12]>90.0f){
				(*stage)++;
				rot[12]=90.0f;
			}
			if(cntr<142) cntr+=tv1;
			if(alphaT[11]<254.0f) alphaT[11]+=255.0f*speed/(90.0f*tv2);
			GRRLIB_ObjectViewTrans(0,-size,size);
			GRRLIB_ObjectViewRotate(rot[0],0,0);
			GRRLIB_ObjectViewTrans(0,0,size);
			GRRLIB_ObjectViewRotate(rot[1],0,0);
			GRRLIB_ObjectViewTrans(-size,0,size);
			GRRLIB_ObjectViewRotate(0,0,rot[2]);
			GRRLIB_ObjectViewTrans(-size,0,0);
			GRRLIB_ObjectViewRotate(0,0,rot[3]);
			GRRLIB_ObjectViewTrans(-size,0,0);
			GRRLIB_ObjectViewRotate(0,0,rot[4]);
			GRRLIB_ObjectViewTrans(0,-size,0);
			GRRLIB_ObjectViewRotate(0,0,rot[5]);
			GRRLIB_ObjectViewTrans(0,-size,0);
			GRRLIB_ObjectViewRotate(0,0,rot[6]);
			GRRLIB_ObjectViewTrans(0,-size,-size);
			GRRLIB_ObjectViewRotate(0,-rot[7],0);	
			GRRLIB_ObjectViewTrans(0,0,-size);
			GRRLIB_ObjectViewRotate(0,-rot[8],0);
			GRRLIB_ObjectViewTrans(0,0,-size);
			GRRLIB_ObjectViewRotate(0,-rot[9],0);
			GRRLIB_ObjectViewTrans(size,0,0);
			GRRLIB_ObjectViewRotate(0,-rot[10],0);
			GRRLIB_ObjectViewTrans(size,0,0);
			GRRLIB_ObjectViewRotate(0,-rot[11],0);
			GRRLIB_ObjectViewTrans(-size,size,0);
			GRRLIB_ObjectViewRotate(rot[12],0,0);
			GRRLIB_ObjectViewTrans(2*size,3*size,0);
			GRRLIB_ObjectViewTrans(posx,posy,posz);
			break;
		case 15:
			rot[13]+=speed;
			if(rot[13]>90.0f){
				(*stage)++;
				rot[13]=90.0f;
			}
			if(cntr<152) cntr+=tv1;
			if(alphaT[12]<254.0f) alphaT[12]+=255.0f*speed/(90.0f*tv2);
			GRRLIB_ObjectViewTrans(0,-size,size);
			GRRLIB_ObjectViewRotate(rot[0],0,0);
			GRRLIB_ObjectViewTrans(0,0,size);
			GRRLIB_ObjectViewRotate(rot[1],0,0);
			GRRLIB_ObjectViewTrans(-size,0,size);
			GRRLIB_ObjectViewRotate(0,0,rot[2]);
			GRRLIB_ObjectViewTrans(-size,0,0);
			GRRLIB_ObjectViewRotate(0,0,rot[3]);
			GRRLIB_ObjectViewTrans(-size,0,0);
			GRRLIB_ObjectViewRotate(0,0,rot[4]);
			GRRLIB_ObjectViewTrans(0,-size,0);
			GRRLIB_ObjectViewRotate(0,0,rot[5]);
			GRRLIB_ObjectViewTrans(0,-size,0);
			GRRLIB_ObjectViewRotate(0,0,rot[6]);
			GRRLIB_ObjectViewTrans(0,-size,-size);
			GRRLIB_ObjectViewRotate(0,-rot[7],0);	
			GRRLIB_ObjectViewTrans(0,0,-size);
			GRRLIB_ObjectViewRotate(0,-rot[8],0);
			GRRLIB_ObjectViewTrans(0,0,-size);
			GRRLIB_ObjectViewRotate(0,-rot[9],0);
			GRRLIB_ObjectViewTrans(size,0,0);
			GRRLIB_ObjectViewRotate(0,-rot[10],0);
			GRRLIB_ObjectViewTrans(size,0,0);
			GRRLIB_ObjectViewRotate(0,-rot[11],0);
			GRRLIB_ObjectViewTrans(-size,size,0);
			GRRLIB_ObjectViewRotate(rot[12],0,0);
			GRRLIB_ObjectViewTrans(0,size,0);
			GRRLIB_ObjectViewRotate(rot[13],0,0);
			GRRLIB_ObjectViewTrans(2*size,2*size,0);
			GRRLIB_ObjectViewTrans(posx,posy,posz);
			break;
		case 16:
			rot[14]+=speed;
			if(rot[14]>90.0f){
				(*stage)++;
				rot[14]=90.0f;
			}
			if(cntr<162) cntr+=tv1;
			if(alphaT[13]<254.0f) alphaT[13]+=255.0f*speed/(90.0f*tv2);
			GRRLIB_ObjectViewTrans(0,-size,size);
			GRRLIB_ObjectViewRotate(rot[0],0,0);
			GRRLIB_ObjectViewTrans(0,0,size);
			GRRLIB_ObjectViewRotate(rot[1],0,0);
			GRRLIB_ObjectViewTrans(-size,0,size);
			GRRLIB_ObjectViewRotate(0,0,rot[2]);
			GRRLIB_ObjectViewTrans(-size,0,0);
			GRRLIB_ObjectViewRotate(0,0,rot[3]);
			GRRLIB_ObjectViewTrans(-size,0,0);
			GRRLIB_ObjectViewRotate(0,0,rot[4]);
			GRRLIB_ObjectViewTrans(0,-size,0);
			GRRLIB_ObjectViewRotate(0,0,rot[5]);
			GRRLIB_ObjectViewTrans(0,-size,0);
			GRRLIB_ObjectViewRotate(0,0,rot[6]);
			GRRLIB_ObjectViewTrans(0,-size,-size);
			GRRLIB_ObjectViewRotate(0,-rot[7],0);	
			GRRLIB_ObjectViewTrans(0,0,-size);
			GRRLIB_ObjectViewRotate(0,-rot[8],0);
			GRRLIB_ObjectViewTrans(0,0,-size);
			GRRLIB_ObjectViewRotate(0,-rot[9],0);
			GRRLIB_ObjectViewTrans(size,0,0);
			GRRLIB_ObjectViewRotate(0,-rot[10],0);
			GRRLIB_ObjectViewTrans(size,0,0);
			GRRLIB_ObjectViewRotate(0,-rot[11],0);
			GRRLIB_ObjectViewTrans(-size,size,0);
			GRRLIB_ObjectViewRotate(rot[12],0,0);
			GRRLIB_ObjectViewTrans(0,size,0);
			GRRLIB_ObjectViewRotate(rot[13],0,0);
			GRRLIB_ObjectViewTrans(size,-size,0);
			GRRLIB_ObjectViewRotate(0,rot[14],0);
			GRRLIB_ObjectViewTrans(size,3*size,0);
			GRRLIB_ObjectViewTrans(posx,posy,posz);
			break;
		case 17:
			rot[15]+=speed;
			if(rot[15]>90.0f){
				rot[15]=90.0f;
				count++;
				if(count==3) (*stage)++;
			}
			if(cntr<172) cntr+=tv1;
			if(alphaT[14]<254.0f) alphaT[14]+=255.0f*speed/(90.0f*tv2);
			GRRLIB_ObjectViewTrans(0,-size,size);
			GRRLIB_ObjectViewRotate(rot[0],0,0);
			GRRLIB_ObjectViewTrans(0,0,size);
			GRRLIB_ObjectViewRotate(rot[1],0,0);
			GRRLIB_ObjectViewTrans(-size,0,size);
			GRRLIB_ObjectViewRotate(0,0,rot[2]);
			GRRLIB_ObjectViewTrans(-size,0,0);
			GRRLIB_ObjectViewRotate(0,0,rot[3]);
			GRRLIB_ObjectViewTrans(-size,0,0);
			GRRLIB_ObjectViewRotate(0,0,rot[4]);
			GRRLIB_ObjectViewTrans(0,-size,0);
			GRRLIB_ObjectViewRotate(0,0,rot[5]);
			GRRLIB_ObjectViewTrans(0,-size,0);
			GRRLIB_ObjectViewRotate(0,0,rot[6]);
			GRRLIB_ObjectViewTrans(0,-size,-size);
			GRRLIB_ObjectViewRotate(0,-rot[7],0);	
			GRRLIB_ObjectViewTrans(0,0,-size);
			GRRLIB_ObjectViewRotate(0,-rot[8],0);
			GRRLIB_ObjectViewTrans(0,0,-size);
			GRRLIB_ObjectViewRotate(0,-rot[9],0);
			GRRLIB_ObjectViewTrans(size,0,0);
			GRRLIB_ObjectViewRotate(0,-rot[10],0);
			GRRLIB_ObjectViewTrans(size,0,0);
			GRRLIB_ObjectViewRotate(0,-rot[11],0);
			GRRLIB_ObjectViewTrans(-size,size,0);
			GRRLIB_ObjectViewRotate(rot[12],0,0);
			GRRLIB_ObjectViewTrans(0,size,0);
			GRRLIB_ObjectViewRotate(rot[13],0,0);
			GRRLIB_ObjectViewTrans(size,-size,0);
			GRRLIB_ObjectViewRotate(0,rot[14],0);
			GRRLIB_ObjectViewTrans(-size,0,0);
			GRRLIB_ObjectViewRotate(0,rot[15],0);
			GRRLIB_ObjectViewTrans(2*size,3*size,0);
			GRRLIB_ObjectViewTrans(posx,posy,posz);
			break;
		case 18:
			rot[16]+=fspeed;
			if(rot[16]>=540.0f){
				(*stage)++;
				count=0;
				rot[16]=540.0f;
			}
			if(cntr<190) cntr+=tv1;
			if(rot[16]>=330.0f) aspect=-0.409999f;
			if(rot[16]>=360.0f) aspect=0.74f;
			if(rot[16]>=420.0f) aspect=0.28f;
			if(rot[16]>=450.0f) aspect=0.47f;
			if(rot[16]>=510.0f) aspect=0.56f;
			if(rot[16]>=540.0f) aspect=0.74f;
			GRRLIB_ObjectViewTrans(0,-size,size);
			GRRLIB_ObjectViewRotate(rot[0],0,0);
			GRRLIB_ObjectViewTrans(0,0,size);
			GRRLIB_ObjectViewRotate(rot[1],0,0);
			GRRLIB_ObjectViewTrans(-size,0,size);
			GRRLIB_ObjectViewRotate(0,0,rot[2]);
			GRRLIB_ObjectViewTrans(-size,0,0);
			GRRLIB_ObjectViewRotate(0,0,rot[3]);
			GRRLIB_ObjectViewTrans(-size,0,0);
			GRRLIB_ObjectViewRotate(0,0,rot[4]);
			GRRLIB_ObjectViewTrans(0,-size,0);
			GRRLIB_ObjectViewRotate(0,0,rot[5]);
			GRRLIB_ObjectViewTrans(0,-size,0);
			GRRLIB_ObjectViewRotate(0,0,rot[6]);
			GRRLIB_ObjectViewTrans(0,-size,-size);
			GRRLIB_ObjectViewRotate(0,-rot[7],0);	
			GRRLIB_ObjectViewTrans(0,0,-size);
			GRRLIB_ObjectViewRotate(0,-rot[8],0);
			GRRLIB_ObjectViewTrans(0,0,-size);
			GRRLIB_ObjectViewRotate(0,-rot[9],0);
			GRRLIB_ObjectViewTrans(size,0,0);
			GRRLIB_ObjectViewRotate(0,-rot[10],0);
			GRRLIB_ObjectViewTrans(size,0,0);
			GRRLIB_ObjectViewRotate(0,-rot[11],0);
			GRRLIB_ObjectViewTrans(-size,size,0);
			GRRLIB_ObjectViewRotate(rot[12],0,0);
			GRRLIB_ObjectViewTrans(0,size,0);
			GRRLIB_ObjectViewRotate(rot[13],0,0);
			GRRLIB_ObjectViewTrans(size,-size,0);
			GRRLIB_ObjectViewRotate(0,rot[14],0);
			GRRLIB_ObjectViewTrans(-size,0,0);
			GRRLIB_ObjectViewRotate(0,rot[15],0);
			GRRLIB_ObjectViewTrans(-size,1.5*size,-0.5*size);
			delta+=0.3f/9.0f*tv1;
			GRRLIB_ObjectViewScale(delta,delta,delta);
			GRRLIB_ObjectViewRotate(rot[16],0,0);
			GRRLIB_ObjectViewTrans(3*size,1.5*size,0.5*size);
			if(rot[16]<=60.0f) posy-=28.5f*tv1;
			if(rot[16]>60.0f && rot[16]<=120.0f) posy-=19.0f*tv1;
			if(rot[16]>120.0f && rot[16]<=180.0f) posy-=12.5f*tv1;
			if(rot[16]>180.0f && rot[16]<=240.0f) posy-=6.0f*tv1;
			if(rot[16]>240.0f && rot[16]<=300.0f) posy-=3.0f*tv1;
			if(rot[16]>300.0f && rot[16]<=360.0f) posy-=2.0f*tv1;
			if(rot[16]>360.0f && rot[16]<=420.0f) posy-=2.0f*tv1;
			if(rot[16]>420.0f && rot[16]<=480.0f) posy-=2.0f*tv1;
			if(rot[16]>480.0f && rot[16]<=540.0f) posy-=1.5f*tv1;
			posz-=30.0f/9.0f*tv1;
			posx+=4.3f/9.0f*tv1;
			GRRLIB_ObjectViewTrans(posx,posy,posz);
			break;
		case 19:
			count+=1;
			if(count==freeze) (*stage)++;
			GRRLIB_ObjectViewTrans(0,-size,size);
			GRRLIB_ObjectViewRotate(rot[0],0,0);
			GRRLIB_ObjectViewTrans(0,0,size);
			GRRLIB_ObjectViewRotate(rot[1],0,0);
			GRRLIB_ObjectViewTrans(-size,0,size);
			GRRLIB_ObjectViewRotate(0,0,rot[2]);
			GRRLIB_ObjectViewTrans(-size,0,0);
			GRRLIB_ObjectViewRotate(0,0,rot[3]);
			GRRLIB_ObjectViewTrans(-size,0,0);
			GRRLIB_ObjectViewRotate(0,0,rot[4]);
			GRRLIB_ObjectViewTrans(0,-size,0);
			GRRLIB_ObjectViewRotate(0,0,rot[5]);
			GRRLIB_ObjectViewTrans(0,-size,0);
			GRRLIB_ObjectViewRotate(0,0,rot[6]);
			GRRLIB_ObjectViewTrans(0,-size,-size);
			GRRLIB_ObjectViewRotate(0,-rot[7],0);	
			GRRLIB_ObjectViewTrans(0,0,-size);
			GRRLIB_ObjectViewRotate(0,-rot[8],0);
			GRRLIB_ObjectViewTrans(0,0,-size);
			GRRLIB_ObjectViewRotate(0,-rot[9],0);
			GRRLIB_ObjectViewTrans(size,0,0);
			GRRLIB_ObjectViewRotate(0,-rot[10],0);
			GRRLIB_ObjectViewTrans(size,0,0);
			GRRLIB_ObjectViewRotate(0,-rot[11],0);
			GRRLIB_ObjectViewTrans(-size,size,0);
			GRRLIB_ObjectViewRotate(rot[12],0,0);
			GRRLIB_ObjectViewTrans(0,size,0);
			GRRLIB_ObjectViewRotate(rot[13],0,0);
			GRRLIB_ObjectViewTrans(size,-size,0);
			GRRLIB_ObjectViewRotate(0,rot[14],0);
			GRRLIB_ObjectViewTrans(-size,0,0);
			GRRLIB_ObjectViewRotate(0,rot[15],0);
			GRRLIB_ObjectViewTrans(-size,1.5*size,-0.5*size);
			GRRLIB_ObjectViewScale(delta,delta,delta);
			GRRLIB_ObjectViewRotate(rot[16],0,0);
			GRRLIB_ObjectViewTrans(3*size,1.5*size,0.5*size);
			GRRLIB_ObjectViewTrans(posx,posy,posz);
			break;
		case 20:
			posy+=fall;
			if(posy>=-182.0f){
				(*stage)++;
				posy=-182.0f;
			}
			GRRLIB_ObjectViewTrans(0,-size,size);
			GRRLIB_ObjectViewRotate(rot[0],0,0);
			GRRLIB_ObjectViewTrans(0,0,size);
			GRRLIB_ObjectViewRotate(rot[1],0,0);
			GRRLIB_ObjectViewTrans(-size,0,size);
			GRRLIB_ObjectViewRotate(0,0,rot[2]);
			GRRLIB_ObjectViewTrans(-size,0,0);
			GRRLIB_ObjectViewRotate(0,0,rot[3]);
			GRRLIB_ObjectViewTrans(-size,0,0);
			GRRLIB_ObjectViewRotate(0,0,rot[4]);
			GRRLIB_ObjectViewTrans(0,-size,0);
			GRRLIB_ObjectViewRotate(0,0,rot[5]);
			GRRLIB_ObjectViewTrans(0,-size,0);
			GRRLIB_ObjectViewRotate(0,0,rot[6]);
			GRRLIB_ObjectViewTrans(0,-size,-size);
			GRRLIB_ObjectViewRotate(0,-rot[7],0);	
			GRRLIB_ObjectViewTrans(0,0,-size);
			GRRLIB_ObjectViewRotate(0,-rot[8],0);
			GRRLIB_ObjectViewTrans(0,0,-size);
			GRRLIB_ObjectViewRotate(0,-rot[9],0);
			GRRLIB_ObjectViewTrans(size,0,0);
			GRRLIB_ObjectViewRotate(0,-rot[10],0);
			GRRLIB_ObjectViewTrans(size,0,0);
			GRRLIB_ObjectViewRotate(0,-rot[11],0);
			GRRLIB_ObjectViewTrans(-size,size,0);
			GRRLIB_ObjectViewRotate(rot[12],0,0);
			GRRLIB_ObjectViewTrans(0,size,0);
			GRRLIB_ObjectViewRotate(rot[13],0,0);
			GRRLIB_ObjectViewTrans(size,-size,0);
			GRRLIB_ObjectViewRotate(0,rot[14],0);
			GRRLIB_ObjectViewTrans(-size,0,0);
			GRRLIB_ObjectViewRotate(0,rot[15],0);
			GRRLIB_ObjectViewTrans(-size,1.5*size,-0.5*size);
			GRRLIB_ObjectViewScale(delta,delta,delta);
			GRRLIB_ObjectViewRotate(rot[16],0,0);
			GRRLIB_ObjectViewTrans(3*size,1.5*size,0.5*size);
			GRRLIB_ObjectViewTrans(posx,posy,posz);
			break;
		case 21:
			GRRLIB_ObjectViewTrans(0,-size,size);
			GRRLIB_ObjectViewRotate(rot[0],0,0);
			GRRLIB_ObjectViewTrans(0,0,size);
			GRRLIB_ObjectViewRotate(rot[1],0,0);
			GRRLIB_ObjectViewTrans(-size,0,size);
			GRRLIB_ObjectViewRotate(0,0,rot[2]);
			GRRLIB_ObjectViewTrans(-size,0,0);
			GRRLIB_ObjectViewRotate(0,0,rot[3]);
			GRRLIB_ObjectViewTrans(-size,0,0);
			GRRLIB_ObjectViewRotate(0,0,rot[4]);
			GRRLIB_ObjectViewTrans(0,-size,0);
			GRRLIB_ObjectViewRotate(0,0,rot[5]);
			GRRLIB_ObjectViewTrans(0,-size,0);
			GRRLIB_ObjectViewRotate(0,0,rot[6]);
			GRRLIB_ObjectViewTrans(0,-size,-size);
			GRRLIB_ObjectViewRotate(0,-rot[7],0);	
			GRRLIB_ObjectViewTrans(0,0,-size);
			GRRLIB_ObjectViewRotate(0,-rot[8],0);
			GRRLIB_ObjectViewTrans(0,0,-size);
			GRRLIB_ObjectViewRotate(0,-rot[9],0);
			GRRLIB_ObjectViewTrans(size,0,0);
			GRRLIB_ObjectViewRotate(0,-rot[10],0);
			GRRLIB_ObjectViewTrans(size,0,0);
			GRRLIB_ObjectViewRotate(0,-rot[11],0);
			GRRLIB_ObjectViewTrans(-size,size,0);
			GRRLIB_ObjectViewRotate(rot[12],0,0);
			GRRLIB_ObjectViewTrans(0,size,0);
			GRRLIB_ObjectViewRotate(rot[13],0,0);
			GRRLIB_ObjectViewTrans(size,-size,0);
			GRRLIB_ObjectViewRotate(0,rot[14],0);
			GRRLIB_ObjectViewTrans(-size,0,0);
			GRRLIB_ObjectViewRotate(0,rot[15],0);
			GRRLIB_ObjectViewTrans(-size,1.5*size,-0.5*size);
			GRRLIB_ObjectViewScale(delta,delta,delta);
			GRRLIB_ObjectViewRotate(rot[16],0,0);
			GRRLIB_ObjectViewTrans(3*size,1.5*size,0.5*size);
			GRRLIB_ObjectViewTrans(posx,posy,posz);
			break;
	}
	GRRLIB_ObjectViewEnd();	
}

void dispSmallCube(GXTexObj* env_tex, f32 alphaSC){
	u8 i;
	
	// Rolling cube
	GX_SetTexCoordGen(GX_TEXCOORD0, GX_TG_MTX3x4, GX_TG_NRM, GX_TEXMTX0);	// This block of code (before GX_Begin) is taken from NoNameNo's example from GRRLIB's
	guLightPerspective(mv, ffovy, aspect, nn, oo, pp, qq);					// for texture projection. We use the models' normals to position the texture.
	guMtxRotDeg(rx, 'X', rotcX[(int) cntr]);								// This block or others below might interfere with GRRLIB because the app would
	guMtxRotDeg(ry, 'Y', rotcY[(int) cntr]);								// not return to the system menu if I hadn't added the GX calls at the end of the 
	guMtxRotDeg(rz, 'Z', rotcZ[(int) cntr]);								// main while loop. Not sure if that fix is enough but testing on hardware tends to confirm it.
	guMtxConcat(ry, rx, m); 
	guMtxConcat(m, rz, m);  
	guMtxConcat(mv, m, mv); 
	guMtxScale(mr, 0.5*ssX, ssX, 0.0f);
	guMtxConcat(mr, mv, mv);
	GX_LoadTexMtxImm(mv, GX_TEXMTX0, GX_MTX3x4);
	GX_SetNumTexGens(1);
	
	GXColorS10 scl = {40, 20, 140, 0 };
	GX_SetTevColorS10(GX_TEVREG0, scl);
	GXColorS10 scd = {189, 159, 255, 0 };
	GX_SetTevColorS10(GX_TEVREG1, scd);
	
	GX_SetTevColorIn(GX_TEVSTAGE0,GX_CC_C0,GX_CC_C1,GX_CC_TEXC,GX_CC_ZERO);
	GX_SetTevAlphaIn(GX_TEVSTAGE0,GX_CA_ZERO,GX_CA_TEXA,GX_CA_RASA,GX_CA_RASA);
	GX_SetTevColorOp(GX_TEVSTAGE0,GX_TEV_ADD,GX_TB_ZERO,GX_CS_SCALE_1,GX_TRUE,GX_TEVPREV);
	GX_SetTevAlphaOp(GX_TEVSTAGE0,GX_TEV_ADD,GX_TB_ZERO,GX_CS_SCALE_1,GX_TRUE,GX_TEVPREV);
	
	GX_SetTevOrder(GX_TEVSTAGE0, GX_TEXCOORD0, GX_TEXMAP0, GX_COLOR0A0);
	GX_LoadTexObj(env_tex, GX_TEXMAP0);
	GX_Begin(GX_TRIANGLES, GX_VTXFMT0, 612);
	for (i=0;i<204;i++){
		GX_Position3f32(vsCube[fsCube[i][0][0]-1][0], vsCube[fsCube[i][0][0]-1][1], vsCube[fsCube[i][0][0]-1][2]);
		GX_Normal3f32(vnsCube[fsCube[i][0][2]-1][0], vnsCube[fsCube[i][0][2]-1][1], vnsCube[fsCube[i][0][2]-1][2]);
		GX_Color4u8(cr,cg,cb,alphaSC);
		GX_Position3f32(vsCube[fsCube[i][1][0]-1][0], vsCube[fsCube[i][1][0]-1][1], vsCube[fsCube[i][1][0]-1][2]);
		GX_Normal3f32(vnsCube[fsCube[i][1][2]-1][0], vnsCube[fsCube[i][1][2]-1][1], vnsCube[fsCube[i][1][2]-1][2]);
		GX_Color4u8(cr,cg,cb,alphaSC);
		GX_Position3f32(vsCube[fsCube[i][2][0]-1][0], vsCube[fsCube[i][2][0]-1][1], vsCube[fsCube[i][2][0]-1][2]);
		GX_Normal3f32(vnsCube[fsCube[i][2][2]-1][0], vnsCube[fsCube[i][2][2]-1][1], vnsCube[fsCube[i][2][2]-1][2]);
		GX_Color4u8(cr,cg,cb,alphaSC);
	}
	GX_End();
}

void dispBigCube(GXTexObj* env_tex, f32 alpha){
	u8 i;
	
	//Big translucent cube
	GX_SetTexCoordGen(GX_TEXCOORD0, GX_TG_MTX3x4, GX_TG_NRM, GX_TEXMTX0);
	guLightPerspective(mv, fovy, 1.0f, n, o, p, q);
	guMtxScale(mr, sX, sX, 0.0f);
	guMtxConcat(mr, mv, mv);
	GX_LoadTexMtxImm(mv, GX_TEXMTX0, GX_MTX3x4);
	GX_SetNumTexGens(1);
	
	GXColorS10 bcl = {40, 20, 140, 0 };
	GX_SetTevColorS10(GX_TEVREG0, bcl);
	GXColorS10 bcd = {189, 159, 255, 0 };
	GX_SetTevColorS10(GX_TEVREG1, bcd);
	
	GX_SetTevColorIn(GX_TEVSTAGE0,GX_CC_C0,GX_CC_C1,GX_CC_TEXC,GX_CC_ZERO);
	GX_SetTevAlphaIn(GX_TEVSTAGE0,GX_CA_ZERO,GX_CA_TEXA,GX_CA_RASA,GX_CA_ZERO);
	GX_SetTevColorOp(GX_TEVSTAGE0,GX_TEV_ADD,GX_TB_ZERO,GX_CS_SCALE_1,GX_TRUE,GX_TEVPREV);
	GX_SetTevAlphaOp(GX_TEVSTAGE0,GX_TEV_ADD,GX_TB_ZERO,GX_CS_SCALE_1,GX_TRUE,GX_TEVPREV);
	
	GX_SetTevOrder(GX_TEVSTAGE0, GX_TEXCOORD0, GX_TEXMAP0, GX_COLOR0A0);
	GX_LoadTexObj(env_tex, GX_TEXMAP0);
	GRRLIB_ObjectView(posxB,posyB,poszB,rotxB,rotyB,rotzB,1.0f,1.0f,1.0f);
	GX_Begin(GX_TRIANGLES, GX_VTXFMT0, 612);
	for (i=0;i<204;i++){
		GX_Position3f32(vbCube[fbCube[i][0][0]-1][0], vbCube[fbCube[i][0][0]-1][1], vbCube[fbCube[i][0][0]-1][2]);
		GX_Normal3f32(vnbCube[fbCube[i][0][2]-1][0], vnbCube[fbCube[i][0][2]-1][1], vnbCube[fbCube[i][0][2]-1][2]);
		GX_Color4u8(cr,cg,cb,alpha);
		GX_Position3f32(vbCube[fbCube[i][1][0]-1][0], vbCube[fbCube[i][1][0]-1][1], vbCube[fbCube[i][1][0]-1][2]);
		GX_Normal3f32(vnbCube[fbCube[i][1][2]-1][0], vnbCube[fbCube[i][1][2]-1][1], vnbCube[fbCube[i][1][2]-1][2]);
		GX_Color4u8(cr,cg,cb,alpha);
		GX_Position3f32(vbCube[fbCube[i][2][0]-1][0], vbCube[fbCube[i][2][0]-1][1], vbCube[fbCube[i][2][0]-1][2]);
		GX_Normal3f32(vnbCube[fbCube[i][2][2]-1][0], vnbCube[fbCube[i][2][2]-1][1], vnbCube[fbCube[i][2][2]-1][2]);
		GX_Color4u8(cr,cg,cb,alpha);
	}
	GX_End();
}