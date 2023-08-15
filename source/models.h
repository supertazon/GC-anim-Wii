#ifndef _MODELS_H_
#define _MODELS_H_

#include <gctypes.h>

//Small rolling cube (sCube)
extern f32 vsCube[104][3];		// Array with vertices
extern f32 vnsCube[104][3];		// Array with normals
extern u16 fsCube[204][3][3];	// Array with faces

// Texture projection coordinates for the small rolling cube, index is a frame counter
extern f32 rotcX[191];
extern f32 rotcY[191];
extern f32 rotcZ[191];

// Logo
extern f32 vLogo[100][3];
extern f32 vnLogo[6][3];
extern f32 vtLogo[163][2]; // Array with texture UV coordinates
extern u16 fLogo[90][3][3];

// Logo Top
extern f32 vLogoT[18][3];
extern f32 vnLogoT[1][3];
extern f32 vtLogoT[18][2];
extern u16 fLogoT[12][3][3];

// Logo Left
extern f32 vLogoL[18][3];
extern f32 vnLogoL[1][3];
extern f32 vtLogoL[18][2];
extern u16 fLogoL[10][3][3];

// Logo Right
extern f32 vLogoR[20][3];
extern f32 vnLogoR[1][3];
extern f32 vtLogoR[20][2];
extern u16 fLogoR[14][3][3];

// Big transparent cube (bCube)
extern f32 vbCube[104][3];
extern f32 vnbCube[104][3];
extern u16 fbCube[204][3][3];

// Tile
extern f32 vTile[6][3];
extern f32 vnTile[1][3];
extern f32 vtTile[4][2];
extern u16 fTile[2][3][3];

//N
extern f32 vN[88][3];
extern f32 vnN[8][3];
extern f32 vtN[88][2];
extern u16 fN[36][3][3];

//I
extern f32 vI[28][3];
extern f32 vnI[6][3];
extern f32 vtI[28][2];
extern u16 fI[12][3][3];

//T
extern f32 vT[68][3];
extern f32 vnT[7][3];
extern f32 vtT[70][2];
extern u16 fT[28][3][3];

//E
extern f32 vE[268][3];
extern f32 vnE[25][3];
extern f32 vtE[268][2];
extern u16 fE[108][3][3];

//D
extern f32 vD[240][3];
extern f32 vnD[23][3];
extern f32 vtD[240][2];
extern u16 fD[96][3][3];

//O
extern f32 vO[400][3];
extern f32 vnO[43][3];
extern f32 vtO[400][2];
extern u16 fO[160][3][3];

#endif