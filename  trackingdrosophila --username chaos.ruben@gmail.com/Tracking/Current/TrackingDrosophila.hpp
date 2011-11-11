/*!
 * TrackingDrosophila.h
 *
 *  Created on: 19/07/2011
 *      Author: chao
 */

#ifndef TRACKING_H_
#define TRACKING_H_

#include "VideoTracker.hpp"
#include "Libreria.h"
#include "BGModel.h"
#include <BlobResult.h>
#include "segmentacion.hpp"
#include "ShapeModel.hpp"
#include "Tracking.hpp"
#include "Procesado.hpp"
#include "Visualizacion.hpp"

#define STRUCT_BUFFER_LENGTH IMAGE_BUFFER_LENGTH //!< máximo número de frames que almacenará la estructura.

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

	int Inicializacion(IplImage* frame,
			SHModel** Shape,
			BGModelParams** BGParams,
			int argc,
			char* argv[]);

//!\brief Preprocesado: crea el modelo de fondo estático,solo se ejecuta una vez.
/*!
 * \return Un 1 o True si el procesado se ha realizado correctamente.
 */

	int PreProcesado( );

//!\brief ShowstatDataFr: Imprime en la visualización del frame los datos correspondientes a su procesado.
/*!
 * \param Im Imagen de 8 bits, donde se visualiza el frame actual.
 */

	void ShowStatDataFr( IplImage* Im  );

	void AnalisisEstadistico();

//!\brief FinalizarTraking: Destruir todas las ventanas,imagenes,estructuras etc.

	void FinalizarTracking();

//!\brief InitialBGModelParams: Inicializar los parametros necesarios realizar el modelado de fondo.
/*!
 * \param Params Estructura que contiene los parámetros necesarios para el modelado de fondo.
 *
 * \return Los parátros para un correcto modelado de fondo.
 */

	void InitialBGModelParams( BGModelParams* Params);

//!\brief RetryCap: Reintenta la captura del frame, en caso de fallo de captura.
/*!
 * \return Un 1 o True si el frame ha sido capturado correctamente.
 *
 * Ejemplo:
 * \verbatim
  		if ( !frame ) {
			error(2);
			return 0;
		}
		else return 1;
	}
	else return 1;
	\endverbatim
 */

	int RetryCap();

//!\brief onTrackbarSlider: Crea una trackbar para posicionarse en puntos concretos del video.
/*!
 * \return El frame correspondiente a la posición de la trackbar.
 */

	void onTrackbarSlider(  int  );


//#endif


#endif /* TRACKING_H_ */
