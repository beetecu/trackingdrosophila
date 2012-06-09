/*
 * Visualizacion.hpp
 *
 *  Created on: 27/10/2011
 *      Author: chao
 */

#ifndef VISUALIZACION_HPP_
#define VISUALIZACION_HPP_

#include "VideoTracker.hpp"
#include "Libreria.h"

#include <opencv2/video/tracking.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <iostream>
#include <ctype.h>
#include <stdio.h>

// VISUALIZACIÓN DE IMAGENES.Definición de estados
#define MODE 0
#define SIMPLE 0
#define COMPLETO 1
#define SHOW_PRESENT 0 //!<  presentacion
//// Depuración preprocesado
#define SHOW_LEARNING_FLAT 1
#define SHOW_INIT_BACKGROUND 2
#define SHOW_SHAPE_MODELING 3//!<  modelado de forma.
//
//// Depuración procesado
#define SHOW_PROCESS_IMAGES 4 //!<  procesado etapa a etapa.
#define SHOW_BG_DIF_IMAGES 5 //!< imagenes antes y despues de la limpieza de FG.
#define SHOW_VALIDATION_IMAGES 6//!< imagenes de la validación etapa a etapa.
//
#define SHOW_BG_REMOVAL 7//!< Background y Foreground.
#define SHOW_KALMAN 8


//#define SHOW_WINDOW 9 //!<  Resultado
#define FLAT 10
#define BG_MODEL 11
#define SHAPE 12
#define TRAKING 13

#define SHOW_STATS_MOV 14
#define TOTAL_FRAMES 15

#define CENTRAR cvPoint(-1,-1)
#define CENTRAR_SUP cvPoint(-2, -2 )
#define CENTRAR_INF cvPoint(-3, -3 )

// parametros para visualizar estadisticas de blobs

#define MAX_COLS 8 // numero maximo de columnas
#define MAX_FILS 2 // número máximo de filas
#define MAX_ELEMENTS MAX_COLS*MAX_FILS // número máximo de tracks a visualizar

typedef struct {


	int margenBorde;
	int margenSup;
	int margenInterno;


	CvPoint OrProgres;
	CvPoint FnProgres;
	CvPoint OrImage;
	CvPoint OrFrameStats;
	CvPoint OrStats;
	CvPoint OrGraphic1;
	CvPoint FnGraphic1;
	CvPoint OrGraphic2;
	CvPoint FnGraphic2;
	CvPoint OrDataTrack;
	CvPoint FnImage;
	CvPoint FnFrameStats;
	CvPoint FnStats;
	CvPoint FnDataTrack;

}posBlocks;

typedef struct founts{
	CvFont fuente1; // statdataframe y statDatablobs
	CvFont fuente2;
	CvFont fuente3; // se usan en las transiciones y en incrustar txt
	CvFont fuente4;
}Fuentes;

typedef struct {
	// Public
	int RecWindow;						/// Switch GENERAL de 0 a 1 para activar la grabación de la ventana de visualización
	int ShowWindow ; 					/// Switch GENERAL de 0 a 1 para mostrar la ventana de visualización
											/// NOTA:Los siguientes parámetros no se tendrán en cuenta si no se activa
	int ShowPresent  ;				/// Switch de 0 a 1 para visualizar presentacion inicial.
	int ShowTransition ;				/// Muestra una animaciones entre las distintas partes del proceso.
	int HightGUIControls  ;			/// Activa controles de visualización:
											/// Mientras se ejecuta el programa, si se pulsa:
											/*! 	p : se pone en modo pausa hasta presionar c.
						 				 	r : Inicia la grabación de un video con los resultados.
						 					s : Detiene la grabación del video
						 					f : Avanza frame a frame hasta presionar c.
						 					c : Permite continuar con la ejecución normal.
											g : Hace una captura de pantalla ( solo funciona en modo pausa )
												Se guarda en formato jpeg.!*/
	int ModoCompleto ;				/// Switch GENERAL true/false para visualizar los resultados de los distintos módulos.
											/// NOTA1: Al activar esta opción el rendimiento disminuirá de forma notable.
											/// NOTA2: Los siguientes parámetros no se tendrán en cuenta si no se activa 									// 	  	  el modo detallado.

	int	ShowBGdiffImages ; 		/// Switch de 0 a 1 para visualizar las imagenes antes y despues de la limpieza de FG.

		// Resultados de preprocesado
	int	ShowLearningFlat ;
	int	ShowInitBackground ;
	int	ShowShapeModel ; 				/// Switch true/false para visualizar los resultados del modelado de forma.

		// Resultados de procesado
	int	ShowProcessPhases ; 		/// Switch true/false para visualizar los resultados del procesado etapa a etapa.
	int	ShowValidationPhases ;		/// Switch true/false para visualizar las imagenes de la validación etapa a etapa.
	int	ShowBGremoval ; 				/// Switch true/false para visualizar el Background y Foreground.
	int	ShowKalman ;

	// ventana de tracking
	int ShowStatsMov;

	int zoom;				/// ampliación de la imagen del blob mostrada en ventanas sin track

	//Private
	int TotalFrames;
	bool pause;
	bool stop;
	bool pasoApaso;
	int VisualPos;
	CvSize Resolucion;
	CvRect ROITracking;
	CvRect ROIPreProces;
	int DelayLogo;
	int DelayUp;
	int DelayDown;
	int DelayTr;
	int BPrWidth; // ancho de la barra de progeso


}VisParams;

