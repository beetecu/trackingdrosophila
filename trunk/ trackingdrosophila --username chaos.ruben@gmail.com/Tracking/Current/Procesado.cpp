/*
 * Procesado.cpp
 *
 *  Created on: 20/10/2011
 *      Author: chao
 *
 *   Esta función realiza las siguientes acciones:
 *
 * - Limpieza del foreground en tres etapas :
 *   1_Actualización de fondo y resta de fondo obteniendo el foreground
 *   2_Nueva actualización de fondo usando la máscacara de foreground obtenida.
 *   Resta de fondo
 *   Redefinición de la máscara de foreground mediante ajuste por elipses y
 *   obtención de los parámetros de los blobs en segmentación.
 *   3_Repetir de nuevo el ciclo de actualizacion-resta-segmentación con la nueva máscara
 *   obteniendo así el foreground y el background definitivo
 *
 * - Rellena la lista lineal doblemente enlazada ( Flies )con los datos de cada uno de los blobs
 * - Rellena la estructura FrameData con las nuevas imagenes y la lista Flies.
 * - Finalmente añade la lista Flies a la estructura FrameData
 *   y la estructura FrameData al buffer FramesBuf   */



#include "Procesado.hpp"



BGModelParams *BGPrParams = NULL;
IplImage *Imagen = NULL; // imagen preprocesada
IplImage *FGMask = NULL; // mascara del foreground
IplImage *lastBG = NULL;
IplImage *lastIdes = NULL;

/********************************************************************
 *			 			MODELO MEDIANA      						*
 * 				 SIN ACTUALIZACIÓN SELECTIVA						*
 ********************************************************************/

STFrame* Procesado( IplImage* frame, StaticBGModel* BGModel,SHModel* Shape ){

	extern double NumFrame;
	struct timeval ti, tf,tif,tff; // iniciamos la estructura
	float tiempoParcial;
	// otros parámetros
	static int fr = 0;
	static float suma;
	STFrame* frameData = NULL;

	fr++;
	AllocateDataProcess( frame );
	gettimeofday(&ti, NULL);

	ImPreProcess( frame, Imagen, BGModel->ImFMask, 0, BGModel->DataFROI);
	
	tiempoParcial = obtenerTiempo( ti, 0);
	printf("\t0)Preprocesado : %5.4g ms\n", tiempoParcial);
	static bool first = true;
	// Iniciar estructura para datos del nuevo frame
	frameData = InitNewFrameData( frame );
	// Cargamos datos
	if( first ) { //en la primera iteración iniciamos el modelo dinamico al estático
		cvCopy(  BGModel->Imed,frameData->BGModel);
		cvSet(frameData->IDesvf, cvScalar(1));
		cvCopy(  BGModel->Imed,lastBG);
		first = false;
	}
	else{	// cargamos los últimos parámetros del fondo.
		cvCopy( lastBG, frameData->BGModel);
		cvCopy( lastIdes,frameData->IDesvf );
	}
	// copiamos el frame en la estructura
	cvCopy( frame,frameData->Frame);
//	obtener la mascara del FG y la lista con los datos de sus blobs.

	//// BACKGROUND UPDATE
	// Actualización del fondo
	// establecer parametros
	gettimeofday(&ti, NULL); printf("\t1)Actualización del modelo de fondo:\n");
	putBGModelParams( BGPrParams);
//	cvUpdateBGStatModel( tmp_frame, bg_model, update_bg_model ? -1 : 0 );

	UpdateBGModel( Imagen,frameData->BGModel,frameData->IDesvf, BGPrParams, BGModel->DataFROI, BGModel->ImFMask );
//	cvShowImage("Background",frameData->BGModel);
//	cvWaitKey(0);
	tiempoParcial = obtenerTiempo( ti, 0);
	printf("\t\t-Tiempo total: %5.4g ms\n", tiempoParcial);

	/////// BACKGROUND DIFERENCE. Obtención de la máscara del foreground
	gettimeofday(&ti, NULL);printf("\t2)Resta de Fondo \n");

	BackgroundDifference( Imagen, frameData->BGModel,frameData->IDesvf, frameData->FG ,BGPrParams, BGModel->DataFROI);
	tiempoParcial = obtenerTiempo( ti, 0);
	printf("\t\t-Tiempo total : %5.4g ms\n", tiempoParcial);
//	VisualizarFr( frameData, BGModel );
//	cvWaitKey(0);
	/////// SEGMENTACION
	gettimeofday(&ti, NULL);
	printf("\t3)Segmentación:\n");
	frameData->Flies = segmentacion(Imagen, frameData, BGModel->DataFROI, FGMask);
	cvCopy( FGMask,frameData->FG );
	tiempoParcial = obtenerTiempo( ti, 0);
	suma = suma  + tiempoParcial;
	float total = suma/fr;
	printf("\t\t-Tiempo total: %5.4g ms\n\t\tMedia = %5.4g ms\n", tiempoParcial,total);

	/////// VALIDACIÓN
	gettimeofday(&ti, NULL);
//	frameData->Flies = Validacion(Imagen, frameData , Shape, BGModel->DataFROI, NULL, NULL,FGMask);
	tiempoParcial = obtenerTiempo( ti, 0);
	printf("\t4)Validación de blobs %5.4g ms\n", tiempoParcial);
	cvCopy( frameData->BGModel, lastBG );
	cvCopy( frameData->IDesvf, lastIdes );

	return frameData;
}

