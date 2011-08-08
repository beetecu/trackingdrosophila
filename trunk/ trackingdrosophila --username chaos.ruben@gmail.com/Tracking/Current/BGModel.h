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

#define FRAMES_TRAINING 20

// VARIABLES GLOBALES DE PROGRAMA //

	// Float 1-Channel
	IplImage *IavgF, *IvarF, *IdiffF, *IprevF, *IhiF, *IlowF;
	IplImage *Iscratch, *Iscratch2;
	IplImage *ImGray;
	//Byte 1-Channel
	IplImage temp_frame;
	IplImage *Imaskt;


// PROTOTIPOS DE FUNCIONES //

IplImage* updateBackground(CvBGStatModel * bg_model, IplImage * tmp_frame);


int initBGGModel( CvCapture* t_capture, IplImage* BG, IplImage* ImMask);

IplImage* getBinaryImage(IplImage * image);

void accumulateBackground( CvCapture* t_cap, IplImage* BGMod, IplImage* Mask);

void PreProcesado( IplImage* frame,IplImage* Im, IplImage* ImFMask,bool bin);

void invertirBW( IplImage* Mask  );
void error(int err);
void AllocateImagesBGM( IplImage *I );
void DeallocateImagesBGM();


#endif /* BGMODEL_H_ */
