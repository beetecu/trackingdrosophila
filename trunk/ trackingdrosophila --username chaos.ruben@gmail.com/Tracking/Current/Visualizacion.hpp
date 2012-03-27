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

#define FLAT 1
#define BG_MODEL 2
#define SHAPE 3
#define TRAKING 4
#define CENTRAR cvPoint(-1,-1)
#define CENTRAR_SUP cvPoint(-2, -2 )
#define CENTRAR_INF cvPoint(-3, -3 )

typedef struct {
	bool pause;
	bool stop;
	bool Grab;
	int VisualPos;
	CvSize Resolucion;
	CvRect ROITracking;
	CvRect ROIPreProces;
	int DelayLogo;
	int BPrWidth; // ancho de la barra de progeso
}VisParams;

/// Crea las ventanas de visualización
void CreateWindows(IplImage* ImRef);

/// destruye las ventanas
void DestroyWindows( );

//- opciones en visualización:
//	- pause (p): realiza una pausa refrescando continuamente el frame actual
//				para continuar presionar "c"
//	- stop (s): detiene el procesado de frames y permite visualizar cada frame:
//		- siguiente frame (+)
//		- anterior frame (-)
//		- ultimo frame ( f )
//		- frame inicial ( i )
//		- grabar frame a fichero ( g )
//		- continuar (c) : continua con el procesado
void VisualizarEl( int pos, tlcde* frameBuf, StaticBGModel* Flat ,CvCapture* Cap,CvVideoWriter* Writer,VisParams* visParams);

void VisualizarFr( STFrame* frameData, StaticBGModel* Flat,CvVideoWriter* Writer, VisParams* visParams );

void DraWPresent( );

void DraWWindow( IplImage* ImRaw, StaticBGModel* Flat,CvVideoWriter* Writer,VisParams* params,tlcde* flies, STStatFrame* Stats ,int num );

void AllocDefaultVisParams(VisParams** visParams,IplImage* Image );

void SetVisParams(VisParams* visparams,IplImage* Image );

void Incrustar( IplImage* src1, IplImage* src2, IplImage* dst, CvRect ROI );

void DibujarFondo( VisParams* params);

void IncrustarTxt(VisParams* params,int num);

void IncrustarLogo(const char Cad[100], IplImage* ImLogos,CvPoint Origen,int Delay, bool Clear );

void Transicion( const char texto[],int delay_up, int delay_on, int delay_down);

void Transicion2( const char texto[],VisParams* params, int delay_up );

void Transicion3( const char texto[],VisParams* params, int delay_up );

void Transicion4(const char texto[],VisParams* params, int delay_down);

void desvanecer( IplImage* Imagen , int Delay,CvRect ROI );
//!\brief ShowstatDataFr: Imprime en la visualización del frame los datos correspondientes a su procesado.
/*!
 * \param Im Imagen de 8 bits, donde se visualiza el frame actual.
 */

void ShowStatDataFr( STStatFrame* Stats, IplImage* ImVisual,VisParams* visParams);

void VerEstadoBuffer( IplImage* Imagen,int num, VisParams* params, int max );

void VerEstadoBGModel( IplImage* Imagen );


//!\brief //Representamos los blobs mediante triangulos isosceles
//! dibujamos triangulo isosceles de altura el eje mayor de la elipse, formando el segmento
//! (A,mcb), y de anchura el eje menor dando lugar al segmento (B,C), perpendicular
//! a (A,mcb) cuyo centro es mcb. La unión de A,B,C dará el triangulo resultante.
/*!
 * \param lista_blobs lista con los datos de los blobs a dibujar.
 */
void dibujarBlobs( IplImage* Imagen,tlcde* lista_blobs );

void visualizarBuffer( tlcde* Buffer,StaticBGModel* Flat, VisParams *Params,CvVideoWriter* writer );

void releaseVisParams( VisParams *Parameters);
#endif /* VISUALIZACION_HPP_ */
