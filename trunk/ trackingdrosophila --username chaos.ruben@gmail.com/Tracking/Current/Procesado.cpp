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

/********************************************************************
 *			 			MODELO MEDIANA      						*
 * 				 SIN ACTUALIZACIÓN SELECTIVA						*

 *			 			MODELO GAUSSIANO      						*
 * 				  CON ACTUALIZACIÓN SELECTIVA						*

 *			 			MODELO GAUSSIANO      						*
 * 				   SIN ACTUALIZACIÓN SELECTIVA						*
 ********************************************************************/


#include "Procesado.hpp"




IplImage *Imagen = NULL; // imagen preprocesada
IplImage *FGMask = NULL; // mascara del foreground
IplImage *lastBG = NULL;
IplImage *lastIdes = NULL;

/********************************************************************
 *			 			MODELO MEDIANA      						*
 * 				 CON ACTUALIZACIÓN SELECTIVA
 * 			OBTENCION DE MODELO DINAMICO Y ESTÁTICO	DEL FONDO		*
 ********************************************************************/

///Parámetros fondo para procesado
BGModelParams *BGPrParams = NULL;

STFrame* Procesado( IplImage* frame, StaticBGModel* BGModel,SHModel* Shape ){

	extern double NumFrame;
	struct timeval ti, tif; // iniciamos la estructura
	float tiempoParcial;
	static int update_count = 0;
	// otros parámetros
	int fr = 0;


	STFrame* frameData = NULL;
	tlcde* OldFGFlies = NULL;
	tlcde* FGFlies = NULL;


#ifdef	MEDIR_TIEMPOS
	gettimeofday(&tif, NULL);
	printf("\n1)Procesado:\n");
	gettimeofday(&ti, NULL);
	printf("\t0)Preprocesado \n");
#endif
	AllocateDataProcess( frame );
	ImPreProcess( frame, Imagen, BGModel->ImFMask, 0, BGModel->DataFROI);
#ifdef	MEDIR_TIEMPOS
	tiempoParcial = obtenerTiempo( ti, 0);
	printf("\t\t-Tiempo: %5.4g ms\n", tiempoParcial);
#endif
	// Cargar datos iniciales y establecer parametros
	// Cargamos datos
	if( !frameData ) { //en la primera iteración iniciamos el modelo dinámico al estático
		cvCopy( BGModel->Imed,lastBG);
		cvCopy( BGModel->IDesvf,lastIdes);
	}

	// Iniciar estructura para datos del nuevo frame
	frameData = InitNewFrameData( frame );
	// cargamos las últimas imágenes del frame y del fondo.
	cvCopy( lastBG, frameData->BGModel);
	cvCopy( lastIdes, frameData->IDesvf);
	cvCopy( frame,frameData->Frame);
	cvCopy( Imagen,frameData->ImGray);

	//// BACKGROUND UPDATE
	//	obtener la mascara del FG y la lista con los datos de sus blobs.
	// Actualización de fondo modelo dinámico
#ifdef	MEDIR_TIEMPOS
	gettimeofday(&ti, NULL);
	printf("\t1)Actualización del modelo de fondo:\n");
#endif
	if( update_count == BGPrParams->BG_Update  )
		UpdateBGModel( Imagen,frameData->BGModel,frameData->IDesvf, BGPrParams, BGModel->DataFROI, BGModel->ImFMask );

#ifdef	MEDIR_TIEMPOS
	tiempoParcial = obtenerTiempo( ti, 0);
	printf("\t\t-Tiempo: %5.4g ms\n", tiempoParcial);
	gettimeofday(&ti, NULL);
	printf("\t2)Resta de Fondo \n");
#endif
	/////// BACKGROUND DIFERENCE. Obtención de la máscara del foreground
	crearTrackBarsBGModel();
	BackgroundDifference( Imagen, frameData->BGModel,frameData->IDesvf, frameData->FG ,BGPrParams, BGModel->DataFROI);
	DraWWindow(frameData->FG ,NULL, NULL, SHOW_PROCESS_IMAGES, COMPLETO  );
	if( SHOW_PROCESS_IMAGES ){

	}
#ifdef	MEDIR_TIEMPOS
	tiempoParcial = obtenerTiempo( ti, 0);
	printf("\t\t-Tiempo: %5.4g ms\n", tiempoParcial);
	gettimeofday(&ti, NULL);
	printf("\t3)Segmentación:\n");
#endif
	//// SEGMENTACIÓN

	frameData->Flies = segmentacion2(Imagen, frameData->BGModel,frameData->FG, BGModel->DataFROI, NULL);
	dibujarBGFG( frameData->Flies,frameData->FG,1);
	DraWWindow(frameData->FG,NULL, NULL, SHOW_PROCESS_IMAGES, COMPLETO  );
#ifdef	MEDIR_TIEMPOS
	tiempoParcial = obtenerTiempo( ti, 0);
	printf("\t\t-Tiempo: %5.4g ms\n", tiempoParcial);
	gettimeofday(&ti, NULL);
	printf("\t4)Validación de blobs\n");

#endif

	/////// VALIDACIÓN
	if( frameData->num_frame == 1195 ){
						printf("hola");
					}

	Validacion2(Imagen, frameData , Shape, BGModel, FGMask );


	DraWWindow( frameData->FG,NULL, NULL, SHOW_PROCESS_IMAGES, COMPLETO  );

#ifdef	MEDIR_TIEMPOS
	tiempoParcial = obtenerTiempo( ti, 0);
	printf("\t\t-Tiempo %5.4g ms\n", tiempoParcial);
	gettimeofday(&ti, NULL);
#endif
	/////// ACTUALIZACIÓN SELECTIVA. Modelo de fondo estático.
	//	Obtención de máscara de foreground
	if( update_count == BGPrParams->BG_Update  ){
		cvZero( FGMask);
		cvDilate( frameData->FG, FGMask, 0, 2 );
		cvAdd( BGModel->ImFMask, FGMask, FGMask);
		UpdateBGModel( Imagen, lastBG, lastIdes, BGPrParams, BGModel->DataFROI, FGMask );
		cvCopy( lastBG, frameData->BGModel);
		cvCopy(  lastIdes, frameData->IDesvf);
		update_count = 0;
	}
	else{
		cvCopy( frameData->BGModel, lastBG );
		cvCopy( frameData->IDesvf, lastIdes);
	}
	/////// GUARDAMOS las imagenes definitivas para la siguiente iteración


#ifdef MEDIR_TIEMPOS
	tiempoParcial = obtenerTiempo( ti, 0);
	printf("\t5)Actualización selectiva del Modelo de fondo: %5.4g ms\n", tiempoParcial);
	tiempoParcial = obtenerTiempo( tif , NULL);
	printf("Procesado correcto.Tiempo total %5.4g ms\n", tiempoParcial);
#endif
	update_count++;

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

	FrameData->ImGray = cvCreateImage(size,8,1);
	FrameData->ImKalman = cvCreateImage( size, 8, 3 );
	cvZero( FrameData->Frame );
	cvZero( FrameData->BGModel );
	cvZero( FrameData->FG );
	cvZero( FrameData->IDesvf );
	cvZero( FrameData->ImKalman);
	cvZero( FrameData->ImGray);
	FrameData->Flies = NULL;
	FrameData->Stats = NULL;
	FrameData->Tracks = NULL;
	FrameData->GStats = NULL;

	FrameData->num_frame = (int)NumFrame;

	return FrameData;
}



