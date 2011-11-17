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
//void Procesado( IplImage* frame,tlcde* framesBuf, StaticBGModel* BGModel,SHModel* Shape ){
//
//	extern double NumFrame;
//	struct timeval ti, tf, tif, tff; // iniciamos la estructura
//	int tiempoParcial;
//	// otros parámetros
//	int fr = 0;
//	int BGUpdate = 1;
//	int UpdateCount = 0;
//	BGModelParams *BGPrParams = NULL;
//
//	STFrame* frameData;
//
//	IplImage* BGTemp;
//	IplImage* DETemp;
//	IplImage *Imagen;
//	IplImage* FGMask;
//
//	CvSize size = cvGetSize( frame );
//	BGTemp = cvCreateImage( size,8,1);
//	DETemp = cvCreateImage( size,8,1);
//	Imagen = cvCreateImage( size ,8,1);
//	FGMask = cvCreateImage( size, 8, 1);
//	cvZero( BGTemp );
//	cvZero( DETemp );
//	cvZero( Imagen );
//	cvZero( FGMask );
//
//	static int first = 1;
//	gettimeofday(&ti, NULL);
//	BGPrParams = ( BGModelParams *) malloc( sizeof( BGModelParams));
//	if ( !BGPrParams ) {error(4);exit(-1 );}
//
//	ImPreProcess( frame, Imagen, BGModel->ImFMask, 0, BGModel->DataFROI);
//
//	gettimeofday(&tf, NULL);
//	tiempoParcial= (tf.tv_sec - ti.tv_sec)*1000 + \
//							(tf.tv_usec - ti.tv_usec)/1000.0;
//	printf( "\n\t\t\tFRAME %.0f\n", NumFrame);
//	printf("\nPreprocesado de imagen: %5.4g ms\n", tiempoParcial);
//
//	// Iniciar estructura para datos del nuevo frame
//	if( first ) { //en la primera iteración inicializamos el fondo al obtenido en el modelo de fondo
//		frameData = ( STFrame *) malloc( sizeof(STFrame));
//		InitNewFrameData( Imagen, frameData );
//		cvCopy( frame,frameData->Frame);
//		cvCopy(  BGModel->Imed,frameData->BGModel);
//		cvCopy(BGModel->IDesv,frameData->IDesv);
//		first = 0;
//	}
//	else{ // en las siguientes iteraciones inicializamos el nuevo al Actual.
//		irAlFinal( framesBuf );
//		frameData = ( STFrame*)obtenerActual( framesBuf );
//		cvCopy( frame,frameData->Frame);
//		cvCopy( frameData->BGModel, BGTemp);
//		cvCopy( frameData->IDesv, DETemp);
//		frameData = NULL;
//		frameData = ( STFrame *) malloc( sizeof(STFrame));
//		InitNewFrameData( Imagen, frameData );
//		// copiamos los últimos parámetros del fondo.
//		cvCopy( BGTemp, frameData->BGModel);
//		cvCopy( DETemp, frameData->IDesv);
//	}
//	cvCopy(  frameData->BGModel,BGTemp );
//	cvCopy( frameData->IDesv,DETemp );
//
//
//
//	for ( int i = 0; i < 3; i++){
//		gettimeofday(&ti, NULL);
//		if ( i == 0 ) printf("\nDefiniendo foreground :\n\n");
//		if ( i > 0 ) printf("\nRedefiniendo foreground %d de 2:\n\n", i);
//		//// BACKGROUND UPDATE
//		// Actualización del fondo original
//		// establecer parametros
//		putBGModelParams( BGPrParams);
//
//		UpdateBGModel( Imagen, frameData->BGModel,frameData->IDesv, BGPrParams, BGModel->DataFROI, frameData->FG );
//
//		gettimeofday(&tf, NULL);
//		tiempoParcial= (tf.tv_sec - ti.tv_sec)*1000 + \
//										(tf.tv_usec - ti.tv_usec)/1000.0;
//		printf("Background update: %5.4g ms\n", tiempoParcial);
//
//		/////// BACKGROUND DIFERENCE. Obtención de la máscara del foreground
//		gettimeofday(&ti, NULL);
//
//		BackgroundDifference( Imagen, frameData->BGModel,frameData->IDesv, frameData->FG ,BGPrParams, BGModel->DataFROI);
//
//		gettimeofday(&tf, NULL);
//		tiempoParcial= (tf.tv_sec - ti.tv_sec)*1000 + \
//									(tf.tv_usec - ti.tv_usec)/1000.0;
//		printf("Obtención de máscara de Foreground : %5.4g ms\n", tiempoParcial);
//		/////// SEGMENTACION
//		if( i > 0 ){
//			if(frameData->Flies != NULL) {
//				liberarListaFlies( frameData->Flies );//nos quedamos con la última
//				free(frameData->Flies);
//				frameData->Flies = NULL;
//			}
//			gettimeofday(&ti, NULL);
//			printf( "Segmentando Foreground...");
//
//			frameData->Flies = segmentacion(Imagen, frameData, BGModel->DataFROI, FGMask );
//
//			gettimeofday(&tf, NULL);
//			tiempoParcial= (tf.tv_sec - ti.tv_sec)*1000 + \
//									(tf.tv_usec - ti.tv_usec)/1000.0;
//			printf(" %5.4g ms\n", tiempoParcial);
//		}
//		// Volvemos a cargar el original,en la ultima iteracion nos kedamos con ultimo BGModel obtenido
//		if (i < 2 ){
//			cvCopy( BGTemp, frameData->BGModel );
//			cvCopy( DETemp, frameData->IDesv );
//		}
//		/////// VALIDACIÓN
//
//			if (i > 1){
//				gettimeofday(&ti, NULL);
//				printf( "\nValidando contornos...");
//
//
//				//frameData->Flies = Validacion(Imagen, frameData , Shape, BGModel->DataFROI, NULL, NULL,FGMask);
//
//
//				gettimeofday(&tf, NULL);
//				tiempoParcial= (tf.tv_sec - ti.tv_sec)*1000 +
//										(tf.tv_usec - ti.tv_usec)/1000.0;
//				printf(" %5.4g ms\n", tiempoParcial);
//			}
//
//	}
//	// Una vez validada añadimos ( al final ) las estructuras a las listas (buffers).
//	free(BGPrParams);
//	cvReleaseImage( &BGTemp );
//	cvReleaseImage( &DETemp );
//	cvReleaseImage( &Imagen );
//	cvReleaseImage(&FGMask);
//
//	anyadirAlFinal( frameData, framesBuf );
//	//STFrame* Mosca=(STFrame*)obtenerActual(framesBuf);
////	printf("\n ELEMENTOS : %d",Mosca->Flies->numeroDeElementos);
//}



