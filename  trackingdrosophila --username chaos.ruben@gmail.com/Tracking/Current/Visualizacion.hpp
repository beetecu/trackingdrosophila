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

typedef struct VisualParams{
	bool pause;
	bool stop;
	bool Grab;
}VParams;

/// Crea las ventanas de visualización
void CreateWindows();

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
void VisualizarEl( int pos, tlcde* frameBuf, StaticBGModel* Flat );

void VisualizarFr( STFrame* frameData, StaticBGModel* Flat );

void DefaultVParams( VParams **Parameters);

//!\brief ShowstatDataFr: Imprime en la visualización del frame los datos correspondientes a su procesado.
/*!
 * \param Im Imagen de 8 bits, donde se visualiza el frame actual.
 */

void ShowStatDataFr( IplImage* Im  );

void VerEstadoBuffer( IplImage* Imagen,int num );

void VerEstadoBGModel( IplImage* Imagen );

void visualizarId(IplImage* Imagen,CvPoint pos, int id , CvScalar color );
//!\brief //Representamos los blobs mediante triangulos isosceles
//! dibujamos triangulo isosceles de altura el eje mayor de la elipse, formando el segmento
//! (A,mcb), y de anchura el eje menor dando lugar al segmento (B,C), perpendicular
//! a (A,mcb) cuyo centro es mcb. La unión de A,B,C dará el triangulo resultante.
/*!
 * \param lista_blobs lista con los datos de los blobs a dibujar.
 */
void dibujarBlobs( IplImage* Imagen,tlcde* lista_blobs );

void visualizarBuffer( tlcde* Buffer,StaticBGModel* Flat, int posbuf );

#endif /* VISUALIZACION_HPP_ */