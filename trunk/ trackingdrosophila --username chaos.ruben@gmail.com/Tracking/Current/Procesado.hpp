/*
 * Procesado.hpp
 *
 *  Created on: 20/10/2011
 *      Author: chao
 */

#ifndef PROCESADO_HPP_
#define PROCESADO_HPP_

#include "VideoTracker.hpp"
#include "Libreria.h"
#include "BGModel.h"
#include "segmentacion.hpp"
#include "validacion.hpp"
#include "segmentacion.hpp"
#include "Visualizacion.hpp"
#include <opencv2/video/background_segm.hpp>
//!\brief Procesado: - Obtención del foreground mediante modelo de fondo mediana  :
//!\n 1. Actualización de fondo y resta de fondo obteniendo el foreground.
//!\n 2. Nueva actualización de fondo usando la máscacara de foreground obtenida.
//!\n Resta de fondo
//!\n Redefinición de la máscara de foreground mediante ajuste por elipses y
//!\n obtención de los parámetros de los blobs en segmentación.

//!\n -Rellena la lista lineal doblemente enlazada ( Flies )con los datos de cada uno de los blobs.
//!\n -Rellena la estructura FrameData con las nuevas imagenes y la lista Flies.
//!\n -Finalmente añade la lista Flies a la estructura FrameData.
//!\n y la estructura FrameData al buffer FramesBuf  y la estructura FrameData al buffer FramesBuf.
/*!
 * \param frame Imagen fuente de 8 bits que contiene el frame actual.
 * \param framesBuf Buffer que contiene la lista de los frames con las moscas validadas.
 * \param BGModel Estructura que contiene los parametros para el modelado de fondo.
 * \param Shape Estructura que contiene lo valores del modelado de forma.
 *
 * \return Un Buffer que contiene la lista con los blobs validados en cada frame y rellena la estructura FrameData con las nuevas imagenes y la lista Flies.
 *
 *
 * \see VideoTracker.hpp
 */
STFrame* Procesado( IplImage* frame, StaticBGModel* BGModel,SHModel* Shape );

//!\brief Procesado2: - Obtención del foreground mediante modelo de fondo mediana
//!\n con actualización selectiva en tres etapas :
//!\n 1. Actualización de fondo.
//!\n 2. resta de fondo obteniendo el foreground.
//!\n Redefinición de la máscara de foreground mediante ajuste por elipses y
//!\n obtención de los parámetros de los blobs en segmentación.
//!\n 3. Actualización selectiva mediante uso de máscara de foreground.

//!\n -Rellena la lista lineal doblemente enlazada ( Flies )con los datos de cada uno de los blobs.
//!\n -Rellena la estructura FrameData con las nuevas imagenes y la lista Flies.
//!\n -Finalmente añade la lista Flies a la estructura FrameData.
//!\n y la estructura FrameData al buffer FramesBuf  y la estructura FrameData al buffer FramesBuf.
/*!
 * \param frame Imagen fuente de 8 bits que contiene el frame actual.
 * \param framesBuf Buffer que contiene la lista de los frames con las moscas validadas.
 * \param BGModel Estructura que contiene los parametros para el modelado de fondo.
 * \param Shape Estructura que contiene lo valores del modelado de forma.
 *
 * \return Un Buffer que contiene la lista con los blobs validados en cada frame y rellena la estructura FrameData con las nuevas imagenes y la lista Flies.
 *
 * \see VideoTracker.hpp
 */

STFrame* Procesado2( IplImage* frame, StaticBGModel* BGModel,SHModel* Shape );

///! Igual que procesado uno pero con modelo de fondo gaussiano simple
STFrame* Procesado3( IplImage* frame, StaticBGModel* BGModel,SHModel* Shape );
///! Igual que procesado2 pero con modelo de fondo gaussiano simple
STFrame* Procesado4( IplImage* frame,StaticBGModel* BGModel, CvBGStatModel* bg_model,SHModel* Shape );

STFrame* InitNewFrameData(IplImage* I );
//!\brief InitNewFrameData:  Inicializa las imagenes de la estructura FrameData y la Lista Flies.
/*!
 * \param I Imagen fuente de 8 bits.
 * \param FrameData Estructura que contiene las imagenes que cada capa.
 */

void InitNewFrameData(IplImage* I, STFrame *FrameData );

//!\brief putBGModelParams: Restablece los parametros para el modelado de fondo.
/*!
 * \param Params Estructura que contiene los parametros para el modelado de fondo.
 */

void putBGModelParams( BGModelParams* Params);

void AllocateDataProcess( IplImage *I );

void releaseDataProcess();

#endif /* PROCESADO_HPP_ */