/********************************************************************
 *			 			MODELO MEDIANA      						*
 * 				 CON ACTUALIZACIÓN SELECTIVA						*
 ********************************************************************/
STFrame* Procesado2( IplImage* frame, StaticBGModel* BGModel,SHModel* Shape ){

	extern double NumFrame;
	struct timeval ti, tf,tif,tff; // iniciamos la estructura
	float tiempoParcial;
	// otros parámetros
	int fr = 0;
	int BGUpdate = 1;
	int UpdateCount = 0;

	STFrame* frameData = NULL;
	printf("\nIniciando Procesado:\n");
	gettimeofday(&tif, NULL);

	AllocateDataProcess( frame );
	gettimeofday(&ti, NULL);

	ImPreProcess( frame, Imagen, BGModel->ImFMask, 0, BGModel->DataFROI);
	tiempoParcial = obtenerTiempo( ti, 0);
	printf("\t0)Preprocesado : %5.4g ms\n", tiempoParcial);


	static bool first = true;
	// Iniciar estructura para datos del nuevo frame
	frameData = InitNewFrameData( frame );
	// Cargamos datos
	if( first ) { //en la primera iteración iniciamos el modelo dinamico al estático
		cvCopy(  BGModel->Imed,frameData->BGModel);
		cvCopy(BGModel->IDesvf,frameData->IDesvf);
		cvCopy(  BGModel->Imed,lastBG);
		cvCopy(BGModel->IDesvf,lastIdes);
		first = false;
	}
	else{	// cargamos los últimos parámetros del fondo.
		cvCopy( lastBG, frameData->BGModel);
		cvCopy( lastIdes, frameData->IDesvf);
	}
	// copiamos el frame en la estructura
	cvCopy( frame,frameData->Frame);
//	obtener la mascara del FG y la lista con los datos de sus blobs.

	//// BACKGROUND UPDATE
	// Actualización del fondo
	// establecer parametros
	gettimeofday(&ti, NULL);
	putBGModelParams( BGPrParams);
//	cvUpdateBGStatModel( tmp_frame, bg_model, update_bg_model ? -1 : 0 );

	UpdateBGModel( Imagen,frameData->BGModel,frameData->IDesvf, BGPrParams, BGModel->DataFROI, BGModel->ImFMask );
//	cvShowImage("Background",frameData->BGModel);
//	cvWaitKey(0);
	tiempoParcial = obtenerTiempo( ti, 0);
	printf("\t1)Actualización del modelo de fondo: %5.4g ms\n", tiempoParcial);

	/////// BACKGROUND DIFERENCE. Obtención de la máscara del foreground
	gettimeofday(&ti, NULL);

	BackgroundDifference( Imagen, frameData->BGModel,frameData->IDesvf, frameData->FG ,BGPrParams, BGModel->DataFROI);
	tiempoParcial = obtenerTiempo( ti, 0);
	printf("\t2)Resta de Fondo : %5.4g ms\n", tiempoParcial);
//	VisualizarFr( frameData, BGModel );
//	cvWaitKey(0);
	/////// SEGMENTACION
	gettimeofday(&ti, NULL);
//	frameData->Flies = segmentacion(Imagen, frameData, BGModel->DataFROI, FGMask);
	//cvCopy( FGMask,frameData->FG );
	tiempoParcial = obtenerTiempo( ti, 0);
	printf("\t3)Obtención de blobs: %5.4g ms\n", tiempoParcial);

	/////// VALIDACIÓN
	gettimeofday(&ti, NULL);
//	frameData->Flies = Validacion(Imagen, frameData , Shape, BGModel->DataFROI, NULL, NULL,FGMask);
	tiempoParcial = obtenerTiempo( ti, 0);
	printf("\t4)Validación de blobs %5.4g ms\n", tiempoParcial);

	// Actualización selectiva
	gettimeofday(&ti, NULL);
	cvZero( FGMask);
	cvCopy( BGModel->ImFMask,FGMask );
	cvAdd( frameData->FG, FGMask, FGMask );
	UpdateBGModel( Imagen, lastBG,lastIdes, BGPrParams, BGModel->DataFROI, FGMask );
	// guardamos las imagenes definitivas
	cvCopy( lastBG, frameData->BGModel);
	cvCopy(  lastIdes, frameData->IDesvf);
	tiempoParcial = obtenerTiempo( ti, 0);
	printf("\t5)Actualización selectiva del Modelo de fondo: %5.4g ms\n", tiempoParcial);

	gettimeofday(&tff, NULL);
	tiempoParcial= (tff.tv_sec - tif.tv_sec)*1000 +
							(tff.tv_usec - tif.tv_usec)/1000.0;
	printf("Tiempo de procesado %5.4g ms\n", tiempoParcial);

	return frameData;

}

