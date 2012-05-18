/*
 * ShapeModel.cpp
 *
 *  Created on: 19/09/2011
 *      Author: chao
 */

#include "ShapeModel.hpp"

ShapeParams* ShParams = NULL;

SHModel* ShapeModel( CvCapture* g_capture,StaticBGModel* BGModel , BGModelParams* BGParams){

	int num_frames = 0;
	int total_blobs=0;
	int k=0;
	float Dif=0;
	float MaxContourArea = 0;
	float MaxContourPerimeter = 0;

	float areas[ShParams->FramesTraining*15]; // almacena las areas de todos los blobs encontrados

	IplImage* frame = NULL;

	STFrame* frameData = NULL;
	SHModel* Shape = NULL;

	CBlobResult blobs;
	CBlob *currentBlob;

	IplImage* ImGris = cvCreateImage(cvGetSize( BGModel->Imed ), 8, 1 );
	IplImage* Imblob = cvCreateImage(cvGetSize( BGModel->Imed ), 8, 3 );
	IplImage* lastBG = cvCreateImage( cvGetSize( BGModel->Imed ),8, 1 );
	IplImage* lastIdes = cvCreateImage( cvGetSize( BGModel->Imed ), IPL_DEPTH_32F, 1);
	cvZero(Imblob);
	// Iniciar estructura para modelo de forma

	Shape = ( SHModel *) malloc( sizeof( SHModel));
	if ( !Shape ) {error(4);return 0;}
	Shape->FlyAreaDes = 0;
	Shape->FlyAreaMed = 0;
	Shape->FlyAreaMedia=0;
	//Pone a 0 los valores del vector areas

	for(int i=0;i<ShParams->FramesTraining*15;i++){
		areas[i]=0;
	}

	//EXTRACCION DE LOS BLOBS Y CALCULO DE MEDIANA/MEDIA Y DESVIACION TIPICA PARA TODOS LOS FRAMES
	cvSetCaptureProperty( g_capture,1,BGParams->initDelay ); // establecemos la posición
	while( num_frames < ShParams->FramesTraining ){
		frame = cvQueryFrame( g_capture );
		if ( !frame ) {
			error(2);
			break;
		}
		if ( (cvWaitKey(10) & 255) == 27 ) break;

		ImPreProcess( frame, ImGris, BGModel->ImFMask, 0, BGModel->DataFROI);

		// Cargamos datos del fondo
		if(!frameData ) { //en la primera iteración iniciamos el modelo dinamico al estático
			// Iniciar estructura para datos del nuevo frame
			frameData = InitNewFrameData( frame );
			cvCopy(  BGModel->Imed,frameData->BGModel);
			cvSet(frameData->IDesvf, cvScalar(1));
			cvCopy(  BGModel->Imed,lastBG);
		}
		else{	// cargamos los últimos parámetros del fondo.
			cvCopy( lastBG, frameData->BGModel);
			cvCopy( lastIdes,frameData->IDesvf );
		}
	//	obtener la mascara del FG y la lista con los datos de sus blobs.
		//// BACKGROUND UPDATE
		// Actualización del fondo
		// establecer parametros

		UpdateBGModel( ImGris,frameData->BGModel,frameData->IDesvf, BGParams, BGModel->DataFROI, BGModel->ImFMask );
		/////// BACKGROUND DIFERENCE. Obtención de la máscara del foreground
		BackgroundDifference( ImGris, frameData->BGModel,frameData->IDesvf, frameData->FG ,BGParams, BGModel->DataFROI);

		// guardamos las imagenes para iniciar el siguiente frame
		cvCopy( frameData->BGModel, lastBG);
		cvCopy(  frameData->IDesvf,lastIdes);

		//Obtener los Blobs y excluir aquellos que no interesan por su tamaño
		cvSetImageROI(  frameData->FG , BGModel->DataFROI);

		blobs = CBlobResult( frameData->FG, NULL, 100, true );
		blobs.Filter( blobs, B_EXCLUDE, CBlobGetArea(),B_GREATER,100);
		blobs.Filter( blobs, B_EXCLUDE, CBlobGetPerimeter(),B_GREATER,1000);

		int j = blobs.GetNumBlobs();//numero de blos encontrados en el frame

		total_blobs=total_blobs+j; // Contabiliza los blobs encontrados para todos los frames

		// Guardar los Blobs en un archivo txt (OPCIONAL)

		//Recorrer Blob a blob y obtener las caracteristicas del AREA de cada uno de ellos

		for (int i = 0; i < blobs.GetNumBlobs(); i++ ){ //for 1

			currentBlob = blobs.GetBlob(i);

			CBlobGetArea();

			areas[k]=currentBlob->area;// almacenar el valor del area del blob en el vector

			// Mediana de las Areas

			if(currentBlob->area < Shape->FlyAreaMed) Shape->FlyAreaMed=Shape->FlyAreaMed-1;
			if(currentBlob->area > Shape->FlyAreaMed) Shape->FlyAreaMed=Shape->FlyAreaMed+1;

			//Media de las Areas

			Shape->FlyAreaMedia=Shape->FlyAreaMedia+currentBlob->area;//sumatorio de los valores de las areas

			if(ShParams->SHOW_DATA_AREAS) {

				printf("Area blob %d = %f ",i,currentBlob->area);
			}

			currentBlob->FillBlob( Imblob, CV_RGB(255,0,0));

			k++;//incrementar indice del vector que contiene las areas

		}//Fin del For 1
		if(ShParams->SHOW_DATA_AREAS) printf("\n");
		num_frames += 1;
		cvResetImageROI(frameData->FG);

		DraWWindow(Imblob, frameData, BGModel, SHOW_SHAPE_MODELING, COMPLETO);
		DraWWindow(Imblob, frameData, BGModel, SHAPE,SIMPLE );

	}
	desvanecer( NULL, 20);
	Shape->FlyAreaMedia=Shape->FlyAreaMedia/total_blobs;// Media de las Areas para cada frame

	//Calcular la desvición típica

	for(int l=0;l<ShParams->FramesTraining*15;l++){ // For 2

		Dif=abs(areas[l]-Shape->FlyAreaMed);// valor del area - mediana

		int valor=areas[l];

		if(valor == 0) break;

		if(Dif < Shape->FlyAreaDes) Shape->FlyAreaDes=Shape->FlyAreaDes-1;
		if(Dif > Shape->FlyAreaDes) Shape->FlyAreaDes=Shape->FlyAreaDes+1;

	} // Fin del For 2

	//Mostrar mediana y media para todos los frames

	if(ShParams->SHOW_DATA_AREAS )
		printf("\n MEDIANA AREAS: %f \t MEDIA AREAS: %f \t DESVIACION AREAS: %f",Shape->FlyAreaMed,Shape->FlyAreaMedia,Shape->FlyAreaDes);

	free( ShParams);
	liberarSTFrame( frameData );
	cvReleaseImage( &ImGris);
	cvReleaseImage( &Imblob);
	cvReleaseImage( &lastIdes);
	cvReleaseImage( &lastBG);

	return Shape;

}//Fin de la función ShapeModel2