void SetProcesParams(  ){

    //init parameters
	config_t cfg;
	config_setting_t *setting;
	char settingName[50];
	char configFile[30];
	char settingFather[50];

	int EXITO;
	int DEFAULT = false;

	// Reservar memoria

	BGPrParams = ( BGModelParams *) malloc( sizeof( BGModelParams));
	if ( !BGPrParams ) {error(4);exit (1 );}

	fprintf(stderr, "\nCargando parámetros de Procesado:");
	fprintf(stderr, "\nCargando parámetros de umbralización y limpieza de primer plano...");

	config_init(&cfg);

	sprintf( configFile, "config.cfg");


	 /* Leer archivo. si hay un error, informar y cargar configuración por defecto */
	if(! config_read_file(&cfg, configFile))
	{
		fprintf(stderr, "%s:%d - %s\n", config_error_file(&cfg),
				config_error_line(&cfg), config_error_text(&cfg));

		fprintf(stderr, "Error al acceder al fichero de configuración %s .\n"
						" Estableciendo valores por defecto.\n"
						,configFile);
		DEFAULT = true;
	}
	else
	{
		sprintf( settingFather,"Procesado" );
		setting = config_lookup(&cfg, settingFather);
		/* Si no se se encuentra la setting o bien existe la variable hijo Auto y ésta es true, se establecen TODOS los valores por defecto.*/
		if(setting != NULL)
		{
			sprintf(settingName,"Auto");
			/* Obtener el valor */
			EXITO = config_setting_lookup_bool ( setting, settingName, &DEFAULT);
			if(!EXITO) DEFAULT = true;
			else if( EXITO && DEFAULT ) fprintf(stderr, "\n Opción Auto activada para el campo %s.\n"
												" Estableciendo valores por defecto.\n",settingFather);
			else if( EXITO && !DEFAULT) fprintf(stderr, "\n Opción Auto desactivada para el campo %s.\n"
												" Estableciendo valores del fichero de configuración.\n",settingFather);
		}
		else {
			DEFAULT = true;
			fprintf(stderr, "Error.No se ha podido leer el campo %s.\n"
							" Estableciendo valores por defecto.\n",settingFather);
		}
		sprintf( settingFather,"Procesado.BGModel" );
		setting = config_lookup(&cfg, settingFather);
		/* Si no se se encuentra la setting o bien existe la variable hijo Auto y ésta es true, se establecen TODOS los valores por defecto.*/
		if(setting != NULL)
		{
			sprintf(settingName,"Auto");
			/* Obtener el valor */
			EXITO = config_setting_lookup_bool ( setting, settingName, &DEFAULT);
			if(!EXITO) DEFAULT = true;
			else if( EXITO && DEFAULT ) fprintf(stderr, "\n Opción Auto activada para el campo %s.\n"
												" Estableciendo valores por defecto.\n",settingFather);
			else if( EXITO && !DEFAULT) fprintf(stderr, "\n Opción Auto desactivada para el campo %s.\n"
												" Estableciendo valores del fichero de configuración.\n",settingFather);
		}
		else {
			DEFAULT = true;
			fprintf(stderr, "Error.No se ha podido leer el campo %s.\n"
							" Estableciendo valores por defecto.\n",settingFather);
		}
	}

	if( DEFAULT ) {
		DefaultProcesBGParams( );
	}
	/* Valores leídos del fichero de configuración. Algunos valores puedes ser establecidos por defecto si se indica
	 * expresamente en el fichero de configuración. Si el valor es erroneo o no se encuentra la variable, se establecerán
	 * a los valores por defecto.
	 */
	else{
		double val;
		sprintf(settingName,"MODEL_TYPE");
		if(! config_setting_lookup_int ( setting, settingName, &BGPrParams->MODEL_TYPE )  ){
			 BGPrParams->MODEL_TYPE = MEDIAN_S_UP;
			fprintf(stderr, "No se encuentra la variable %s en el archivo de configuración o el tipo de dato es incorrecto.\n "
							"Establecer por defecto a %d \n",settingName, BGPrParams->MODEL_TYPE );

		}

		sprintf(settingName,"BG_Update");
		if(! config_setting_lookup_int ( setting, settingName, &BGPrParams->BG_Update )  ){
			 BGPrParams->BG_Update = 10;
			fprintf(stderr, "No se encuentra la variable %s en el archivo de configuración o el tipo de dato es incorrecto.\n "
							"Establecer por defecto a %d \n",settingName,BGPrParams->BG_Update);
		}
		else{ // tipo de valor correcto pero erróneo
			if( ( !BGPrParams->BG_Update ) ){
				BGPrParams->BG_Update = 10;
				fprintf(stderr, "El valor de %s está fuera de límites\n "
								"Establecer por defecto %s a %d \n",settingName,settingName,BGPrParams->BG_Update);
			}
		}


		sprintf(settingName,"K");
		if(! config_setting_lookup_float ( setting, settingName, &val   )  ){
			BGPrParams->K = 0.6745;
			fprintf(stderr, "No se encuentra la variable %s en el archivo de configuración o el tipo de dato es incorrecto.\n "
							"Establecer por defecto a %0.6f \n",settingName,BGPrParams->K  );
		}
		else {
			if( ( !val ) ){
				val = 0.6745;
				fprintf(stderr, "El valor de %s está fuera de límites\n "
								"Establecer por defecto %s a %0.6f \n",settingName,settingName,BGPrParams->K);
			}
			BGPrParams->K = (float)val;
		}

		sprintf(settingName,"MORFOLOGIA");
		if(! config_setting_lookup_bool ( setting, settingName, &BGPrParams->MORFOLOGIA )  ){
			BGPrParams->MORFOLOGIA = true;
			fprintf(stderr, "No se encuentra la variable %s en el archivo de configuración o el tipo de dato es incorrecto.\n "
							"Establecer por defecto a %d \n",settingName,BGPrParams->MORFOLOGIA);

		}

		sprintf(settingName,"CVCLOSE_ITR");
		if(! config_setting_lookup_int ( setting, settingName, &BGPrParams->CVCLOSE_ITR )  ){
			BGPrParams->CVCLOSE_ITR = 1;
			fprintf(stderr, "No se encuentra la variable %s en el archivo de configuración o el tipo de dato es incorrecto.\n "
							"Establecer por defecto a %d \n",settingName,BGPrParams->CVCLOSE_ITR);

		}

		sprintf(settingName,"MAX_CONTOUR_AREA");
		if(! config_setting_lookup_int ( setting, settingName, &BGPrParams->MAX_CONTOUR_AREA )  ){
			BGPrParams->MAX_CONTOUR_AREA = 0 ;
			fprintf(stderr, "No se encuentra la variable %s en el archivo de configuración o el tipo de dato es incorrecto.\n "
							"Establecer por defecto a %d \n",settingName,BGPrParams->MAX_CONTOUR_AREA);
		}

		sprintf(settingName,"MIN_CONTOUR_AREA");
		if(! config_setting_lookup_int ( setting, settingName, &BGPrParams->MIN_CONTOUR_AREA )  ){
			BGPrParams->MIN_CONTOUR_AREA = 0; //5
			fprintf(stderr, "No se encuentra la variable %s en el archivo de configuración o el tipo de dato es incorrecto.\n "
							"Establecer por defecto a %d \n",settingName,BGPrParams->MIN_CONTOUR_AREA);

		}

		sprintf(settingName,"HIGHT_THRESHOLD");
		if(! config_setting_lookup_int ( setting, settingName, &BGPrParams->HIGHT_THRESHOLD )  ){
			BGPrParams->HIGHT_THRESHOLD = 20;
			fprintf(stderr, "No se encuentra la variable %s en el archivo de configuración o el tipo de dato es incorrecto.\n "
							"Establecer por defecto a %0.1f \n",settingName,BGPrParams->HIGHT_THRESHOLD);

		}
		else {
			if( ( !BGPrParams->HIGHT_THRESHOLD || BGPrParams->HIGHT_THRESHOLD>255 ) ){
				BGPrParams->HIGHT_THRESHOLD = 20;
				fprintf(stderr, "El valor de %s está fuera de límites\n "
						"Establecer por defecto %s a %d \n",settingName,settingName,BGPrParams->BG_Update);
			}

		}

		sprintf(settingName,"LOW_THRESHOLD");
		if(! config_setting_lookup_int ( setting, settingName, &BGPrParams->LOW_THRESHOLD )  ){
			BGPrParams->LOW_THRESHOLD = 15;
			fprintf(stderr, "No se encuentra la variable %s en el archivo de configuración o el tipo de dato es incorrecto.\n "
							"Establecer por defecto a %d \n",settingName,BGPrParams->LOW_THRESHOLD);
		}
		else {
			if( ( !BGPrParams->LOW_THRESHOLD || BGPrParams->LOW_THRESHOLD>255 ) ){
				BGPrParams->LOW_THRESHOLD = 15;
				fprintf(stderr, "El valor de %s está fuera de límites\n "
								"Establecer por defecto %s a %d \n",settingName,settingName,BGPrParams->BG_Update);
				if( BGPrParams->LOW_THRESHOLD > 50 ) fprintf(stderr, "ADVERTENCIA: El valor de %s es muy elevado\n "
						"Establecer por defecto %s a %d \n",settingName,settingName,BGPrParams->LOW_THRESHOLD);
			}

		}

	}
	ShowProcesBGParams( settingFather );
	SetValidationParams( );

//	ShowStatsParams( settingFather );
	config_destroy(&cfg);
}

