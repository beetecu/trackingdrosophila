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




int initBGGModel( CvCapture* t_capture, IplImage* BG, IplImage* ImMask);

IplImage* getBinaryImage(IplImage * image);
//! \brief Recibe una imagen en escala de grises preprocesada. estima a la mediana
//!	en BGMod y la varianza en IvarF según:
/*! Mediana:
//!	\f[
//!		mu(p)= median\I_t(p)
//!		\f]
    /*
      \param ImGray : Imagen fuente de 8 bit niveles de gris.
      \param BGMod : Imagen de fondo sobre la que se estima la mediana
    */

void accumulateBackground( IplImage* ImGray, IplImage* BGMod);
void UpdateBackground(IplImage * tmp_frame, IplImage* bg_model );
void BackgroundDifference( IplImage* ImGray, IplImage* bg_model, int HiF, int LowF );
void setHighThreshold( IplImage* BG, int HiF );
void setLowThreshold( IplImage* BG, int LowF );
void error(int err);
void AllocateImagesBGM( IplImage *I );
void DeallocateImagesBGM();


#endif /* BGMODEL_H_ */
