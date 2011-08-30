/*!
 * Tracking.h
 *
 *  Created on: 19/07/2011
 *      Author: chao
 */

#ifndef TRACKING_H_
#define TRACKING_H_

#include "VideoTracker.hpp"

#include <BlobResult.h>
#include "Interfaz_Lista.h"
#include "Libreria.h"



using namespace cv;
using namespace std;

// DEFINICIONES

//#define INTERVAL_BACKGROUND_UPDATE 10000


#define SHOW_BG_REMOVAL 1 ///<- switch from 0 to 1 para visualizar background
#define SHOW_VISUALIZATION 0 ///<- switch from 0 to 1 para visualizar resultado


// VARIABLES GLOBALES DE PROGRAMA
#ifndef VARIABLES_GLOBALES_PROGRAMA
	#define VARIABLES_GLOBALES_PROGRAMA


	double GlobalTime ; /// Medida de tiempos
	double InitialTime;

	int FrameCount = 0;
	CvCapture *g_capture ; /// puntero a una estructura de tipo CvCapture
	static int hecho = 0;

	//HightGui
	int g_slider_pos = 0;

	// Plato
	int PCentroX;
	int PCentroY;
	int PRadio;
	static CvRect DataFROI;

	// Modelado de fondo
	int fr = 0;
	int BGUpdate = 1;
	IplImage* BGTemp;
	IplImage* DETemp;

	/// Umbrales modelo de fondo estático 32 bit 1 canal
//	IplImage *ImHiThr;
//	IplImage *ImLowThr;

	/// Imagenes que se usarán en el programa principal
	/// CAPTURA
	/// Imagenes RGB 3 canales
	IplImage* frame;
	IplImage *Imagen;
	IplImage *ImVisual;

	/// PREPROCESADO
	/// Imagenes 8 bits niveles de gris 1 canal

//	IplImage *ImGris;
//	IplImage *ImFilter;
	IplImage *ImROI;



	/// SEGMENTACION
	IplImage *ImBlobs;
	IplImage *ImThres;

	int spatialRad = 10, colorRad = 10 , maxPyrLevel = 1;

#endif

// DEFINICIÓN DE ESTRUCTURAS

#ifndef DEFINICION_DE_ESTRUCTURAS
#define DEFINICION_DE_ESTRUCTURAS

	// Capas
	typedef struct {
		IplImage* BGModel;  ///BackGround Model
		IplImage* IDesv; /// Desviación tipica
		IplImage* OldFG; ///OldForeGround
		IplImage* FG;  ///Foreground
		IplImage* ImFMask; /// Mascara del plato
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


	/// Crea las ventanas de visualización
	void CreateWindows();

	/// localiza en memoria las imágenes necesarias para la ejecución
	void AllocateImages( IplImage* );

	/// Busca el plato y crea la máscara
	void MascaraPlato(CvCapture*,IplImage*, int*, int*, int*);

	/** Recibe la imagen del video y devuelve la imagen en un canal de niveles
	    de gris filtrada con el plato extraido **/
//	void PreProcesado( IplImage* src,IplImage* dst, IplImage* ImFMask, bool);

	// Para invertir mascaras
//	void invertirBW( IplImage* Imagen);
	/** Crea un modelo de fondo gaussiano usando la mediana. Establece los umbrales
	 de binarización. **/

	void AllocateImagesBGM( IplImage *I );

	int initBGGModel( CvCapture* t_capture, IplImage* BG,IplImage *DE, IplImage* ImMask);

	void UpdateBGModel(IplImage * tmp_frame,IplImage* BGModel,IplImage* DESVI, CvRect DataROI,IplImage* Mask );

	void BackgroundDifference( IplImage* ImGray, IplImage* bg_model,IplImage* Ides, IplImage* fg, CvRect dataroi );

	void DeallocateImagesBGM();


	int CreateBlobs(IplImage* ROI,IplImage* blobs_img, STMoscas**, Lista );

//	void mostrarLista(Lista);

	/// Crea la capa de ROIS de cada objeto
	void CreateRois( IplImage*, IplImage*);

	/// Limpia de la memoria las imagenes usadas durante la ejecución
	void DeallocateImages( void );

	/// Contiene los tipos de errores
	void error(int);

	/// Función para obtener el número de frames en algunos S.O Linux
	int getAVIFrames( char* );

	/// Crea una trackbar para posicionarse en puntos concretos del video
	void onTrackbarSlider(  int  );

	/// destruye las ventanas
	void DestroyWindows( );

//#endif


#endif /* TRACKING_H_ */
