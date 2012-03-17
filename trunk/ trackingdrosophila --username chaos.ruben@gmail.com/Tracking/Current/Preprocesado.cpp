/*
 * Preprocesado.cpp
 *
 *  Created on: 17/11/2011
 *      Author: chao
 */

#include "Preprocesado.hpp"

BGModelParams *BGParams = NULL;

int PreProcesado(char* nombreVideo, StaticBGModel** BGModel,SHModel** Shape ){
	struct timeval ti,tif; // iniciamos la estructura
	float TiempoParcial;
	float TiempoTotal;
	int hecho = 0;

	CvCapture* capture;

	StaticBGModel* bgmodel;
	SHModel *shape;
#ifdef	MEDIR_TIEMPOS
	gettimeofday(&tif, NULL);
	gettimeofday(&ti, NULL);
#endif
	printf("\nIniciando preprocesado...");

	//captura
	capture = cvCaptureFromAVI( nombreVideo );
		if ( !capture ) {
			error( 1 );
			help();
			return -1;
		}
	// Iniciar estructura para parametros del modelo de fondo en primera actualización

	BGParams = ( BGModelParams *) malloc( sizeof( BGModelParams));
	if ( !BGParams ) {error(4);return 0;}

	// Crear Modelo de fondo estático
	printf("\n\t1)Creando modelo de Fondo..... ");
	// establecer parametros
	InitialBGModelParams( BGParams );
	bgmodel = initBGModel( capture , BGParams );
	if(!bgmodel) {error(8); return 0;}
	TiempoParcial = obtenerTiempo( ti , 1);
	printf("Hecho. Tiempo empleado: %0.2f seg\n", TiempoParcial);

	// Crear modelo de forma

	printf("\t2)Creando modelo de forma..... ");
#ifdef	MEDIR_TIEMPOS
	gettimeofday(&ti, NULL);
#endif
//	ShapeModel( capture, shape , bgmodel->ImFMask, bgmodel->DataFROI );
	shape = ShapeModel2( capture, bgmodel );
	if(!shape) {error(9); return 0;}
#ifdef	MEDIR_TIEMPOS
	TiempoParcial = obtenerTiempo( ti , 1 );
	printf("Hecho. Tiempo empleado: %0.2f seg\n", TiempoParcial);
#endif
	*BGModel = bgmodel;
	*Shape = shape;

	cvReleaseCapture(&capture);
#ifdef	MEDIR_TIEMPOS
	TiempoTotal = obtenerTiempo( tif , 1);
	printf("\nPreprocesado correcto.Tiempo total empleado: %0.2f s\n", TiempoTotal);
#endif
	//	cvDestroyAllWindows();
	return 1;
}

void InitialBGModelParams( BGModelParams* Params){

	 if ( DETECTAR_PLATO ) Params->FLAT_FRAMES_TRAINING = 5;
	 else Params->FLAT_FRAMES_TRAINING = 0;
	 Params->FRAMES_TRAINING = 200;
	 Params->ALPHA = 0 ;
	 Params->INITIAL_DESV = 0.05;

}

void releaseDataPreProcess(){
	free(BGParams);

}