/********************************************************************
 *			 			MODELO GAUSSIANO      						*
 * 				   SIN ACTUALIZACIÓN SELECTIVA						*
 ********************************************************************/

/// SIN ACTUALIZACIÓN SELECTIVA y modelo gaussiano

STFrame* Procesado3( IplImage* frame, StaticBGModel* BGModel,SHModel* Shape ){

	extern double NumFrame;
	struct timeval ti, tf,tif,tff; // iniciamos la estructura
	float tiempoParcial;
	// otros parámetros
	int fr = 0;
	int BGUpdate = 1;
	int UpdateCount = 0;

	STFrame* frameData = NULL;
	printf("\nIniciando Procesado:\n");
	gettimeofday(&tif, NULL);

	AllocateDataProcess( frame );
	gettimeofday(&ti, NULL);

	ImPreProcess( frame, Imagen, BGModel->ImFMask, 0, BGModel->DataFROI);

	
	
								tiempoParcial = obtenerTiempo( ti, 0);
	printf("\t0)Preprocesado : %5.4g ms\n", tiempoParcial);


	static bool first = true;
	// Iniciar estructura para datos del nuevo frame
	frameData = InitNewFrameData( frame );
	// Cargamos datos
	if( first ) { //en la primera iteración iniciamos el modelo dinamico al estático
		cvCopy(  BGModel->Imed,frameData->BGModel);
		cvCopy(BGModel->IDesvf,frameData->IDesvf);
		cvCopy(  BGModel->Imed,lastBG);
		cvCopy(BGModel->IDesvf,lastIdes);
		first = false;
	}
	else{	// cargamos los últimos parámetros del fondo.
		cvCopy( lastBG, frameData->BGModel);
		cvCopy( lastIdes, frameData->IDesvf);
	}
	// copiamos el frame en la estructura
	cvCopy( frame,frameData->Frame);
//	obtener la mascara del FG y la lista con los datos de sus blobs.

	//// BACKGROUND UPDATE
	// Actualización del fondo
	// establecer parametros
	gettimeofday(&ti, NULL);
	putBGModelParams( BGPrParams);
//	cvUpdateBGStatModel( tmp_frame, bg_model, update_bg_model ? -1 : 0 );

	UpdateBGModel( Imagen,frameData->BGModel,frameData->IDesvf, BGPrParams, BGModel->DataFROI, BGModel->ImFMask );
//	cvShowImage("Background",frameData->BGModel);
//	cvWaitKey(0);
	
	
									tiempoParcial = obtenerTiempo( ti, 0);
	printf("\t1)Actualización del modelo de fondo: %5.4g ms\n", tiempoParcial);

	/////// BACKGROUND DIFERENCE. Obtención de la máscara del foreground
	gettimeofday(&ti, NULL);

	BackgroundDifference( Imagen, frameData->BGModel,frameData->IDesvf, frameData->FG ,BGPrParams, BGModel->DataFROI);

	
	
								tiempoParcial = obtenerTiempo( ti, 0);
	printf("\t2)Resta de Fondo : %5.4g ms\n", tiempoParcial);
//	VisualizarFr( frameData, BGModel );
//	cvWaitKey(0);
	/////// SEGMENTACION
	gettimeofday(&ti, NULL);
//	frameData->Flies = segmentacion(Imagen, frameData, BGModel->DataFROI, FGMask);
//	cvCopy( FGMask,frameData->FG );
	
	
									tiempoParcial = obtenerTiempo( ti, 0);
	printf("\t3)Obtención de blobs: %5.4g ms\n", tiempoParcial);

	/////// VALIDACIÓN
	gettimeofday(&ti, NULL);
//	frameData->Flies = Validacion(Imagen, frameData , Shape, BGModel->DataFROI, NULL, NULL,FGMask);
	

							tiempoParcial = obtenerTiempo( ti, 0);
	printf("\t4)Validación de blobs %5.4g ms\n", tiempoParcial);


	gettimeofday(&tff, NULL);
	tiempoParcial= (tff.tv_sec - tif.tv_sec)*1000 +
							(tff.tv_usec - tif.tv_usec)/1000.0;
	printf("Tiempo de procesado %5.4g ms\n", tiempoParcial);

	return frameData;

}

