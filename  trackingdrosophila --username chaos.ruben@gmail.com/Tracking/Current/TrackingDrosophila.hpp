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

#define STRUCT_BUFFER_LENGTH IMAGE_BUFFER_LENGTH /// máximo número de frames que almacenará la estructura. Impar
								/// para trabajar en el centro con 25 frames a cada lado.
//#define INTERVAL_BACKGROUND_UPDATE 10000


//PROTOTIPOS DE FUNCIONES
//#ifndef PROTOTIPOS_DE_FUNCIONES
//#define PROTOTIPOS_DE_FUNCIONES



	///! Inicialización: Crea las ventanas, inicializa las estructuras (excepto STFlies y STFrame que se inicia en procesado ),
	///! asigna espacio a las imagenes, establece los parámetros del modelo de fondo y crea el fichero de datos.
	int Inicializacion(IplImage* frame,
			SHModel** Shape,
			BGModelParams** BGParams,
			int argc,
			char* argv[]);

	int PreProcesado( );

	///! Imprime en el frame datos del procesado para visualizacion
	void visualizarDatos( IplImage* Im  );

	void AnalisisEstadistico();

	void FinalizarTracking();

	void InitialBGModelParams( BGModelParams* Params);

	/// Reintenta la captura del frame
	int RetryCap();

	/// Crea una trackbar para posicionarse en puntos concretos del video
	void onTrackbarSlider(  int  );


//#endif


#endif /* TRACKING_H_ */
