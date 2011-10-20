/*!
 * TrackingDrosophila.h
 *
 *  Created on: 19/07/2011
 *      Author: chao
 */

#ifndef TRACKING_H_
#define TRACKING_H_

#include "VideoTracker.hpp"
#include "Libreria.h"
#include "BGModel.h"
#include <BlobResult.h>
#include "segmentacion.h"
#include "ShapeModel.hpp"
#include "Plato.hpp"

#define STRUCT_BUFFER_LENGTH IMAGE_BUFFER_LENGTH /// máximo número de frames que almacenará la estructura. Impar
								/// para trabajar en el centro con 25 frames a cada lado.
//#define INTERVAL_BACKGROUND_UPDATE 10000

// VARIABLES GLOBALES DE PROGRAMA
#ifndef VARIABLES_GLOBALES_PROGRAMA
	#define VARIABLES_GLOBALES_PROGRAMA

	char nombreFichero[30];

	struct timeval ti, tf, tif, tff; // iniciamos la estructura
	float TiempoInicial;
	float TiempoParcial;
	float TiempoFrame;
	float TiempoGlobal = 0;

	double FrameCountAbs = 0; /// contador de frames absolutos ( incluyendo preprocesado )
	double FrameCountRel = 0; /// contador de frames relativos ( sin incluir preprocesado)
	double TotalFrames = 0;
	CvCapture *g_capture ; /// puntero a una estructura de tipo CvCapture

	///HightGui
	int g_slider_pos = 0;

	/// MODELADO DE FONDO
	StaticBGModel* BGModel = NULL;
	BGModelParams *BGParams = NULL;

	/// Estructura frame
	STFrame* FrameData = NULL;
	/// Buffer frames
	tlcde *FramesBuf = NULL;

	/// Estructura fly
	STFly* Fly = NULL;
	/// Lista flies
	tlcde *Flies = NULL;

	/// Modelo del Plato
	STFlat* Flat;

	// otros parámetros
	int fr = 0;
	int BGUpdate = 1;
	int UpdateCount = 0;
	IplImage* BGTemp;
	IplImage* DETemp;

	// Modelado de forma

	SHModel* Shape;

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



	///! Inicialización: Crea las ventanas, inicializa las estructuras (excepto STFlies y STFrame que se inicia en procesado ),
	///! asigna espacio a las imagenes, establece los parámetros del modelo de fondo y crea el fichero de datos.
	int Inicializacion(IplImage* frame,
			STFlat** Flat,
			SHModel** Shape,
			BGModelParams** BGParams,
			StaticBGModel** BGModel,
			int argc,
			char* argv[]);

	int PreProcesado( );

	void Procesado();

	void Tracking( );

	void Visualizacion();

	void AnalisisEstadistico();

	void Finalizar();

	void InitialBGModelParams( BGModelParams* Params);

	void InitNewFrameData(IplImage* I, STFrame *FrameData );



	/// Crea las ventanas de visualización
	void CreateWindows();

	/// localiza en memoria las imágenes necesarias para la ejecución
	void AllocateImages( IplImage*, StaticBGModel* bgmodel);

	void MotionTemplate( IplImage* img, IplImage* dst);

	void OpticalFlowLK( IplImage* ImLast, IplImage* velX, IplImage* velY);

	///
	int RetryCap();

	/// Crea una trackbar para posicionarse en puntos concretos del video
	void onTrackbarSlider(  int  );

	/// Limpia de la memoria las imagenes usadas durante la ejecución
	void DeallocateImages( void );

	/// destruye las ventanas
	void DestroyWindows( );

//#endif


#endif /* TRACKING_H_ */