/********************************************************************
 *			 			MODELO GAUSSIANO      						*
 * 				  CON ACTUALIZACIÓN SELECTIVA						*
 ********************************************************************/

STFrame* Procesado4( IplImage* frame, StaticBGModel* BGModel,SHModel* Shape ){

	extern double NumFrame;
	struct timeval ti, tf,tif,tff; // iniciamos la estructura
	float tiempoParcial;
	// otros parámetros
	int fr = 0;
	int BGUpdate = 1;
	int UpdateCount = 0;

	STFrame* frameData = NULL;
	printf("\nIniciando Procesado:\n");
	gettimeofday(&tif, NULL);

	AllocateDataProcess( frame );
	gettimeofday(&ti, NULL);

	ImPreProcess( frame, Imagen, BGModel->ImFMask, 0, BGModel->DataFROI);

	
	
								tiempoParcial = obtenerTiempo( ti, 0);
	printf("\t0)Preprocesado : %5.4g ms\n", tiempoParcial);


	static bool first = true;
	// Iniciar estructura para datos del nuevo frame
	frameData = InitNewFrameData( frame );
	// Cargamos datos
	if( first ) { //en la primera iteración iniciamos el modelo dinamico al estático
		cvCopy(  BGModel->Imed,frameData->BGModel);
		cvCopy(BGModel->IDesvf,frameData->IDesvf);
		cvCopy(  BGModel->Imed,lastBG);
		cvCopy(BGModel->IDesvf,lastIdes);
		first = false;
	}
	else{	// cargamos los últimos parámetros del fondo.
		cvCopy( lastBG, frameData->BGModel);
		cvCopy( lastIdes, frameData->IDesvf);
	}
	// copiamos el frame en la estructura
	cvCopy( frame,frameData->Frame);
//	obtener la mascara del FG y la lista con los datos de sus blobs.

	//// BACKGROUND UPDATE
	// Actualización del fondo
	// establecer parametros
	gettimeofday(&ti, NULL);
	putBGModelParams( BGPrParams);
//	cvUpdateBGStatModel( tmp_frame, bg_model, update_bg_model ? -1 : 0 );

	UpdateBGModel( Imagen,frameData->BGModel,frameData->IDesvf, BGPrParams, BGModel->DataFROI, BGModel->ImFMask );
//	cvShowImage("Background",frameData->BGModel);
//	cvWaitKey(0);
	
	
									tiempoParcial = obtenerTiempo( ti, 0);
	printf("\t1)Actualización del modelo de fondo: %5.4g ms\n", tiempoParcial);

	/////// BACKGROUND DIFERENCE. Obtención de la máscara del foreground
	gettimeofday(&ti, NULL);

	BackgroundDifference( Imagen, frameData->BGModel,frameData->IDesvf, frameData->FG ,BGPrParams, BGModel->DataFROI);

	
	
								tiempoParcial = obtenerTiempo( ti, 0);
	printf("\t2)Resta de Fondo : %5.4g ms\n", tiempoParcial);
//	VisualizarFr( frameData, BGModel );
//	cvWaitKey(0);
	/////// SEGMENTACION
	gettimeofday(&ti, NULL);
//	frameData->Flies = segmentacion(Imagen, frameData, BGModel->DataFROI, FGMask);
	//cvCopy( FGMask,frameData->FG );
	
	
									tiempoParcial = obtenerTiempo( ti, 0);
	printf("\t3)Obtención de blobs: %5.4g ms\n", tiempoParcial);

	/////// VALIDACIÓN
	gettimeofday(&ti, NULL);
//	frameData->Flies = Validacion(Imagen, frameData , Shape, BGModel->DataFROI, NULL, NULL,FGMask);
	tiempoParcial = obtenerTiempo( ti, 0);
	printf("\t4)Validación de blobs %5.4g ms\n", tiempoParcial);

	// Actualización selectiva
	gettimeofday(&ti, NULL);
	cvZero( FGMask);
	cvCopy( BGModel->ImFMask,FGMask );
	cvAdd( frameData->FG, FGMask, FGMask );
	UpdateBGModel( Imagen, lastBG,lastIdes, BGPrParams, BGModel->DataFROI, FGMask );
	// guardamos las imagenes definitivas
	cvCopy( lastBG, frameData->BGModel);
	cvCopy(  lastIdes, frameData->IDesvf);
	
	
									tiempoParcial = obtenerTiempo( ti, 0);
	printf("\t5)Actualización selectiva del Modelo de fondo: %5.4g ms\n", tiempoParcial);

	gettimeofday(&tff, NULL);
	tiempoParcial= (tff.tv_sec - tif.tv_sec)*1000 +
							(tff.tv_usec - tif.tv_usec)/1000.0;
	printf("Tiempo de procesado %5.4g ms\n", tiempoParcial);

	return frameData;

}

