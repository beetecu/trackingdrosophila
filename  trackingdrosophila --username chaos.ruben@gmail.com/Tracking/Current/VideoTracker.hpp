/*
 * VideoTracker.hpp
 *
 *  Created on: 12/08/2011
 *      Author: chao
 */

#ifndef VIDEOTRACKER_HPP_
#define VIDEOTRACKER_HPP_ //Modifica

#include <opencv2/imgproc/imgproc_c.h>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <stdio.h>
#include <iostream>
#include <fstream>
#include <math.h>
#include <opencv2/video/background_segm.hpp>
#include "Errores.hpp"
#include "Libreria.h"

#define CVX_RED CV_RGB(255,0,0)
#define CVX_BLUE CV_RGB(0,0,255)
#define CVX_GREEN CV_RGB(0,255,0)
#define CVX_WHITE CV_RGB(255,255,255)
#define CVX_BLACK CV_RGB(0,0,0)

#define PI 3.14159265

#define IMAGE_BUFFER_LENGTH 51 // valor impar
//Opciones de visualización


#define CREATE_TRACKBARS 0 ///<- switch from 0 to 1 para visualizar trackbars.
#define SHOW_BG_REMOVAL 1 ///<- switch from 0 to 1 para visualizar background y foreground.
#define SHOW_VISUALIZATION 0 ///<- switch from 0 to 1 para visualizar resultado.
#define SHOW_OPTICAL_FLOW 0 ///<- switch from 0 to 1 para visualizar flujo optico.
#define SHOW_MOTION_TEMPLATE 0
#define SHOW_BACKGROUND_DATA 1
#define SHOW_SEGMENTATION_DATA 0
#define SHOW_SEGMENTACION_STRUCT 0


#define SHOW_SHAPE_MODEL_DATA_AREAS 0
#define SHOW_SHAPE_MODEL_DATA_MEDIANA 1



#ifndef DEFINICION_DE_ESTRUCTURAS
#define DEFINICION_DE_ESTRUCTURAS

//	typedef struct{
//
//	};
	typedef struct Flies{

		int etiqueta;  /// Identificación del blob
		CvScalar Color; /// Color para dibujar el blob
		CvPoint posicion; /// Posición del blob
		float a,b; /// semiejes de la elipse
		float orientacion; /// Almacena la orientación
		double perimetro;
		CvRect Roi;
		bool Static;  /// Flag para indicar que el blob permanece estático
		int num_frame; /// Almacena el numero de frame (tiempo)
		int num_Flies_frame; /// Número de blobs detectados en el frame
		int num_total_elementos; /// Número de blobs en la estructura hasta el momento.
		int num_frames; /// almacena el número de frames de la lista
		struct Flies* siguiente_frame;
		struct Flies* anterior_frame;
		struct Flies* siguiente;
		struct Flies* anterior;
	}STFlies;

//	typedef struct {
//		IplImage** BGModel;
//		IplImage** Idesv;
//		IplImage** FG;
//		IplImage** OldFG;
//
//	}BuffSTCapas;
/// Estructura para almacenar el modelo de fondo estático
	typedef struct {
		IplImage* Imed; ///BackGround Model
		IplImage* IDesv; /// Desviación tipica del modelo de fondo estático
		IplImage* ImFMask; /// Mascara del plato
	}StaticBGModel;

	typedef struct Capas{
		int num_frame;
		int num_elementos;
		IplImage* BGModel;  /// backGround Model dinámico
		IplImage* IDesv;	/// Desviación tipica del modelo de fondo dinámico
		IplImage* OldFG; ///OldForeGround ( blobs estáticos ).
		IplImage* FG;  ///Foreground ( blobs en movimiento ).
		IplImage* ImMotion;
		struct Capas* siguiente_frame;
		struct Capas* anterior_frame;
	}STCapas;

	/// Estructura para el modelo del plato

	typedef struct {
		int PCentroX ;
		int PCentroY ;
		int PRadio ;
		CvRect DataFROI;
	}STFlat;

	/// Estructura para el modelo de forma
typedef struct {
	float FlyAreaMed ;
	float FlyAreaMedia;
	float FlyAreaDes ;
}SHModel;

#endif

#endif /* VIDEOTRACKER_HPP_ */