void DefaultProcesBGParams( ){

	 BGPrParams->BG_Update = 10;

	 BGPrParams->MODEL_TYPE = MEDIAN_S_UP;

	 BGPrParams->MORFOLOGIA = true;
	 BGPrParams->CVCLOSE_ITR = 1;
	 BGPrParams->MAX_CONTOUR_AREA = 0 ; //200
	 BGPrParams->MIN_CONTOUR_AREA = 0; //5
	 BGPrParams->K = 0.6745;

	 BGPrParams->HIGHT_THRESHOLD = 20;
	 BGPrParams->LOW_THRESHOLD = 15;

}

void ShowProcesBGParams( char* Campo ){

	printf(" \nVariables para el campo %s : \n", Campo);
	printf(" -MODEL_TYPE = %d \n", BGPrParams->MODEL_TYPE);
	printf(" -BG_Update = %d \n",  BGPrParams->BG_Update);
	printf(" -LOW_THRESHOLD = %d \n", BGPrParams->LOW_THRESHOLD);
	printf(" -MORFOLOGIA = %d \n",BGPrParams->MORFOLOGIA);
	printf(" -CVCLOSE_ITR = %d \n", BGPrParams->CVCLOSE_ITR );
	printf(" -MAX_CONTOUR_AREA = %d \n",  BGPrParams->MAX_CONTOUR_AREA);
	printf(" -MIN_CONTOUR_AREA= %d \n", BGPrParams->MIN_CONTOUR_AREA);
	printf(" -HIGHT_THRESHOLD = %d \n", BGPrParams->HIGHT_THRESHOLD);
	printf(" -K = %0.6f \n", BGPrParams->K);

}
void crearTrackBarsBGModel(){

	if(obtenerVisParam( MODE )) {

		cvCreateTrackbar( "LowThreshold","Foreground", &BGPrParams->LOW_THRESHOLD, 100);
		cvCreateTrackbar( "HightThreshold","Foreground", &BGPrParams->HIGHT_THRESHOLD, 100);
		cvCreateTrackbar( "BG Update","Foreground", &BGPrParams->BG_Update, 100);

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

}
void releaseDataProcess( ){
	free(BGPrParams);

	cvReleaseImage( &Imagen );
	cvReleaseImage( &FGMask );
	cvReleaseImage( &lastBG );
	cvReleaseImage( &lastIdes );
	ReleaseDataSegm( );
	ReleaseDataVal( );
}
