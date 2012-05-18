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
	// Crear Modelo de fondo estático
	printf("\n\t1)Creando modelo de Fondo..... ");

	bgmodel = initBGModel( capture , BGParams );
	if(!bgmodel) {error(8); return 0;}
#ifdef	MEDIR_TIEMPOS
	TiempoParcial = obtenerTiempo( ti , 1);
	printf("Hecho. Tiempo empleado: %0.2f seg\n", TiempoParcial);
#endif


	// Crear modelo de forma

	printf("\t2)Creando modelo de forma.....\n ");
#ifdef	MEDIR_TIEMPOS
	gettimeofday(&ti, NULL);
#endif

	shape = ShapeModel( capture, bgmodel, BGParams );
	if(!shape) {error(9); return 0;}
#ifdef	MEDIR_TIEMPOS
	TiempoParcial = obtenerTiempo( ti , 1 );
	printf("Hecho. Tiempo empleado: %0.2f seg\n", TiempoParcial);
#endif
	*BGModel = bgmodel;
	*Shape = shape;

	cvReleaseCapture(&capture);
	free(BGParams);
#ifdef	MEDIR_TIEMPOS
	TiempoTotal = obtenerTiempo( tif , 1);
	printf("\nPreprocesado correcto.Tiempo total empleado: %0.2f s\n", TiempoTotal);
#endif
	//	cvDestroyAllWindows();
	return 1;
}

void SetPreProcesParams(  ){

    //init parameters
	config_t cfg;
	config_setting_t *setting;
	char settingName[50];
	char configFile[30];
	char settingFather[50];

	int EXITO;
	int DEFAULT = false;

	// Reservar memoria
	// Iniciar estructura para parametros del modelo de fondo en primera actualización
	BGParams = ( BGModelParams *) malloc( sizeof( BGModelParams));
	if ( !BGParams ) {error(4);exit (1 );}

	fprintf(stderr, "\nCargando parámetros de Preprocesado:");
	fprintf(stderr, "\nCargando parámetros para Modelo de fondo...");

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
		sprintf( settingFather,"Preprocesado" );
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
		sprintf( settingFather,"Preprocesado.InitBackGroundModel" );
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

		DefaultInitBGModelParams( );
	}
	/* Valores leídos del fichero de configuración. Algunos valores puedes ser establecidos por defecto si se indica
	 * expresamente en el fichero de configuración. Si el valor es erroneo o no se encuentra la variable, se establecerán
	 * a los valores por defecto.
	 */
	else{
		double val;
		 /* Get the store name. */
		sprintf(settingName,"FlatDetection");
		if(! config_setting_lookup_bool ( setting, settingName, &BGParams->FLAT_DETECTION  )  ){
			BGParams->FLAT_DETECTION = true;
			fprintf(stderr, "No se encuentra la variable %s en el archivo de configuración o el tipo de dato es incorrecto.\n "
							"Establecer por defecto a %d \n",settingName,BGParams->FLAT_DETECTION );

		}

		sprintf(settingName,"FLAT_FRAMES_TRAINING");
		if(! config_setting_lookup_int ( setting, settingName, &BGParams->FLAT_FRAMES_TRAINING )  ){
			if( BGParams->FLAT_DETECTION ) BGParams->FLAT_FRAMES_TRAINING = 500;
			else BGParams->FLAT_FRAMES_TRAINING = 0;
			fprintf(stderr, "No se encuentra la variable %s en el archivo de configuración o el tipo de dato es incorrecto.\n "
							"Establecer por defecto a %d \n",settingName,BGParams->FLAT_FRAMES_TRAINING);
		}
		else{ // tipo de valor correcto pero erróneo
			if( (BGParams->FLAT_DETECTION )&& ( BGParams->FLAT_FRAMES_TRAINING <= 0) ){
				BGParams->FLAT_FRAMES_TRAINING = 500;
				fprintf(stderr, "Se ha activado la detección del plato pero el número de frames de entrenamiento es nulo\n "
								"Establecer por defecto %s a %d \n",settingName,BGParams->FLAT_FRAMES_TRAINING);
			}
		}

		sprintf(settingName,"MODEL_TYPE");
		if(! config_setting_lookup_int ( setting, settingName, & BGParams->MODEL_TYPE )  ){
			 BGParams->MODEL_TYPE = MEDIAN_S_UP;
			fprintf(stderr, "No se encuentra la variable %s en el archivo de configuración o el tipo de dato es incorrecto.\n "
							"Establecer por defecto a %d \n",settingName, BGParams->MODEL_TYPE );

		}



		sprintf(settingName,"BG_Update");
		if(! config_setting_lookup_int ( setting, settingName, &BGParams->BG_Update )  ){
			 BGParams->BG_Update = 5;
			fprintf(stderr, "No se encuentra la variable %s en el archivo de configuración o el tipo de dato es incorrecto.\n "
							"Establecer por defecto a %d \n",settingName,BGParams->BG_Update);
		}
		else{ // tipo de valor correcto pero erróneo
			if( ( !BGParams->BG_Update ) ){
				BGParams->BG_Update = 5;
				fprintf(stderr, "El valor de %s está fuera de límites\n "
								"Establecer por defecto %s a %d \n",settingName,settingName,BGParams->BG_Update);
			}
		}

		sprintf(settingName,"Jumps");
		if(! config_setting_lookup_int ( setting, settingName, &BGParams->Jumps  )  ){
			BGParams->Jumps = 4;
			fprintf(stderr, "No se encuentra la variable %s en el archivo de configuración o el tipo de dato es incorrecto.\n "
							"Establecer por defecto a %d \n",settingName,BGParams->Jumps  );
		}

		sprintf(settingName,"initDelay");
		if(! config_setting_lookup_int ( setting, settingName, &BGParams->initDelay )  ){
			BGParams->initDelay = 50;
			fprintf(stderr, "No se encuentra la variable %s en el archivo de configuración o el tipo de dato es incorrecto.\n "
							"Establecer por defecto a %d \n",settingName,BGParams->initDelay);

		}

		sprintf(settingName,"FRAMES_TRAINING");
		if(! config_setting_lookup_int ( setting, settingName, &BGParams->FRAMES_TRAINING )  ){
			BGParams->FRAMES_TRAINING = 700;
			fprintf(stderr, "No se encuentra la variable %s en el archivo de configuración o el tipo de dato es incorrecto.\n "
							"Establecer por defecto a %d \n",settingName,BGParams->FRAMES_TRAINING);

		}

		sprintf(settingName,"INITIAL_DESV");
		if(! config_setting_lookup_float ( setting, settingName, &val )  ){
			BGParams->INITIAL_DESV = 0.05;
			fprintf(stderr, "No se encuentra la variable %s en el archivo de configuración o el tipo de dato es incorrecto.\n "
							"Establecer por defecto a %0.3f \n",settingName,BGParams->INITIAL_DESV);
		}
		else BGParams->INITIAL_DESV = (float)val;

		sprintf(settingName,"K");
		if(! config_setting_lookup_float ( setting, settingName, &val   )  ){
		BGParams->K = 0.6745;
		fprintf(stderr, "No se encuentra la variable %s en el archivo de configuración o el tipo de dato es incorrecto.\n "
						"Establecer por defecto a %0.6f \n",settingName,BGParams->K  );
		}
		else {
			if( ( !val ) ){
				val = 0.6745;
				fprintf(stderr, "El valor de %s está fuera de límites\n "
								"Establecer por defecto %s a %0.6f \n",settingName,settingName,BGParams->K);
			}
			BGParams->K = (float)val;
		}


	}
	ShowInitBGModelParams( settingFather );
	SetShapeParams( BGParams );
