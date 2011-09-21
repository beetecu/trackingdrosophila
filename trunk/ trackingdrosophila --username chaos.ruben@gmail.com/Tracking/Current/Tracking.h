/*!
 * Tracking.h
 *
 *  Created on: 19/07/2011
 *      Author: chao
 */

#ifndef TRACKING_H_
#define TRACKING_H_

#include <time.h>
#include "VideoTracker.hpp"
#include <sys/time.h>
#include <BlobResult.h>
#include "Interfaz_Lista.h"
#include "Libreria.h"



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


	//HightGui
	int g_slider_pos = 0;

	// Plato
	int PCentroX;
	int PCentroY;
	int PRadio;
	static CvRect DataFROI;
	static CvRect SegROI;

	// Modelado de fondo
	int fr = 0;
	int BGUpdate = 1;
	int UpdateCount = 0;
	IplImage* BGTemp;
	IplImage* FOTemp;
	IplImage* DETemp;

	// Modelado de forma

	int FlyAreaMed, FlyAreaDes;

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

	/// Sombras
	IplImage* ImPyr;



	/// SEGMENTACION
	IplImage *ImBlobs;
	IplImage *ImThres;

	/// TRACKING
	IplImage *ImOpFlowX;
	IplImage *ImOpFlowY;

	int spatialRad = 10, colorRad = 10 , maxPyrLevel = 1;

#endif

// DEFINICIÓN DE ESTRUCTURAS

#ifndef DEFINICION_DE_ESTRUCTURAS
#define DEFINICION_DE_ESTRUCTURAS

	// Capas
	typedef struct {
		IplImage* BGModel;  ///BackGround Model
		IplImage* IDesv;
		IplImage* OldFG; ///OldForeGround ( objetos estáticos )
		IplImage* FGTemp; /// Imagen a segmentar y validar
		IplImage* FG;  ///Foreground ( objetos en movimiento )
		IplImage* ImFMask; /// Mascara del plato
		IplImage* ImRois;
		IplImage* ImMotion;
	}STCapas;

	STCapas* Capa = NULL;

	typedef struct Moscas{

			int etiqueta;
			CvPoint posicion;
			float area;
			float orientacion;
			CvRect DataRoi;
			float velocidad;
			float VV,VH;
			CvPoint moment[8000];
			CvPoint2D32f punto1,punto2;

	}STMoscas;

#endif

//PROTOTIPOS DE FUNCIONES
//#ifndef PROTOTIPOS_DE_FUNCIONES
//#define PROTOTIPOS_DE_FUNCIONES

	/// Medida de tiempos
	int gettimeofday( struct timeval *tv, struct timezone *tz );

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

	void initBGGModel( CvCapture* t_capture, IplImage* BG,IplImage *DE, IplImage* ImMask, CvRect ROI);

	void ShapeModel( CvCapture* g_capture, int* FlyAreaMed, int* FlyAreaDes, IplImage* ImMask,CvRect ROI );

	void UpdateBGModel(IplImage * tmp_frame,IplImage* BGModel,IplImage* DESVI, CvRect DataROI,IplImage* Mask );

	void BackgroundDifference( IplImage* ImGray, IplImage* bg_model,IplImage* Ides, IplImage* fg, CvRect dataroi );

	void segmentacion(IplImage *Brillo,IplImage *mediana,IplImage *desviacion,IplImage *Foreg,CvRect Segroi);

	void MotionTemplate( IplImage* img, IplImage* dst);

	void OpticalFlowLK( IplImage* ImLast, IplImage* velX, IplImage* velY);



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
