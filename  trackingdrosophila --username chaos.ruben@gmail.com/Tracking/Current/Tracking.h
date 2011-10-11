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

#define STRUCT_BUFFER_LENGTH 50 /// máximo número de frames que almacenará la estructura
//#define INTERVAL_BACKGROUND_UPDATE 10000

// VARIABLES GLOBALES DE PROGRAMA
#ifndef VARIABLES_GLOBALES_PROGRAMA
	#define VARIABLES_GLOBALES_PROGRAMA

/// Ficheros

	char nombreFichero[30];


/// Medida de tiempos
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

	/// Modelo del Plato
	STFlat* Flat;

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
	STFlies* FlieTemp = NULL; /// Será la estructura creada en segmentación
	STFlies* Flie = NULL; /// Aquí se irán copiando los elementos creados en segmentación...
							// ...una vez que han sido validados.
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



	///! Inicialización: Crea las ventanas, inicializa las estructuras (excepto la STFlies, que se inicia en segmentación ),
	///! asigna espacio a las imagenes, establece los parámetros del modelo de fondo y crea el fichero de datos.
	int Inicializacion(IplImage* frame, STFlat** Flat,STCapas** Capa , SHModel** Shape, BGModelParams** BGParams,int argc, char* argv[]);
	/// Preprocesado

	int existe(char *nombreFichero);

	void crearFichero(char *nombreFichero );

	int PreProcesado( );

	void Procesado();

	void Tracking( );

	void Visualizacion();

	void mostrarLista(STFlies *Flie);

	void AnyadirFlies( STFlies** FlieTemp, STFlies **Flie);

	void AlmacenarDatos( STFlies* Flie, char* nombreFichero );

	void LiberarMemoria( STFlies** Flie );

	void AnalisisEstadistico();
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
