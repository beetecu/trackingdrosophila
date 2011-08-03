/*
 * Tracking.h
 *
 *  Created on: 19/07/2011
 *      Author: chao
 */

#ifndef TRACKING_H_
#define TRACKING_H_

#include <opencv2/video/background_segm.hpp>
#include <opencv2/imgproc/imgproc_c.h>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/highgui/highgui.hpp>

#include <stdio.h>

#include <BlobResult.h>

#include <iostream>
#include <fstream>
#include <math.h>

#include "Interfaz_Lista.h"

using namespace cv;
using namespace std;

// DEFINICIONES

#define CVX_RED CV_RGB(255,0,0)
#define CVX_BLUE CV_RGB(0,0,255)
#define CVX_GREEN CV_RGB(0,255,0)
#define CVX_WHITE CV_RGB(255,255,255)
#define CVX_BLACK CV_RGB(0,0,0)

#define SHOW_BG_REMOVAL 1  //<- switch from 0 to 1
// VARIABLES GLOBALES DE PROGRAMA //

#ifndef VARIABLES_GLOBALES_PROGRAMA
	#define VARIABLES_GLOBALES_PROGRAMA


	double GlobalTime ; // Medida de tiempos
	double InitialTime;
	CvCapture *g_capture ; //puntero a una estructura de tipo CvCapture
	static int hecho = 0;

	//HightGui
	int g_slider_position = 0;

	// Plato
	int PCentroX;
	int PCentroY;
	int PRadio;


	// Modelado de fondo
	int fr = 0;
	// Crea las imagenes que se usarán en el programa

	// CAPTURA
	// Imagenes RGB 3 canales
	IplImage* frame;
	IplImage *Imagen;
	IplImage *ImVisual;

	// PREPROCESADO
	// Imagenes 8 bits niveles de gris 1 canal

//	IplImage *ImGris;
//	IplImage *ImFilter;
	IplImage *ImROI;

	// Umbrales modelo de fondo estático 32 bit 1 canal
	IplImage *ImHiThr;
	IplImage *ImLowThr;

	// SEGMENTACION
	IplImage *ImBlobs;
	IplImage *ImThres;

	int spatialRad = 10, colorRad = 10 , maxPyrLevel = 1;

#endif

// DEFINICIÓN DE ESTRUCTURAS

#ifndef DEFINICION_DE_ESTRUCTURAS
#define DEFINICION_DE_ESTRUCTURAS

	// Capas
	typedef struct {
		IplImage* BGModel;  //BackGround Model
		IplImage* OldFG; //OldForeGround
		IplImage* FG;  //Foreground
		IplImage* ImFMask; // Mascara del plato
		IplImage* ImRois;
	}STCapas;

	STCapas* Capa = NULL;

	typedef struct Moscas{

			int etiqueta;
			float velocidad;
			float area;
			float VV,VH;
			CvPoint moment[8000];
			CvPoint2D32f punto1,punto2;

	}STMoscas;

#endif

//PROTOTIPOS DE FUNCIONES
//#ifndef PROTOTIPOS_DE_FUNCIONES
//#define PROTOTIPOS_DE_FUNCIONES


	// Crea las ventanas de visualización
	void CreateWindows();

	// localiza en memoria las imágenes necesarias para la ejecución
	void AllocateImages( IplImage* );

	// Busca el plato y crea la máscara
	void MascaraPlato(CvCapture*,IplImage*, int*, int*, int*);

	// Recibe la imagen del video y devuelve la imagen en un canal de niveles
	// de gris filtrada con el plato extraido
	void PreProcesado( IplImage* frame,IplImage* Im, IplImage* ImFMask, bool);

	// Para invertir mascaras
	void invertirBW( IplImage* Imagen);
	// Crea un modelo de fondo gaussiano usando la mediana. Establece los umbrales
	// de binarización.
	void initBackgroundModel(CvBGStatModel ** bgmodel, IplImage* tmp_frame,
							CvGaussBGStatModelParams* paramMoG);
	int BGGModel( CvCapture* t_capture, IplImage* BGModel, IplImage* IhiF,
			IplImage* IlowF, IplImage* ImMask);

	int CreateBlobs(IplImage* ROI,IplImage* blobs_img, STMoscas**, Lista );

	void mostrarLista(Lista);

	// Crea la capa de ROIS de cada objeto
	void CreateRois( IplImage*, IplImage*);

	// Limpia de la memoria las imagenes usadas durante la ejecución
	void DeallocateImages( void );

	//contiene los tipos de errores
	void error(int);

	// Función para obtener el número de frames en algunos S.O Linux
	int getAVIFrames( char* );

	// Crea una trackbar para posicionarse en puntos concretos del video
	void onTrackbarSlider(  int  );

	// destruye las ventanas
	void DestroyWindows( );

//#endif


#endif /* TRACKING_H_ */
