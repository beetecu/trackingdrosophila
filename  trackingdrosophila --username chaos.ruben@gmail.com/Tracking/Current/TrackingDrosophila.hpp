/*!
 * TrackingDrosophila.h
 *
 *  Created on: 19/07/2011
 *      Author: chao
 */

#ifndef TRACKING_H_
#define TRACKING_H_

//#include "VideoTracker.hpp"
//#include "Libreria.h"

#include "Inicializacion.hpp"
#include "Preprocesado.hpp"
#include "Procesado.hpp"
#include "Tracking.hpp"
#include "Stats.hpp"
#include "Visualizacion.hpp"

#define MAX_BUFFER IMAGE_BUFFER_LENGTH //!< máximo número de frames que almacenará la estructura.

//#define INTERVAL_BACKGROUND_UPDATE 10000


//PROTOTIPOS DE FUNCIONES
//#ifndef PROTOTIPOS_DE_FUNCIONES
//#define PROTOTIPOS_DE_FUNCIONES



//!\brief Inicialización: Crea las ventanas, inicializa las estructuras (excepto STFlies y STFrame que se inicia en procesado ),
//!asigna espacio a las imagenes, establece los parámetros del modelo de fondo y crea el fichero de datos.
/*!
 * \param frame Imagen fuente 8 bits en escala de grises.
 * \param Shape Estructura con los datos del modelo de forma.
 * \param BGParams Estructura que contiene los parámetros del modelado de fondo.
 * \param argc Argumento que contiene el video a cargar.
 * \param argv Argumento que contiene el video a cargar.
 *
 * \return Un 1 si todas las vetanas y estructuras han sido creadas correctamente.
 *
 * \see VideoTracker.hpp
 */

int Inicializacion( IplImage* frame,
			int argc,
			char* argv[],
			tlcde** FramesBuf,
			STStatFrame** Stats);

void onMouse( int event, int x, int y, int, void* );


//!\brief FinalizarTraking: Destruir todas las ventanas,imagenes,estructuras etc.

	void Finalizar(CvCapture **g_capture,CvVideoWriter**VWriter);


//!\brief onTrackbarSlider: Crea una trackbar para posicionarse en puntos concretos del video.
/*!
 * \return El frame correspondiente a la posición de la trackbar.
 */

	void onTrackbarSlider(  int  );


//#endif


#endif /* TRACKING_H_ */