STFrame* Procesado2( IplImage* frame, StaticBGModel* BGModel,SHModel* Shape ){

	extern double NumFrame;
	struct timeval ti, tf, tif, tff; // iniciamos la estructura
	int tiempoParcial;
	// otros parámetros
	int fr = 0;
	int BGUpdate = 1;
	int UpdateCount = 0;

	STFrame* frameData = NULL;

	tlcde* FliesFG = NULL;
	//tlcde* FliesOldFG = NULL;

	gettimeofday(&ti, NULL);

	AllocateDataProcess( frame );

	ImPreProcess( frame, Imagen, BGModel->ImFMask, 0, BGModel->DataFROI);

	static bool first = true;
	// Iniciar estructura para datos del nuevo frame
	frameData = InitNewFrameData( frame );
	// Cargamos datos
	if( first ) { //en la primera iteración iniciamos el modelo dinamico al estático
		cvCopy(  BGModel->Imed,frameData->BGModel);
		cvCopy(BGModel->IDesv,frameData->IDesv);
		first = false;
	}
	else{	// cargamos los últimos parámetros del fondo.
		cvCopy( lastBG, frameData->BGModel);
		cvCopy( lastIdes, frameData->IDesv);
	}
	// copiamos el frame en la estructura
	cvCopy( frame,frameData->Frame);
//	obtener la mascara del FG y la lista con los datos de sus blobs.
	gettimeofday(&ti, NULL);
    printf("\nDefiniendo foreground :\n\n");

	//// BACKGROUND UPDATE
	// Actualización del fondo
	// establecer parametros
	putBGModelParams( BGPrParams);
//	cvUpdateBGStatModel( tmp_frame, bg_model, update_bg_model ? -1 : 0 );
	UpdateBGModel( Imagen, frameData->BGModel,frameData->IDesv, BGPrParams, BGModel->DataFROI, frameData->FG );

	gettimeofday(&tf, NULL);
	tiempoParcial= (tf.tv_sec - ti.tv_sec)*1000 + \
									(tf.tv_usec - ti.tv_usec)/1000.0;
	printf("Background update: %5.4g ms\n", tiempoParcial);

	/////// BACKGROUND DIFERENCE. Obtención de la máscara del foreground
	gettimeofday(&ti, NULL);

	BackgroundDifference( Imagen, frameData->BGModel,frameData->IDesv, frameData->FG ,BGPrParams, BGModel->DataFROI);



	gettimeofday(&tf, NULL);
	tiempoParcial= (tf.tv_sec - ti.tv_sec)*1000 + \
								(tf.tv_usec - ti.tv_usec)/1000.0;
	printf("Obtención de máscara de Foreground : %5.4g ms\n", tiempoParcial);
	/////// SEGMENTACION

	frameData->Flies = segmentacion(Imagen, frameData, BGModel->DataFROI, FGMask);

	gettimeofday(&tf, NULL);
	tiempoParcial= (tf.tv_sec - ti.tv_sec)*1000 + \
							(tf.tv_usec - ti.tv_usec)/1000.0;
	printf(" %5.4g ms\n", tiempoParcial);

	cvCopy( FGMask,frameData->FG );

	/////// VALIDACIÓN
	gettimeofday(&ti, NULL);
	printf( "\nValidando contornos...");
//	frameData->Flies = Validacion(Imagen, frameData , Shape, BGModel->DataFROI, NULL, NULL,FGMask);

//	cvCopy( FGMask,frameData->FG );
	cvCopy( frameData->BGModel, lastBG);
	cvCopy( frameData->IDesv, lastIdes);
	// Medir tiempos
	gettimeofday(&tf, NULL);
	tiempoParcial= (tf.tv_sec - ti.tv_sec)*1000 +
							(tf.tv_usec - ti.tv_usec)/1000.0;
	printf(" %5.4g ms\n", tiempoParcial);

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
	FrameData->IDesv = cvCreateImage(size,8,1);
	FrameData->OldFG = cvCreateImage(size,8,1);
	FrameData->ImMotion = cvCreateImage( size, 8, 3 );
	FrameData->ImMotion->origin = I->origin;
	cvZero( FrameData->Frame );
	cvZero( FrameData->BGModel );
	cvZero( FrameData->FG );
	cvZero( FrameData->IDesv );
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
	 Params->MAX_CONTOUR_AREA = 200 ;
	 Params->MIN_CONTOUR_AREA = 5;
	 if (CREATE_TRACKBARS == 1){
				 // La primera vez inicializamos los valores.
				 if (first == 1){
					 Params->HIGHT_THRESHOLD = 20;

					 Params->LOW_THRESHOLD = 10;

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
		 Params->HIGHT_THRESHOLD = 20;
		 Params->LOW_THRESHOLD = 10;
	 }
}

void AllocateDataProcess( IplImage *I ) {

	CvSize size = cvGetSize( I );

		if(!Imagen) {
			Imagen = cvCreateImage( size ,8,1);
			FGMask = cvCreateImage( size, 8, 1);
			lastIdes = cvCreateImage( size ,8,1);
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
}
