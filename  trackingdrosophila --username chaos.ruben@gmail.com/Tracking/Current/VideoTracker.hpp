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
#include <cvaux.h>
#include <stdio.h>
#include <cv.h>
#include <cxcore.h>


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
#define ULTIMO IMAGE_BUFFER_LENGTH-1
#define PRIMERO 0


// Opciones generales de programa

#define DETECTAR_PLATO 1
#define MEDIR_TIEMPOS 1
//Opciones de visualización de datos y tiempos de procesos
#define CREATE_TRACKBARS 1 //!< Switch de 0 a 1 para visualizar trackbars.

#define SHOW_SHAPE_MODELING 0//!< Switch de 0 a 1 para visualizar los resultados del modelado de forma.
#define SHOW_SHAPE_MODEL_DATA_AREAS 0//!< Switch de 0 a 1 para visualizar el valor de las areas de cada blob.
#define SHOW_SHAPE_MODEL_DATA_MEDIANA 0//!< Switch de 0 a 1 para visualizar el valor mediana para todos los blobs.

#define SHOW_PROCESS_TIMES

#define SHOW_BGMODEL_DATA 0 //!< Switch de 0 a 1 para visualizar el Background.
#define SHOW_BGMODEL_TIMES 0

#define SHOW_SEGMENTATION_TIMES 1 //!< Switch de 0 a 1 para visualizar los tiempos.
#define SHOW_SEGMENTATION_DATA 0 //!< Switch de 0 a 1 para visualizar los resulatdos de la segmentación.
#define SHOW_SEGMENTATION_MATRIX 0

#define SHOW_VALIDATION_DATA 1//!< Switch de 0 a 1 para visualizar los resulatdos de la validación.

#define SHOW_VALIDATION_TIMES 1
#define SHOW_OPTICAL_FLOW 0 //!< Switch de 0 a 1 para visualizar el flujo optico.
#define SHOW_MOTION_TEMPLATE 1//!< Switch de 0 a 1 para visualizar el gradiente.
#define SHOW_DATA_ASSIGNMENT 1

#define SHOW_DATA_ASSIGNMENT 0

#define SHOW_KALMAN_RESULT 1

// visualización en ventana
#define SHOW_VISUALIZATION 1 //!< Switch de 0 a 1 para visualizar el resultado.
#define SHOW_BG_REMOVAL 1 //!< Switch de 0 a 1 para visualizar el Background y Foreground.

#ifndef DEFINICION_DE_ESTRUCTURAS
#define DEFINICION_DE_ESTRUCTURAS

#ifndef _ELEMENTO_H
#define _ELEMENTO_H

/// Tipo Elemento (un elemento de la lista)

	typedef struct s
	{
	  void *dato;          //!< área de datos
	  struct s *anterior;  //!< puntero al elemento anterior
	  struct s *siguiente; //!< puntero al elemento siguiente
	} Elemento;


#endif // _ELEMENTO_H


#ifndef _INTERFAZ_LCSE_H
#define _INTERFAZ_LCSE_H

/// Parámetros de la lista.

	typedef struct
{
  Elemento *ultimo;      //!< apuntará siempre al último elemento
  Elemento *actual;      //!< apuntará siempre al elemento accedido
  int numeroDeElementos; //!< número de elementos de la lista
  int posicion;          //!< índice del elemento apuntado por actual
} tlcde;

