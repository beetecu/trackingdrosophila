/*
 * validacion.hpp
 *
 *  Created on: 22/09/2011
 *      Author: chao
 */

#ifndef VALIDACION_HPP_
#define VALIDACION_HPP_

#include "VideoTracker.hpp"
#include "segmentacion.hpp"
#include "BGModel.h"


/// Parametros validación

typedef struct {
	int UmbralProb; //!< establece cuantas desviaciones tipicas se puede alejar el area del area media sin que se considere inválido el blob
	int UmbralCirc; //!< Máxima circularidad a partir de la cual un blob se considerará no válido
	int MaxIncLTHIters; //!< Número de iteraciones en las que se incrementará el umbral bajo para dividir una elipse
	int MaxLowTH; //!< Máximo valor que alcanzará el umbral mínimo para dividir elipse
	float PxiMin; //!< Probabilidad mínima admisible a partir de la cual se deja de aumentar MaxLowTH
	int MaxDecLTHIters; //!< Número de iteraciones en las que se incrementará el umbral bajo para aumentar P(xi)
	int MinLowTH; //!< Mínimo valor que alcanzará el umbral bajo para aumentar P(xi)
	float Umbral_H;//!< Umbral Alto,que debe alcanzar la probailidad del blob para se considerado valido.
	float Umbral_L;//!< Umbral Bajo,que debe alcanzar la probailidad del blob para se considerado valido.
	float PxiMax;

}ValParams;

//!\brief Realiza la división y combinación de los componentes o blobs conectados, de la siguiente manera:
//!\n
//!\n Si la densidad o probabilidad a priori de una de elipse para un determinado blob es pequeña ( su area es grande)
//! este componente conectado en realidad pueden corresponder a varios blobs, que forma parte de una mosca,
//! o la detección de espurios.En este caso se aumenta el umbral ( por lo que aumentará la probabilidad)
//! hasta que la elipse es segementada en dos o mas componentes conectadas o hasta que la elipse alcanza una
//! probabilidad minima.
//!\n
//!\n Si si la densidad es demasiado pequeña debido a que el area de la elipse es pequeña entonces se
//! decrementa el umbral para aumentar el area hasta que la elipse se fusiona con elipses cercanas o
//! hasta que la elipse desaparezca.
/*!
 * \param Imagen Imagen fuente de 8 bits, contiene el frame actual.
 * \param FrameData Estructura que contiene las capas.
 * \param SH Estructuraque contiene los valores del modelado de la forma.
 * \param Segroi Establece la region de interes del Plat.
 * \param BGParams Contiene los parametros para el modelado de fondo.
 * \param VParams Parametros de validación.
 * \param Mask Mascara qe contiene el Foreground.
 *
 * \return Una Lista circular doblemente enlazada, con los blobs que han sido validados correctamente.
 *
 */

tlcde* Validacion(IplImage *Imagen, STFrame* FrameData, SHModel* SH,CvRect Segroi,BGModelParams* BGParams, ValParams* VParams,IplImage* Mask);

//!\brief Inicializa los Parametros para la validación.
/*!
 * \param Params Parametros de validación.
 */

void setValParams( ValParams* Params);

//!\brief Inicializada los Parametros del modelado de fondo.
/*!
 * \param Params Parametros para el modelado de fondo.
 */

void setBGModParams( BGModelParams* Params);

//!\brief Calcula la probabilidad total de todos los blos detectados en el frame.
/*!
 * \param Lista Lista con los dato los blobs.
 * \param SH Valores del modelado de forma.
 * \param VParams Parametros de validación.
 * \param FlyData Estructura que contiene los datos del blob.
 *
 * \return La probabilidad total para todos los blobs ( PX).
 *
 * \note Esta probabilidad se usa como criterio de validación.
 */

double CalcProbTotal(tlcde* Lista,SHModel* SH,ValParams* VParams,STFly* FlyData);

//!\brief Calcula la probabilidad del blob.
/*!
 * \param SH Valores del modelado de forma.
 * \param Flie Estructura que contiene los datos del blob.
 *
 * \return La probabilidad del blob ( Pxi ).
 *
 * \note Esta probabilidad debe estar dentro de unos umbrales para que el blob sea valiadado.
 */

double CalcProbMosca( SHModel* SH , STFly* Flie );

//!\brief Calcula la circlaridad de la mosca.
/*!
 * \param Flie Estructura que contiene los datos del blob.
 *
 * \return el valor de la circularidad del blob.
 *
 * \note Si el blob tiene mucha circularidad este no es valido, al poder tratarse de una sombra.
 *
 */

double CalcCircul( STFly* Flie);

//!\brief Establece si un blob no cumple las condiciones para ser validado, ya sea por un exceso de area
//!o por un defecto de area. Calcula tambien los umbrales establecidos como criterios de validación.
/*!
 * \param SH Valores del modelado de forma.
 * \param VParams VParams Parametros de validación.
 * \param Flie Estructura que contiene los datos del blob.
 *
 * \return Si la probabilidad es pequeña debido a un area excesiva devuelve un 1,por el contrario devuelve un 0;
 *
 * Ejemplo:
 *
 * \verbatim
	if ((area_blob - SH->FlyAreaMedia)<0)  return 0;
	else return 1;
	\endverbatim
 */

int CalcProbUmbral( SHModel* SH ,ValParams* VParams,STFly* Flie);

//!\brief Obtiene la maxima distancia de los pixeles al fonfo.
/*!
 * \param Imagen Imagen fuente de 8 bits de niveles de gris.
 * \param FrameData Estructura que contiene las capas.
 * \param Roi Región de interes perteneciente a cada blob.
 *
 * \return el valor maximo correspondiente a la maxima distancia de los pixels al fondo.
 *
 * \note Se utiliza como límite superior a la hora de aumentar el umbral.
 */
int ObtenerMaximo(IplImage* Imagen, STFrame* FrameData,CvRect Roi );

#endif /* VALIDACION_HPP_ */
