/*
 * AsignarIdentidades.hpp
 *
 *  Created on: 02/09/2011
 *      Author: chao
 */

#ifndef ASIGNARIDENTIDADES_HPP_
#define ASIGNARIDENTIDADES_HPP_

#include "VideoTracker.hpp"
#include "opencv2/video/tracking.hpp"
#include "Libreria.h"

void MotionTemplate( tlcde* framesBuf );

///! brief Realiza la asignación de identidades. El primer parámetro es la lista
///! el segundo parámetro es un flag que indica qué individuos asignar. 1 para los
///! del foreground ( estado dinámico )  y 0 para los del oldforeground ( estado estático )
tlcde* matchingIdentity( tlcde* framesBuf ,int estado );

void releaseMotionTemplate();

#endif /* ASIGNARIDENTIDADES_HPP_ */
