/*
 * VideoTracker.hpp
 *
 *  Created on: 12/08/2011
 *      Author: chao
 */

#ifndef VIDEOTRACKER_HPP_
#define VIDEOTRACKER_HPP_

#include <opencv2/imgproc/imgproc_c.h>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <stdio.h>
#include <iostream>
#include <fstream>
#include <math.h>
#include <time.h>
#include <sys/time.h>
#include <opencv2/video/background_segm.hpp>

using namespace cv;
using namespace std;
//#include "Libreria.h"

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

#ifndef _ELEMENTO_H
#define _ELEMENTO_H

	// Tipo Elemento (un elemento de la lista) ///////////////////////
	typedef struct s
	{
	  void *dato;          // área de datos
	  struct s *anterior;  // puntero al elemento anterior
	  struct s *siguiente; // puntero al elemento siguiente
	} Elemento;


#endif // _ELEMENTO_H


#ifndef _INTERFAZ_LCSE_H
#define _INTERFAZ_LCSE_H

// Parámetros de la lista
typedef struct
{
  Elemento *ultimo;      // apuntará siempre al último elemento
  Elemento *actual;      // apuntará siempre al elemento accedido
  int numeroDeElementos; // número de elementos de la lista
  int posicion;          // índice del elemento apuntado por actual
} tlcde;

#endif //_INTERFAZ_LCSE_H

	typedef struct {

		int etiqueta;  /// Identificación del blob
		CvScalar Color; /// Color para dibujar el blob
		CvPoint posicion; /// Posición del blob
		float a,b; /// semiejes de la elipse
		float orientacion; /// Almacena la orientación
		double perimetro;
		CvRect Roi;
		bool Static;  /// Flag para indicar que el blob permanece estático.
		int num_frame; /// Almacena el numero de frame (tiempo)
	}STFly;

/// Estructura para almacenar el modelo de fondo estático
	typedef struct {
		IplImage* Imed; ///BackGround Model
		IplImage* IDesv; /// Desviación tipica del modelo de fondo estático
		IplImage* ImFMask; /// Mascara del plato
	}StaticBGModel;


	typedef struct {
		int num_frame;
		IplImage* BGModel;  /// backGround Model dinámico
		IplImage* IDesv;	/// Desviación tipica del modelo de fondo dinámico
		IplImage* OldFG; ///OldForeGround ( blobs estáticos ).
		IplImage* FG;  ///Foreground ( blobs en movimiento ).
		IplImage* ImMotion;
		tlcde* Flies; /// Puntero a lista circular doblemente enlazada con los datos de cada fly
	}STFrame;

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

#endif /*Estructuras*/

#endif /* VIDEOTRACKER_HPP_ */
