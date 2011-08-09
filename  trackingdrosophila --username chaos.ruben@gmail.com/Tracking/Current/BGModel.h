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
	IplImage *Iscratch; /// Imagen preprocesada en float 32 bit
	IplImage *Iscratch2; /// Contendrá la mediana. Float 32 bit

	//Byte 1-Channel
	IplImage *ImGray;
	IplImage temp_frame;
	IplImage *Imaskt;


// PROTOTIPOS DE FUNCIONES //

IplImage* updateBackground(CvBGStatModel * bg_model, IplImage * tmp_frame);


int initBGGModel( CvCapture* t_capture, IplImage* BG, IplImage* ImMask);

IplImage* getBinaryImage(IplImage * image);

void accumulateBackground( CvCapture* t_cap, IplImage* BGMod, IplImage* Mask);

//void PreProcesado( IplImage* frame,IplImage* Im, IplImage* ImFMask,bool bin);

//void invertirBW( IplImage* Mask  );
void error(int err);
void AllocateImagesBGM( IplImage *I );
void DeallocateImagesBGM();


#endif /* BGMODEL_H_ */
