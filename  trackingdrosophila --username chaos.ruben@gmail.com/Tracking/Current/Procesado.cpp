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

void Procesado( IplImage* frame,tlcde* framesBuf, StaticBGModel* BGModel,STFlat* Flat,SHModel* Shape ){

	extern double NumFrame;
	struct timeval ti, tf, tif, tff; // iniciamos la estructura
	int tiempoParcial;
	// otros parámetros
	int fr = 0;
	int BGUpdate = 1;
	int UpdateCount = 0;
	BGModelParams *BGParams = NULL;

	STFrame* frameData;

	IplImage* BGTemp;
	IplImage* DETemp;
	IplImage *Imagen;
	IplImage* FGMask;

	CvSize size = cvGetSize( frame );
	BGTemp = cvCreateImage( size,8,1);
	DETemp = cvCreateImage( size,8,1);
	Imagen = cvCreateImage( size ,8,1);
	FGMask = cvCreateImage( size, 8, 1);
	cvZero( BGTemp );
	cvZero( DETemp );
	cvZero( Imagen );
	cvZero( FGMask );

	static int first = 1;
	gettimeofday(&ti, NULL);
	BGParams = ( BGModelParams *) malloc( sizeof( BGModelParams));
	if ( !BGParams ) {error(4);exit(-1 );}

	ImPreProcess( frame, Imagen, BGModel->ImFMask, 0, Flat->DataFROI);

	gettimeofday(&tf, NULL);
	tiempoParcial= (tf.tv_sec - ti.tv_sec)*1000 + \
							(tf.tv_usec - ti.tv_usec)/1000.0;
	printf( "\n\t\t\tFRAME %.0f\n", NumFrame);
	printf("\nPreprocesado de imagen: %5.4g ms\n", tiempoParcial);

	// Iniciar estructura para datos del nuevo frame
	if( first ) { //en la primera iteración inicializamos el fondo al obtenido en el modelo de fondo
		frameData = ( STFrame *) malloc( sizeof(STFrame));
		InitNewFrameData( Imagen, frameData );
		cvCopy(  BGModel->Imed,frameData->BGModel);
		cvCopy(BGModel->IDesv,frameData->IDesv);
		first = 0;
	}
	else{ // en las siguientes iteraciones inicializamos el nuevo al Actual.
		irAlFinal( framesBuf );
		frameData = ( STFrame*)obtenerActual( framesBuf );
		cvCopy( frameData->BGModel, BGTemp);
		cvCopy( frameData->IDesv, DETemp);
		frameData = NULL;
		frameData = ( STFrame *) malloc( sizeof(STFrame));
		InitNewFrameData( Imagen, frameData );
		// copiamos los últimos parámetros del fondo.
		cvCopy( BGTemp, frameData->BGModel);
		cvCopy( DETemp, frameData->IDesv);
	}
	cvCopy(  frameData->BGModel,BGTemp );
	cvCopy( frameData->IDesv,DETemp );



	for ( int i = 0; i < 3; i++){
		gettimeofday(&ti, NULL);
		if ( i == 0 ) printf("\nDefiniendo foreground :\n\n");
		if ( i > 0 ) printf("\nRedefiniendo foreground %d de 2:\n\n", i);
		//// BACKGROUND UPDATE
		// Actualización del fondo original
		// establecer parametros
		putBGModelParams( BGParams);

		UpdateBGModel( Imagen, frameData->BGModel,frameData->IDesv, BGParams, Flat->DataFROI, frameData->FG );

		gettimeofday(&tf, NULL);
		tiempoParcial= (tf.tv_sec - ti.tv_sec)*1000 + \
										(tf.tv_usec - ti.tv_usec)/1000.0;
		printf("Background update: %5.4g ms\n", tiempoParcial);

		/////// BACKGROUND DIFERENCE. Obtención de la máscara del foreground
		gettimeofday(&ti, NULL);

		BackgroundDifference( Imagen, frameData->BGModel,frameData->IDesv, frameData->FG ,BGParams, Flat->DataFROI);

		gettimeofday(&tf, NULL);
		tiempoParcial= (tf.tv_sec - ti.tv_sec)*1000 + \
									(tf.tv_usec - ti.tv_usec)/1000.0;
		printf("Obtención de máscara de Foreground : %5.4g ms\n", tiempoParcial);
		/////// SEGMENTACION
		if( i > 0 ){
			if(frameData->Flies != NULL) {
				liberarListaFlies( frameData->Flies );//nos quedamos con la última
				free(frameData->Flies);
				frameData->Flies = NULL;
			}
			gettimeofday(&ti, NULL);
			printf( "Segmentando Foreground...");

			frameData->Flies = segmentacion(Imagen, frameData, Flat->DataFROI, FGMask );

			gettimeofday(&tf, NULL);
			tiempoParcial= (tf.tv_sec - ti.tv_sec)*1000 + \
									(tf.tv_usec - ti.tv_usec)/1000.0;
			printf(" %5.4g ms\n", tiempoParcial);
		}
		// Volvemos a cargar el original,en la ultima iteracion nos kedamos con ultimo BGModel obtenido
		if (i < 2 ){
			cvCopy( BGTemp, frameData->BGModel );
			cvCopy( DETemp, frameData->IDesv );
		}
		/////// VALIDACIÓN

			if (i > 1){
				gettimeofday(&ti, NULL);
				printf( "\nValidando contornos...");


				//frameData->Flies = Validacion(Imagen, frameData , Shape, Flat->DataFROI, NULL, NULL,FGMask);


				gettimeofday(&tf, NULL);
				tiempoParcial= (tf.tv_sec - ti.tv_sec)*1000 +
										(tf.tv_usec - ti.tv_usec)/1000.0;
				printf(" %5.4g ms\n", tiempoParcial);
			}

	}
	// Una vez validada añadimos ( al final ) las estructuras a las listas (buffers).
	free(BGParams);
	cvReleaseImage( &BGTemp );
	cvReleaseImage( &DETemp );
	cvReleaseImage( &Imagen );
	cvReleaseImage(&FGMask);

	anyadirAlFinal( frameData, framesBuf );
	//STFrame* Mosca=(STFrame*)obtenerActual(framesBuf);
//	printf("\n ELEMENTOS : %d",Mosca->Flies->numeroDeElementos);
}

