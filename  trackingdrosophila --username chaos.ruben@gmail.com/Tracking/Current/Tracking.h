/*!
 * Tracking.h
 *
 *  Created on: 19/07/2011
 *      Author: chao
 */

#ifndef TRACKING_H_
#define TRACKING_H_

#include "VideoTracker.hpp"
#include "BGModel.h"
#include <time.h>
#include <sys/time.h>
#include <BlobResult.h>
#include "Interfaz_Lista.h"
#include "segmentacion.h"
#include "ShapeModel.hpp"
#include "Plato.hpp"

using namespace cv;
using namespace std;

// DEFINICIONES
#ifndef CLK_TCK
#define CLK_TCK CLOCKS_PER_SEC
#endif
//#define INTERVAL_BACKGROUND_UPDATE 10000

// VARIABLES GLOBALES DE PROGRAMA
#ifndef VARIABLES_GLOBALES_PROGRAMA
	#define VARIABLES_GLOBALES_PROGRAMA

/// Medida de tiempos
struct timeval ti, tf, tif, tff; // iniciamos la estructura

	float TiempoInicial;
	float TiempoParcial;
	float TiempoFrame;
	float TiempoGlobal = 0;

	double FrameCount = 0;
	double TotalFrames = 0;
	CvCapture *g_capture ; /// puntero a una estructura de tipo CvCapture

	///HightGui
	int g_slider_pos = 0;

	/// Modelo del Plato
	STFlat* Flat;
	static CvRect SegROI;

	/// MODELADO DE FONDO

	STCapas* Capa = NULL;
	BGModelParams *BGParams = NULL;

	// otros parámetros
	int fr = 0;
	int BGUpdate = 1;
	int UpdateCount = 0;
	IplImage* BGTemp;
	IplImage* DETemp;
	IplImage* BGTemp1;
	IplImage* DETemp1;


	// Modelado de forma

	SHModel* Shape;

	///Estructura Flies
	STFlies* Flie=NULL;

	/// SEGMENTACION
	int Nc; ///Numero de contornos devueltos por segmentacion

	/// Imagenes que se usarán en el programa principal
	/// CAPTURA
	/// Imagenes RGB 3 canales
	IplImage* frame;
	IplImage *Imagen;
	IplImage *ImVisual;

	/// TRACKING
	IplImage *ImOpFlowX;
	IplImage *ImOpFlowY;

#endif

//PROTOTIPOS DE FUNCIONES
//#ifndef PROTOTIPOS_DE_FUNCIONES
//#define PROTOTIPOS_DE_FUNCIONES

	/// Inicialización

	int Inicializacion(IplImage* frame, STFlat** Flat,STCapas** Capa , SHModel** Shape, BGModelParams** BGParams);
	/// Preprocesado

	int PreProcesado( CvCapture*g_capture, STFlat* Flat,STCapas*  Capa,SHModel* Shape);

	void Procesado();
	/// Medida de tiempos
	int gettimeofday( struct timeval *tv, struct timezone *tz );

	/// Crea las ventanas de visualización
	void CreateWindows();

	/// localiza en memoria las imágenes necesarias para la ejecución
	void AllocateImages( IplImage*, STCapas* Capa);

	void InitialBGModelParams( BGModelParams* Params);

	// Para invertir mascaras
//	void invertirBW( IplImage* Imagen);
	/** Crea un modelo de fondo gaussiano usando la mediana. Establece los umbrales
	 de binarización. **/

	void MotionTemplate( IplImage* img, IplImage* dst);

	void OpticalFlowLK( IplImage* ImLast, IplImage* velX, IplImage* velY);

	int CreateBlobs(IplImage* ROI,IplImage* blobs_img, STFlies**, Lista );

//	void mostrarLista(Lista);

	/// Crea la capa de ROIS de cada objeto
	void CreateRois( IplImage*, IplImage*);

	/// Limpia de la memoria las imagenes usadas durante la ejecución
	void DeallocateImages( void );

	/// Función para obtener el número de frames en algunos S.O Linux
	int getAVIFrames( char* );

	/// Crea una trackbar para posicionarse en puntos concretos del video
	void onTrackbarSlider(  int  );

	/// destruye las ventanas
	void DestroyWindows( );

//#endif


#endif /* TRACKING_H_ */
