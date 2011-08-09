/*!
 * BGModel.h
 *
 *  Created on: 19/07/2011
 *      Author: chao
 */

#ifndef BGMODEL_H_
#define BGMODEL_H_

#include <opencv2/video/background_segm.hpp>
#include <opencv2/imgproc/imgproc_c.h>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/highgui/highgui.hpp>

#include <stdio.h>
#include <iostream>
#include <fstream>
#include <math.h>

#include "Libreria.h"

#define FRAMES_TRAINING 20

// VARIABLES GLOBALES DE PROGRAMA //

	// Float 1-Channel
	IplImage *IdiffF;
	IplImage *IvarF; /// Varianza
	IplImage *IhiF; /// La mediana mas x veces la desviación típica
	IplImage *IlowF; /// La mediana menos x veces la desviación típica

	//Byte 1-Channel
	IplImage *ImGray;
	IplImage *Imaskt;

// PROTOTIPOS DE FUNCIONES //

void UpdateBackground(IplImage * tmp_frame, IplImage* bg_model );

int initBGGModel( CvCapture* t_capture, IplImage* BG, IplImage* ImMask);

IplImage* getBinaryImage(IplImage * image);

void accumulateBackground( IplImage* ImGray, IplImage* BGMod);

//void PreProcesado( IplImage* frame,IplImage* Im, IplImage* ImFMask,bool bin);

//void invertirBW( IplImage* Mask  );
void error(int err);
void AllocateImagesBGM( IplImage *I );
void DeallocateImagesBGM();


#endif /* BGMODEL_H_ */