void SetShapeParams(  BGModelParams* BGParams ){

    //init parameters
	config_t cfg;
	config_setting_t *setting;
	char settingName[30];
	char configFile[30];
	char settingFather[30];

	int EXITO;
	int DEFAULT = false;

	// Reservar memoria
	// Iniciar estructura para parametros del modelo de fondo en primera actualización
	if(!ShParams){
		ShParams = ( ShapeParams * )malloc( sizeof(ShapeParams));
		if( !ShParams ) {error(4);exit(1);}

	}

	config_init(&cfg);
	fprintf(stderr, "\nCargando parámetros Modelo de Forma:");
	fprintf(stderr, "\nCargando parámetros de umbralización y limpieza de primer plano...");

	sprintf( configFile, "config.cfg");
	sprintf( settingFather,"Preprocesado" );
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
		sprintf( settingFather,"Preprocesado.ShapeModel" );
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

	if( DEFAULT ) SetDefaultShapeParams(   BGParams );
	/* Valores leídos del fichero de configuración. Algunos valores puedes ser establecidos por defecto si se indica
	 * expresamente en el fichero de configuración. Si el valor es erroneo o no se encuentra la variable, se establecerán
	 * a los valores por defecto.
	 */
	else{



		sprintf(settingName,"MORFOLOGIA");
		if(! config_setting_lookup_bool ( setting, settingName, &BGParams->MORFOLOGIA )  ){
			BGParams->MORFOLOGIA = true;
			fprintf(stderr, "No se encuentra la variable %s en el archivo de configuración o el tipo de dato es incorrecto.\n "
							"Establecer por defecto a %d \n",settingName,BGParams->MORFOLOGIA);

		}

		sprintf(settingName,"CVCLOSE_ITR");
		if(! config_setting_lookup_int ( setting, settingName, &BGParams->CVCLOSE_ITR )  ){
			BGParams->CVCLOSE_ITR = 1;
			fprintf(stderr, "No se encuentra la variable %s en el archivo de configuración o el tipo de dato es incorrecto.\n "
							"Establecer por defecto a %d \n",settingName,BGParams->CVCLOSE_ITR);

		}

		sprintf(settingName,"MAX_CONTOUR_AREA");
		if(! config_setting_lookup_int ( setting, settingName, &BGParams->MAX_CONTOUR_AREA )  ){
		BGParams->MAX_CONTOUR_AREA = 0 ;
		fprintf(stderr, "No se encuentra la variable %s en el archivo de configuración o el tipo de dato es incorrecto.\n "
						"Establecer por defecto a %d \n",settingName,BGParams->MAX_CONTOUR_AREA);
		}

		sprintf(settingName,"MIN_CONTOUR_AREA");
		if(! config_setting_lookup_int ( setting, settingName, &BGParams->MIN_CONTOUR_AREA )  ){
			BGParams->MIN_CONTOUR_AREA = 0; //5
			fprintf(stderr, "No se encuentra la variable %s en el archivo de configuración o el tipo de dato es incorrecto.\n "
							"Establecer por defecto a %d \n",settingName,BGParams->MIN_CONTOUR_AREA);

		}

		sprintf(settingName,"HIGHT_THRESHOLD");
		if(! config_setting_lookup_int ( setting, settingName, &BGParams->HIGHT_THRESHOLD )  ){
			BGParams->HIGHT_THRESHOLD = 20;
			fprintf(stderr, "No se encuentra la variable %s en el archivo de configuración o el tipo de dato es incorrecto.\n "
							"Establecer por defecto a %0.1f \n",settingName,BGParams->HIGHT_THRESHOLD);

		}
		else {
			if( ( !BGParams->HIGHT_THRESHOLD || BGParams->HIGHT_THRESHOLD>255 ) ){
				BGParams->HIGHT_THRESHOLD = 20;
				fprintf(stderr, "El valor de %s está fuera de límites\n "
						"Establecer por defecto %s a %d \n",settingName,settingName,BGParams->BG_Update);
			}

		}

		sprintf(settingName,"LOW_THRESHOLD");
		if(! config_setting_lookup_int ( setting, settingName, &BGParams->LOW_THRESHOLD )  ){
			BGParams->LOW_THRESHOLD = 15;
			fprintf(stderr, "No se encuentra la variable %s en el archivo de configuración o el tipo de dato es incorrecto.\n "
							"Establecer por defecto a %d \n",settingName,BGParams->LOW_THRESHOLD);
		}
		else {
			if( ( !BGParams->LOW_THRESHOLD || BGParams->LOW_THRESHOLD>255 ) ){
				BGParams->LOW_THRESHOLD = 15;
				fprintf(stderr, "El valor de %s está fuera de límites\n "
								"Establecer por defecto %s a %d \n",settingName,settingName,BGParams->BG_Update);
				if( BGParams->LOW_THRESHOLD > 50 ) fprintf(stderr, "ADVERTENCIA: El valor de %s es muy elevado\n "
						"Establecer por defecto %s a %d \n",settingName,settingName,BGParams->LOW_THRESHOLD);
			}

		}

		fprintf(stderr, "\nCargando parámetros específicos del modelo de forma...");

		 /* Get the store name. */
		sprintf(settingName,"FramesTraining");
		if(! config_setting_lookup_int ( setting, settingName, &ShParams->FramesTraining )  ){

			ShParams->FramesTraining = 200;
			fprintf(stderr, "No se encuentra la variable %s en el archivo de configuración o el tipo de dato es incorrecto.\n "
							"Establecer por defecto a %d \n",settingName,ShParams->FramesTraining);
		}
		else{ // tipo de valor correcto pero erróneo
			if( ( ShParams->FramesTraining <= 0) ){
				ShParams->FramesTraining = 200;
				fprintf(stderr, "Se ha activado la detección del plato pero el número de frames de entrenamiento es nulo\n "
								"Establecer por defecto %s a %d \n",settingName,ShParams->FramesTraining);
			}
		}

		sprintf(settingName,"Max_Area");
		if(! config_setting_lookup_int ( setting, settingName, &ShParams->Max_Area   )  ){
			ShParams->Max_Area = 100;
		fprintf(stderr, "No se encuentra la variable %s en el archivo de configuración o el tipo de dato es incorrecto.\n "
						"Establecer por defecto a %d \n",settingName,ShParams->Max_Area  );
		}

		sprintf(settingName,"SHOW_DATA_AREAS");
		if(! config_setting_lookup_bool ( setting, settingName, &ShParams->SHOW_DATA_AREAS   )  ){
			ShParams->SHOW_DATA_AREAS = false;
			fprintf(stderr, "No se encuentra la variable %s en el archivo de configuración o el tipo de dato es incorrecto.\n "
						"Establecer por defecto a %d \n",settingName,ShParams->SHOW_DATA_AREAS  );
		}

		sprintf(settingName,"Max_Perimeter");
		if(! config_setting_lookup_int ( setting, settingName, &ShParams->Max_Perimeter  )  ){
			ShParams->Max_Perimeter = 1000;
		fprintf(stderr, "No se encuentra la variable %s en el archivo de configuración o el tipo de dato es incorrecto.\n "
						"Establecer por defecto a %d \n",settingName,ShParams->Max_Perimeter );

		}

	}


	ShowShapeParams( settingFather, BGParams );
	config_destroy(&cfg);
}