///! Igual que procesado, pero solo una iteración. Background dinámico para detección de movimiento

void Procesado2( IplImage* frame,tlcde* framesBuf, StaticBGModel* BGModel,STFlat* Flat,SHModel* Shape ){

	extern double NumFrame;
	struct timeval ti, tf, tif, tff; // iniciamos la estructura
	int tiempoParcial;
	// otros parámetros
	int fr = 0;
	int BGUpdate = 1;
	int UpdateCount = 0;
	BGModelParams *BGParams = NULL;

	STFrame* frameData = NULL;

	tlcde* FliesFG = NULL;
	//tlcde* FliesOldFG = NULL;

	IplImage *Imagen;
	IplImage *FGMask;

	CvSize size = cvGetSize( frame );

	Imagen = cvCreateImage( size ,8,1);
	FGMask = cvCreateImage( size, 8, 1);
	cvZero( Imagen );
	cvZero( FGMask );

	gettimeofday(&ti, NULL);
	BGParams = ( BGModelParams *) malloc( sizeof( BGModelParams));
	if ( !BGParams ) {error(4);exit(-1 );}

	ImPreProcess( frame, Imagen, BGModel->ImFMask, 0, Flat->DataFROI);

	static int first = 1;
	// Iniciar estructura para datos del nuevo frame
	if( first ) { //en la primera iteración
		frameData = ( STFrame *) malloc( sizeof(STFrame));
		InitNewFrameData( Imagen, frameData );
		cvCopy(  BGModel->Imed,frameData->BGModel);
		cvCopy(BGModel->IDesv,frameData->IDesv);
		first = 0;
	}
	else{
		irAlFinal( framesBuf );
		STFrame* frameAnterior = ( STFrame*)obtenerActual( framesBuf );
		frameData = ( STFrame *) malloc( sizeof(STFrame));
		InitNewFrameData( Imagen, frameData );
		// cargamos los últimos parámetros del fondo.
		cvCopy( frameAnterior->BGModel, frameData->BGModel);
		cvCopy( frameAnterior->IDesv, frameData->IDesv);
	}

//	obtener la mascara del FG y la lista con los datos de sus blobs.
	gettimeofday(&ti, NULL);
    printf("\nDefiniendo foreground :\n\n");

	//// BACKGROUND UPDATE
	// Actualización del fondo
	// establecer parametros
	putBGModelParams( BGParams);
//	cvUpdateBGStatModel( tmp_frame, bg_model, update_bg_model ? -1 : 0 );
	UpdateBGModel( Imagen, frameData->BGModel,frameData->IDesv, BGParams, Flat->DataFROI, frameData->FG );

	gettimeofday(&tf, NULL);
	tiempoParcial= (tf.tv_sec - ti.tv_sec)*1000 + \
									(tf.tv_usec - ti.tv_usec)/1000.0;
	printf("Background update: %5.4g ms\n", tiempoParcial);

	/////// BACKGROUND DIFERENCE. Obtención de la máscara del foreground
	gettimeofday(&ti, NULL);

	BackgroundDifference( Imagen, frameData->BGModel,frameData->IDesv, frameData->FG ,BGParams, Flat->DataFROI);

	gettimeofday(&tf, NULL);
	tiempoParcial= (tf.tv_sec - ti.tv_sec)*1000 + \
								(tf.tv_usec - ti.tv_usec)/1000.0;
	printf("Obtención de máscara de Foreground : %5.4g ms\n", tiempoParcial);
	/////// SEGMENTACION

	frameData->Flies = segmentacion(Imagen, frameData, Flat->DataFROI, FGMask);

	gettimeofday(&tf, NULL);
	tiempoParcial= (tf.tv_sec - ti.tv_sec)*1000 + \
							(tf.tv_usec - ti.tv_usec)/1000.0;
	printf(" %5.4g ms\n", tiempoParcial);
	/////// VALIDACIÓN
	// solo en la última iteracion

	gettimeofday(&ti, NULL);
	printf( "\nValidando contornos...");

//	frameData->Flies = Validacion(Imagen, frameData , Shape, Flat->DataFROI, NULL, NULL,FGMask);

	gettimeofday(&tf, NULL);
	tiempoParcial= (tf.tv_sec - ti.tv_sec)*1000 +
							(tf.tv_usec - ti.tv_usec)/1000.0;
	printf(" %5.4g ms\n", tiempoParcial);
//	FliesFG = (STFlies*)obtenerUltimo( frameData->Flies);

	//Anyadir al buffer

	anyadirAlFinal( frameData, framesBuf );

	// Liberar memoria

	free(BGParams);
	cvReleaseImage( &Imagen );
	cvReleaseImage( &FGMask );
}

