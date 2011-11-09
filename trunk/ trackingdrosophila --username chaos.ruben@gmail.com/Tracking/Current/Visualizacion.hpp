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
/// Crea las ventanas de visualización
void CreateWindows();

/// destruye las ventanas
void DestroyWindows( );

void VisualizarEl( int pos, tlcde* frameBuf, StaticBGModel* Flat );

void VerEstadoBuffer( IplImage* Imagen,int num );

void VerEstadoBGModel( IplImage* Imagen );

void visualizarId(IplImage* Imagen,CvPoint pos, int id , CvScalar color );

#endif /* VISUALIZACION_HPP_ */
