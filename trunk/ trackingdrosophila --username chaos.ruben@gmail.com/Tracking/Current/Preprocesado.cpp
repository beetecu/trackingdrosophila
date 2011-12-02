/*
 * Preprocesado.cpp
 *
 *  Created on: 17/11/2011
 *      Author: chao
 */

#include "Preprocesado.hpp"

BGModelParams *BGParams = NULL;

int PreProcesado(CvCapture* g_capture, StaticBGModel** BGModel,SHModel** Shape ){
	struct timeval ti,tif; // iniciamos la estructura
	float TiempoParcial;
	int hecho = 0;

	extern double NumFrame;
	StaticBGModel* bgmodel;
	SHModel *shape;


	gettimeofday(&ti, NULL);
	// Iniciar estructura para parametros del modelo de fondo en primera actualización
	BGModelParams *BGParams = NULL;
	BGParams = ( BGModelParams *) malloc( sizeof( BGModelParams));
	if ( !BGParams ) {error(4);return 0;}

	// Crear Modelo de fondo estático .Solo en la primera ejecución
	printf("\n\t1)Creando modelo de Fondo..... ");
	// establecer parametros
	InitialBGModelParams( BGParams );

	bgmodel = initBGModel( g_capture , BGParams );
	if(!bgmodel) return 0;
	TiempoParcial = obtenerTiempo( ti , 1);
	printf("\t-Tiempo total: %0.2f seg\n", TiempoParcial);

	gettimeofday(&ti, NULL);
	// Crear modelo de forma
	printf("\t2)Creando modelo de forma..... ");
	// Iniciar estructura para modelo de forma

	shape = ( SHModel *) malloc( sizeof( SHModel));
	if ( !shape ) {error(4);return 0;}
	shape->FlyAreaDes = 0;
	shape->FlyAreaMed = 0;
	shape->FlyAreaMedia=0;
	gettimeofday(&ti, NULL);

	ShapeModel( g_capture, shape , bgmodel->ImFMask, bgmodel->DataFROI );

	TiempoParcial = obtenerTiempo( ti , 1 );

	printf("Hecho. Tiempo: %0.2f seg\n", TiempoParcial);

	NumFrame = cvGetCaptureProperty( g_capture, 1 ); //Actualizamos los frames

	*BGModel = bgmodel;
	*Shape = shape;

	return hecho = 1;
}

void InitialBGModelParams( BGModelParams* Params){

	 if ( DETECTAR_PLATO ) Params->FLAT_FRAMES_TRAINING = 50;
	 else Params->FLAT_FRAMES_TRAINING = 0;
	 Params->FRAMES_TRAINING = 30;
	 Params->ALPHA = 0 ;
	 Params->INITIAL_DESV = 0.05;

}

void releaseDataPreProcess(){
	free(BGParams);

}