//void Procesado3( IplImage* frame,tlcde* framesBuf, StaticBGModel* BGModel,STFlat* Flat,SHModel* Shape ){
//
//	extern double NumFrame;
//	struct timeval ti, tf, tif, tff; // iniciamos la estructura
//	int tiempoParcial;
//	// otros parámetros
//	int fr = 0;
//	int BGUpdate = 1;
//	int UpdateCount = 0;
//	BGModelParams *BGParams = NULL;
//
//	STFrame* frameData = NULL;
//
//	tlcde* FliesFG = NULL;
//	//tlcde* FliesOldFG = NULL;
//
//	IplImage *Imagen;
//	IplImage *FGMask;
//
//	CvSize size = cvGetSize( frame );
//
//	Imagen = cvCreateImage( size ,8,1);
//	FGMask = cvCreateImage( size, 8, 1);
//	cvZero( Imagen );
//	cvZero( FGMask );
//
//	gettimeofday(&ti, NULL);
//	BGParams = ( BGModelParams *) malloc( sizeof( BGModelParams));
//	if ( !BGParams ) {error(4);exit(-1 );}
//
//	ImPreProcess( frame, Imagen, BGModel->ImFMask, 0, Flat->DataFROI);
//
//	static int first = 1;
//	// Iniciar estructura para datos del nuevo frame
//	if( first ) { //en la primera iteración
//		frameData = ( STFrame *) malloc( sizeof(STFrame));
//		InitNewFrameData( Imagen, frameData );
//		cvCopy(  BGModel->Imed,frameData->BGModel);
//		cvCopy(BGModel->IDesv,frameData->IDesv);
//		first = 0;
//	}
//	else{
//		irAlFinal( framesBuf );
//		STFrame* frameAnterior = ( STFrame*)obtenerActual( framesBuf );
//		frameData = ( STFrame *) malloc( sizeof(STFrame));
//		InitNewFrameData( Imagen, frameData );
//		// cargamos los últimos parámetros del fondo.
//		cvCopy( frameAnterior->BGModel, frameData->BGModel);
//		cvCopy( frameAnterior->IDesv, frameData->IDesv);
//	}
//
////	obtener la mascara del FG y la lista con los datos de sus blobs.
//	gettimeofday(&ti, NULL);
//    printf("\nDefiniendo foreground :\n\n");
//
//	//// BACKGROUND UPDATE
//	// Actualización del fondo
//
//	cvUpdateBGStatModel( tmp_frame, bg_model, update_bg_model ? -1 : 0 );
//
//	gettimeofday(&tf, NULL);
//	tiempoParcial= (tf.tv_sec - ti.tv_sec)*1000 + \
//								(tf.tv_usec - ti.tv_usec)/1000.0;
//	printf("Obtención de máscara de Foreground : %5.4g ms\n", tiempoParcial);
//	/////// SEGMENTACION
//
//	frameData->Flies = segmentacion(Imagen, frameData, Flat->DataFROI, FGMask);
//
//	gettimeofday(&tf, NULL);
//	tiempoParcial= (tf.tv_sec - ti.tv_sec)*1000 + \
//							(tf.tv_usec - ti.tv_usec)/1000.0;
//	printf(" %5.4g ms\n", tiempoParcial);
//	/////// VALIDACIÓN
//	// solo en la última iteracion
//
//	gettimeofday(&ti, NULL);
//	printf( "\nValidando contornos...");
//
////	frameData->Flies = Validacion(Imagen, frameData , Shape, Flat->DataFROI, Flie, NULL, NULL);
//
//	gettimeofday(&tf, NULL);
//	tiempoParcial= (tf.tv_sec - ti.tv_sec)*1000 +
//							(tf.tv_usec - ti.tv_usec)/1000.0;
//	printf(" %5.4g ms\n", tiempoParcial);
////	FliesFG = (STFlies*)obtenerUltimo( frameData->Flies);
//
//	//Anyadir al buffer
//
//	anyadirAlFinal( frameData, framesBuf );
//
//	// Liberar memoria
//
//	free(BGParams);
//	cvReleaseImage( &Imagen );
//	cvReleaseImage( &FGMask );
//}

void InitNewFrameData(IplImage* I, STFrame *FrameData ){

	extern double NumFrame;
	CvSize size = cvGetSize( I );
	FrameData->BGModel = cvCreateImage(size,8,1);
	FrameData->FG = cvCreateImage(size,8,1);
	FrameData->IDesv = cvCreateImage(size,8,1);
	FrameData->OldFG = cvCreateImage(size,8,1);
	FrameData->ImMotion = cvCreateImage( size, 8, 3 );
	FrameData->ImMotion->origin = I->origin;
	cvZero( FrameData->BGModel );
	cvZero( FrameData->FG );
	cvZero( FrameData->IDesv );
	cvZero( FrameData->ImMotion);
	FrameData->Flies = NULL;
	FrameData->num_frame = (int)NumFrame;
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
					 Params->LOW_THRESHOLD = 13;
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