void SetDefaultShapeParams(  BGModelParams* BGParams ){

	/////////////// Inicializar /////////////////
	BGParams->LOW_THRESHOLD = 15;

	/// PARAMETROS PARA LA LIMPIEZA DEL FG

	BGParams->MORFOLOGIA = true;
	BGParams->CVCLOSE_ITR = 1;
	BGParams->MAX_CONTOUR_AREA = 0 ;
	BGParams->MIN_CONTOUR_AREA = 0;
	BGParams->HIGHT_THRESHOLD = 20;
	/// PARAMETROS PARA MODELO DE FORMA
	ShParams->SHOW_DATA_AREAS = false;
	ShParams->FramesTraining = 200;
	ShParams->Max_Area = 100;
	ShParams->Max_Perimeter = 1000;
}

void ShowShapeParams( char* Campo, BGModelParams* BGParams ){

	printf(" \nVariables para el campo %s : \n", Campo);
	printf(" -LOW_THRESHOLD = %d \n", BGParams->LOW_THRESHOLD);
	printf(" -MORFOLOGIA = %d \n",BGParams->MORFOLOGIA);
	printf(" -CVCLOSE_ITR = %d \n", BGParams->CVCLOSE_ITR );
	printf(" -MAX_CONTOUR_AREA = %d \n",  BGParams->MAX_CONTOUR_AREA);
	printf(" -MIN_CONTOUR_AREA= %d \n", BGParams->MIN_CONTOUR_AREA);
	printf(" -HIGHT_THRESHOLD = %d \n", BGParams->HIGHT_THRESHOLD);
	printf(" -SHOW_DATA_AREAS = %d \n", ShParams->SHOW_DATA_AREAS);
	printf(" -FramesTraining = %d \n", ShParams->FramesTraining );
	printf(" -Max_Area  = %d \n",  ShParams->Max_Area );
	printf(" -Max_Perimeter = %d \n", ShParams->Max_Perimeter);

}