STFrame* InitNewFrameData(IplImage* I ){

	STFrame* FrameData;
	FrameData = ( STFrame *) malloc( sizeof(STFrame));

	extern double NumFrame;
	CvSize size = cvGetSize( I );
	FrameData->Frame = cvCreateImage(size,8,3);
	FrameData->BGModel = cvCreateImage(size,8,1);
	FrameData->FG = cvCreateImage(size,8,1);
	FrameData->IDesvf = cvCreateImage(size,IPL_DEPTH_32F,1);
	FrameData->OldFG = cvCreateImage(size,8,1);
	FrameData->ImMotion = cvCreateImage( size, 8, 3 );
	FrameData->ImMotion->origin = I->origin;
	cvZero( FrameData->Frame );
	cvZero( FrameData->BGModel );
	cvZero( FrameData->FG );
	cvZero( FrameData->IDesvf );
	cvZero( FrameData->ImMotion);
	FrameData->Flies = NULL;
	FrameData->num_frame = (int)NumFrame;

	return FrameData;
}

void putBGModelParams( BGModelParams* Params){
	 static int first = 1;
	 Params->FRAMES_TRAINING = 20;
	 Params->ALPHA = 0 ;
	 Params->MORFOLOGIA = 0;
	 Params->CVCLOSE_ITR = 1;
	 Params->MAX_CONTOUR_AREA = 0 ; //200
	 Params->MIN_CONTOUR_AREA = 0; //5
	 if (CREATE_TRACKBARS == 1){
				 // La primera vez inicializamos los valores.
				 if (first == 1){
					 Params->HIGHT_THRESHOLD = 10;

					 Params->LOW_THRESHOLD = 3;

					 first = 0;
				 }
	 			cvCreateTrackbar( "HighT",
	 							  "Foreground",
	 							  &Params->HIGHT_THRESHOLD,
	 							  100  );
	 			cvCreateTrackbar( "LowT",
	 							  "Foreground",
	 							  &Params->LOW_THRESHOLD,
	 							  100  );
	 }else{
		 Params->HIGHT_THRESHOLD = 10;
		 Params->LOW_THRESHOLD = 3;
	 }
}

void AllocateDataProcess( IplImage *I ) {

	CvSize size = cvGetSize( I );

		if(!Imagen) {
			Imagen = cvCreateImage( size ,8,1);
			FGMask = cvCreateImage( size, 8, 1);
			lastIdes = cvCreateImage( size ,IPL_DEPTH_32F,1);
			lastBG = cvCreateImage( size, 8, 1);
		}
		cvZero( Imagen );
		cvZero( FGMask );
		if( !BGPrParams ) BGPrParams = ( BGModelParams *) malloc( sizeof( BGModelParams));
		if ( !BGPrParams ) {error(4);exit(-1 );}
}
void releaseDataProcess(){
	free(BGPrParams);
	cvReleaseImage( &Imagen );
	cvReleaseImage( &FGMask );
	cvReleaseImage( &lastBG );
	cvReleaseImage( &lastIdes );
	ReleaseDataSegm( );
}