//	ShowStatsParams( settingFather );
	config_destroy(&cfg);
}

void DefaultInitBGModelParams( ){

	 BGParams->MODEL_TYPE = MEDIAN_S_UP;
	 BGParams->FLAT_DETECTION = true;
	 BGParams->FLAT_FRAMES_TRAINING = 500;

	 BGParams->BG_Update = 5;
	 BGParams->Jumps = 4;
	 BGParams->initDelay = 50;
	 BGParams->FRAMES_TRAINING = 700;//1500

	 BGParams->INITIAL_DESV = 0.05;
	 BGParams->K = 0.6745;

}

void ShowInitBGModelParams( char* Campo ){

	printf(" \nVariables para el campo %s : \n", Campo);
	printf(" -MODEL_TYPE = %d \n", BGParams->MODEL_TYPE);
	printf(" -FLAT_DETECTION = %d \n",BGParams->FLAT_DETECTION);
	printf(" -FLAT_FRAMES_TRAINING = %d \n", BGParams->FLAT_FRAMES_TRAINING);
	printf(" -BG_Update = %d \n",  BGParams->BG_Update);
	printf(" -Jumps = %d \n", BGParams->Jumps);
	printf(" -initDelay = %d \n", BGParams->initDelay);
	printf(" -FRAMES_TRAINING = %d \n", BGParams->FRAMES_TRAINING );
	printf(" -INITIAL_DESV = %0.6f \n",  BGParams->INITIAL_DESV );
	printf(" -K = %0.6f \n", BGParams->K);

}

void releaseDataPreProcess(){
	free(BGParams);

}