/// Crea las ventanas de visualización e iniciar parámetros
void CreateWindows(IplImage* ImRef);

void SetHightGUIParams(  IplImage* ImRef,char* nombreVideo, double FPS, int TotalFrames );

void SetDefaultHightGUIParams(  IplImage* ImRef );

void SetPrivateHightGUIParams(  IplImage* ImRef, int TotalFrames );

int obtenerVisParam( int type );

void ShowHightGUIParams( char* Campo );

/// PRESENTACIÓN INICIO ///

void DraWPresent( );

void IncrustarLogo(const char Cad[100], IplImage* ImLogos,CvPoint Origen,int Delay, bool Clear );

/// ANIMACIONES ///

void Transicion( const char texto[],int delay_up, int delay_on, int delay_down);

void Transicion2( const char texto[] , int delay_up );

void Transicion3( const char texto[] , int delay_up );

void Transicion4(const char texto[], int delay_down);

void desvanecer( IplImage* Imagen , int Delay );

/// VISUALIZACIÓN RESULTADOS ///

void DraWWindow(IplImage* frame, STFrame* FrameDataOut, StaticBGModel* BGModel, int type, int Mode );

void DrawPreprocesWindow( IplImage* frame);

void DrawTrackingWindow( IplImage* frame, STFrame* FrameDataOut, StaticBGModel* BGModel );

CvVideoWriter* iniciarAvi(  char* nombreVideo, double fps);

/*!\brief Incrusta src2 en src1 en zona indicada por roi
 *
 * @param src1
 * @param src2
 * @param dst
 * @param ROI
 */

void Incrustar( IplImage* src1, IplImage* src2, IplImage* dst, CvRect ROI );

/*!\brief Incrusta zona indicada por roi de src2 en src1. La roi ha de ser de igual tamaño al de la imagen src1
 *
 * @param src1
 * @param src2
 * @param dst
 * @param ROI
 */
void Incrustar2( IplImage* src1, IplImage* src2, IplImage* dst, CvRect ROI );

void DibujarFondo( );

//!\brief //Representamos los blobs mediante triangulos isosceles
//! dibujamos triangulo isosceles de altura el eje mayor de la elipse, formando el segmento
//! (A,mcb), y de anchura el eje menor dando lugar al segmento (B,C), perpendicular
//! a (A,mcb) cuyo centro es mcb. La unión de A,B,C dará el triangulo resultante.
/*!
 * \param lista_blobs lista con los datos de los blobs a dibujar.
 */
void dibujarBlobs( IplImage* Imagen,tlcde* lista_blobs );

void IncrustarTxt( int num );

void setFounts();

void setPosBlocks( IplImage * ImRef);

//!\brief ShowstatDataFr: Imprime en la visualización del frame los datos correspondientes a su procesado.
/*!
 * \param Im Imagen de 8 bits, donde se visualiza el frame actual.
 */

void ShowStatDataFr( STStatFrame* Stats,STGlobStatF* GStats, IplImage* ImVisual);

void ShowStatDataBlobs( tlcde* Flies, tlcde* Tracks );

void VerEstadoBuffer( IplImage* Imagen,int num, int max );

void VerEstadoBGModel( IplImage* Imagen );

/// DEPURACIÓN ///

//- opciones en visualización para depurar:
//	- pause (p): realiza una pausa refrescando continuamente el frame actual
//				para continuar presionar "c"
//	- stop (s): detiene el procesado de frames y permite visualizar cada frame:
//		- siguiente frame (+)
//		- anterior frame (-)
//		- ultimo frame ( f )
//		- frame inicial ( i )
//		- grabar frame a fichero ( g )
//		- continuar (c) : continua con el procesado
void VisualizarEl( tlcde* frameBuf, int pos,  StaticBGModel* Flat );

void VisualizarFr( STFrame* frameData, StaticBGModel* Flat );

void visualizarBuffer( tlcde* Buffer,StaticBGModel* Flat );

/// LIBERAR MEMORIA ///

void releaseVisParams( );
void DestroyWindows( );

#endif /* VISUALIZACION_HPP_ */
