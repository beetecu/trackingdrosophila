/*
 * Preprocesado.hpp
 *
 *  Created on: 17/11/2011
 *      Author: chao
 */

#ifndef PREPROCESADO_HPP_
#define PREPROCESADO_HPP_

#include "VideoTracker.hpp"
#include "Libreria.h"
#include "ShapeModel.hpp"
#include "BGModel.h"

//!\brief Preprocesado: crea el modelo de fondo estático,solo se ejecuta una vez.
/*!
 * \return Un 1 o True si el procesado se ha realizado correctamente.
 */

int PreProcesado(char* nombreVideo, StaticBGModel** BGModel,SHModel** Shape );

//!\brief SetPreProcesParams: Inicializar los parametros necesarios realizar el modelado de fondo y de forma.

void SetPreProcesParams(  );

void DefaultInitBGModelParams( );

void SetPrivatePrPcParams(  );

void ShowInitBGModelParams( char* Campo );

void releaseDataPreProcess();


#endif /* PREPROCESADO_HPP_ */