#endif //_INTERFAZ_LCSE_H

	typedef struct {


		int etiqueta;  //!< Identificación del blob
		CvScalar Color; //!< Color para dibujar el blob
		CvPoint posicion; //!< Posición del blob
		float a,b; //!< semiejes de la elipse
		float orientacion; //!< Almacena la orientación
		float direccion; //!<almacena la dirección del desplazamiento
		float dstTotal; //!< almacena la distancia total recorrida hasta el momento
		double perimetro;
		CvRect Roi; //!< region de interes para el blob
		bool Estado;  //!< Flag para indicar que el blob permanece estático.Servirá para indicar si está en el foreground o en el oldforeground
		bool flag_seg; //!< Indica si e blog a sido segmentado
		bool flag_def; //!< indica si el blob ha  desaparecido durante el analisis del defecto
		int CountState; //!< Si alcanza un valor umbral consideraremos que el blob permanece estático y estado = 0;
		int num_frame; //!< Almacena el numero de frame (tiempo)
		bool salto;	//!< Indica que la mosca ha saltado
		bool Grupo; //!< Indica que la mosca permanece estática en un grupo de 2 o más moscas.
		int Zona; //!< Si se seleccionan zonas de interes en el plato,
						///este flag indicará si el blob se encuentra en alguna de las regiones marcadas

	}STFly;

/// Estructura para almacenar el modelo de fondo estático.

	typedef struct {
		IplImage* Imed; //!< Imagen que contiene el modelo de fondo (Background Model).
		IplImage* IDesvf; //!< Imagen que contiene la desviación tipica del modelo de fondo estático.
		IplImage* ImFMask; //!<Imagen que contiene la  Mascara del plato
		int PCentroX ;//!< Coordenada x del centro del plato.
		int PCentroY ;//!< Coordenada y del centro del plato.
		int PRadio ;//!< Radio del plato.
		CvRect DataFROI;//!< Region de interes del plato.
	}StaticBGModel;

/// Estructura que almacena cálculos estadísticos globales simples para mostrar en tiempo de ejecución.

	typedef struct {
		int totalFrames; //!< Numero de Frames que posee en video
		int numFrame; //!< Numero de Frame que se está procesando.
		float TProces;
		float TTacking;
		float staticBlobs; //!< blobs estáticos en tanto por ciento.
		float CantidadMov;
		float TiempoFrame;
		float TiempoGlobal;
		float CMov30;  //!< Cantidad de movimiento medio en los últimos 30 min.
		float CMov1H;  //!< Cantidad de movimiento medio en la última hora.
		float CMov2H;	//!< Cabtidad de movimiento medio en  últimas 2 horas.
		float CMov4H;
		float CMov8H;
		float CMov16H;
		float CMov24H;
		float CMov48H;
		float CMovMedio;	//!< Cantidad de movimiento medio desde el comienzo.
	}STStatFrame;

/// Estructura que almacena las capas del frame, los datos para realizar calculos estadisticos simples en
/// tiempo de ejecución y la lista de los con los datos de cada blob.

	typedef struct {
		int num_frame; //!< Identificación del Frame procesado.
		IplImage* Frame;//!< Imagen fuente de 8 bit de niveles de gris preprocesada.
		IplImage* BGModel;//!< Imagen de 8 bits que contiene e  BackGround Model Dinámico.
		IplImage* IDesvf;//!< Imagen de 32 bits que contiene la Desviación Típica del modelo de fondo dinámico.
		IplImage* OldFG; //!< Imagen que contiene el OldForeGround ( blobs estáticos ).
		IplImage* FG;  //!< Imagen que contiene el Foreground ( blobs en movimiento ).
		IplImage* ImMotion;//!< Imagen que contiene la orientación de moscas.
//		STStatFrame * Stat;
		tlcde* Flies; //!< Puntero a lista circular doblemente enlazada (tlcde) con los datos de cada Mosca.
	}STFrame;

/// Estructura para el modelo de forma de los blobs

	typedef struct {
	float FlyAreaMed ; //!< Mediana de las areas de los blobs que se encuentran en movimiento en cada frame.
	float FlyAreaMedia;//!< Media de las areas de los blobs que se encuentran en movimiento en cada frame.
	float FlyAreaDes ; //!< Desviación típica de las areas de los blobs que se encuentran en movimiento en cada frame.
}SHModel;


#endif /*Estructuras*/

#endif /* VIDEOTRACKER_HPP_ */
