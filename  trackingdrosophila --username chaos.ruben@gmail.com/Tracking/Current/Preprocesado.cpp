/*
 * Preprocesado.cpp
 *
 *  Created on: 17/11/2011
 *      Author: chao
 */

#include "Preprocesado.hpp"

BGModelParams *BGParams = NULL;

int PreProcesado(CvCapture* g_capture, StaticBGModel** BGModel,SHModel** Shape ){
	struct timeval ti, tf, tif, tff; // iniciamos la estructura
	int TiempoParcial;
	int hecho = 0;
	extern float TiempoGlobal;
	extern double NumFrame;
	StaticBGModel* bgmodel;
	SHModel *shape;

	// Iniciar estructura para parametros del modelo de fondo en primera actualización
	BGModelParams *BGParams = NULL;
	BGParams = ( BGModelParams *) malloc( sizeof( BGModelParams));
	if ( !BGParams ) {error(4);return 0;}

	// Crear Modelo de fondo estático .Solo en la primera ejecución

	printf("Creando modelo de fondo..... ");
	gettimeofday(&ti, NULL);
	// establecer parametros
	InitialBGModelParams( BGParams );

	bgmodel = initBGModel( g_capture , BGParams );
	if(!bgmodel) return 0;

	gettimeofday(&tf, NULL);
	TiempoParcial= (tf.tv_sec - ti.tv_sec)*1000 + \
									(tf.tv_usec - ti.tv_usec)/1000.0;
	TiempoGlobal = TiempoGlobal + TiempoParcial ;
	printf(" %5.4g segundos\n", TiempoGlobal/1000);
	TiempoGlobal = 0; // inicializamos el tiempo global


	// Iniciar estructura para modelo de forma

	shape = ( SHModel *) malloc( sizeof( SHModel));
	if ( !shape ) {error(4);return 0;}
	shape->FlyAreaDes = 0;
	shape->FlyAreaMed = 0;
	shape->FlyAreaMedia=0;

	// Crear modelo de forma
	printf("Creando modelo de forma..... ");
	gettimeofday(&ti, NULL);

	ShapeModel( g_capture, shape , bgmodel->ImFMask, bgmodel->DataFROI );

	gettimeofday(&tf, NULL);
	TiempoParcial= (tf.tv_sec - ti.tv_sec)*1000 + \
									(tf.tv_usec - ti.tv_usec)/1000.0;
	TiempoGlobal= TiempoGlobal + TiempoParcial ;
	printf(" %5.4g seg\n", TiempoGlobal/1000);
	printf("Fin preprocesado. Iniciando procesado...\n");
	TiempoGlobal = 0;

	NumFrame = cvGetCaptureProperty( g_capture, 1 ); //Actualizamos los frames

	*BGModel = bgmodel;
	*Shape = shape;

	return hecho = 1;
}

void InitialBGModelParams( BGModelParams* Params){
	 static int first = 1;
	 if ( DETECTAR_PLATO ) Params->FLAT_FRAMES_TRAINING = 50;
	 else Params->FLAT_FRAMES_TRAINING = 0;
	 Params->FRAMES_TRAINING = 20;
	 Params->ALPHA = 0 ;
	 Params->MORFOLOGIA = 0;
	 Params->CVCLOSE_ITR = 1;
	 Params->MAX_CONTOUR_AREA = 200 ;
	 Params->MIN_CONTOUR_AREA = 5;
	 Params->HIGHT_THRESHOLD = 20;
	 Params->LOW_THRESHOLD = 10;

}

void releaseDataPreProcess(){
	free(BGParams);

}
