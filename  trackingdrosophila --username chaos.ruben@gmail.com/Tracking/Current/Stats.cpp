/*
 * Stats.cpp
 *
 *  Tenemos un proceso estocástico , es decir, una serie de variables aleatorias ordenadas en el tiempo (serie temporal)
 *  En esta función se calcularán una serie de parámetros estadísticos.
 *  Por un lado se calcularán estadísticas de los blobs en conjunto y por otro lado de cada blob de forma individual.
 *
 *  En cálculo de estadísticas de blobs en conjunto obtendremos la cantidad de movimiento medio u su desviación en distintos intervalos
 *  de tiempo. Se usarán medias móviles. Para ello se mantiene una cola FIFO de una dimensión tal que almacena 8 horas de datos
 *
 *
 *
 *  Created on: 31/01/2012
 *      Author: chao
 */

#include "Stats.hpp"


STStatsParams* statsParams = NULL;
STStatsCoef* Coef = NULL;
tlcde* vectorSumFr = NULL;
STStatFrame* Stats = NULL;

StatsCoefs* Val = NULL;


void CalcStatsFrame(STFrame* frameDataOut, ConvUnits* convUnits ){


	if( !frameDataOut) return;
#ifdef MEDIR_TIEMPOS
	gettimeofday(&ti, NULL);
	printf( "Rastreo finalizado con éxito ..." );
	printf( "Comenzando análisis estadístico de los datos obtenidos ...\n" );
	printf("\n3)Cálculo de estadísticas en tiempo de ejecución:\n");

#endif

	 if( statsParams->CalcStatsMov) {
		 frameDataOut->Stats = InitStatsFrame(  frameDataOut->GStats->fps );
		 // Blobs en movimiento
		 statsBloB( frameDataOut );
		 // cálculos estadísticos del movimiento de los blobs en conjunto
		 statsBlobS( frameDataOut );
		 // Normalizar
		 normalizeStatsBlobS( convUnits->pfTOmms); // pasar a unidades del sistema internacional
		 normalizeStatsFlies( frameDataOut->Flies, convUnits );
	 }

	if( statsParams->MuestrearLinea ){
		int Cx = frameDataOut->BGModel->width/2;
		int Cy = frameDataOut->BGModel->height/2;
		muestrearLinea( frameDataOut->ImGray,cvPoint( Cx - 50, Cy ),	cvPoint( Cx + 50, Cy ), 1000, frameDataOut->num_frame);
	}

#ifdef MEDIR_TIEMPOS
	printf( "Análisis finalizado ...\n" );
	TiempoParcial = obtenerTiempo( ti , NULL);
	printf("Cálculos realizados. Tiempo total %5.4g ms\n", TiempoParcial);
#endif
}

STGlobStatF* SetGlobalStats( int NumFrame, timeval tif, timeval tinicio, int TotalFrames, float FPS  ){
	//FrameData->Stats->totalFrames = 0;
	// FRAME
	STGlobStatF* GStats = NULL;
	GStats = ( STGlobStatF *) malloc( sizeof(STGlobStatF));
	if(!GStats) {error(4); return 0;}

	GStats->TiempoFrame = obtenerTiempo( tif, 0 );
	GStats->TiempoGlobal = obtenerTiempo( tinicio, 1);
	GStats->numFrame = NumFrame;
	GStats->fps = FPS;
	GStats->totalFrames = TotalFrames;

	return GStats;

}

STStatFrame* InitStatsFrame(  int fps){

//	// FRAME
//	Stats->TiempoFrame = obtenerTiempo( tif, 0 );
//	Stats->TiempoGlobal = obtenerTiempo( tinicio, 1);
//	Stats->totalFrames = TotalFrames;
//	Stats->numFrame = NumFrame;
//	Stats->fps = FPS;
	// BLOBS
	Stats->TProces = 0;
	Stats->TTacking= 0;
	Stats->staticBlobs= 0; //!< blobs estáticos en tanto por ciento.
	Stats->dinamicBlobs= 0; //!< blobs en movimiento en tanto por ciento
	Stats->TotalBlobs= 0; //!< Número total de blobs.

	Stats->CMov1SMed = 0;  //!< Cantidad de movimiento medio en los últimos 30 seg.
	Stats->CMov1SDes = 0;
	Stats->CMov30SMed = 0;  //!< Cantidad de movimiento medio en los últimos 30 seg.
	Stats->CMov30SDes = 0;
	Stats->CMov1Med = 0;  //!< Cantidad de movimiento medio en el último min.
	Stats->CMov1Des = 0;
	Stats->CMov10Med = 0;  //!< Cantidad de movimiento medio en los últimos 10 min.
	Stats->CMov10Des = 0;
	Stats->CMov1HMed = 0;  //!< Cantidad de movimiento medio en la última hora.
	Stats->CMov1HDes = 0;
	Stats->CMov2HMed = 0;	//!< Cantidad de movimiento medio en  últimas 2 horas.
	Stats->CMov2HDes = 0;
	Stats->CMov4HMed = 0;
	Stats->CMov4HDes = 0;
	Stats->CMov8HMed = 0; //!<//!< Cantidad de movimiento medio en  últimas 2 horas.
	Stats->CMov8HDes = 0;
	Stats->CMov16HMed = 0;
	Stats->CMov16HDes = 0;
	Stats->CMov24HMed = 0;
	Stats->CMov24HDes = 0;
	Stats->CMov48HMed = 0;
	Stats->CMov48HDes = 0;
	Stats->CMovMed = 0;
	Stats->CMovDes = 0;

	return Stats;
}

void statsBloB(STFrame* frameDataOut ){

	STFly* fly = NULL;
	frameDataOut->Stats->TotalBlobs = frameDataOut->Flies->numeroDeElementos;
	for(int i = 0; i< frameDataOut->Stats->TotalBlobs ; i++){
		fly = (STFly*)obtener(i,frameDataOut->Flies);
		if( fly->Estado == 1 ) frameDataOut->Stats->dinamicBlobs +=1;
		else frameDataOut->Stats->staticBlobs +=1;

		// velocidad instantánea
//		EUDistance( fly->Vx, fly->Vy, NULL, &fly->Stats->VInst);
	}
	frameDataOut->Stats->dinamicBlobs = (frameDataOut->Stats->dinamicBlobs/frameDataOut->Stats->TotalBlobs)*100;
	frameDataOut->Stats->staticBlobs = (frameDataOut->Stats->staticBlobs/ frameDataOut->Stats->TotalBlobs)*100;



}
/*!\brief Calcula las medias y varianzas moviles para la cantidad de movimiento
 *
 * @param frameDataStats
 * @param frameDataOut
 */
void statsBlobS( STFrame* frameDataOut ){



	valorSum* valor = NULL;

	if(frameDataOut->Flies->numeroDeElementos == 0 ) return; // si no hay datos del frame, no computar

	// Obtener el nuevo valor
	valor =  ( valorSum *) malloc( sizeof(valorSum));
	valor->sum = sumFrame( frameDataOut ->Flies );
	anyadirAlFinal( valor, vectorSumFr );

	// Si es el primer elemento, iniciar medias
	if(vectorSumFr->numeroDeElementos == 1){
		iniciarMedias( frameDataOut->Stats, valor->sum );
		return;
	}

	// movimiento global. Calculo de medias y desviaciones
	mediaMovil( Coef, vectorSumFr, frameDataOut->Stats );


	if( (unsigned)vectorSumFr->numeroDeElementos == Coef->FMax){
		valor = (valorSum*)liberarPrimero( vectorSumFr );
		free(valor);
	}

}
// Calcula los coeficientes para hayar las medias y desviaciones de la serie temporal
void calcCoef( int FPS ){


	Coef->SumFrame = 0;

	Coef->F1S = FPS ;
	Coef->F30S = 30*FPS ;	//!< frames para calcular la media de 30 s
	Coef->F1  =  60*FPS;
	Coef->F10 = 600*FPS;
	Coef->F1H = 3600*FPS;
	Coef->F2H = 7200*FPS;
	Coef->F4H = 14400*FPS;
	Coef->F8H = 28800*FPS;
	Coef->F16H = 57600*FPS;
	Coef->F24H = 115200*FPS;
	Coef->F48H = 234000*FPS;
	Coef->FTot = 0;
	if( statsParams->MaxBufferTime == 0) statsParams->MaxBufferTime = 234000;
	Coef->FMax = statsParams->MaxBufferTime*FPS;
}

unsigned int sumFrame( tlcde* Flies ){

	STFly* flyOut = NULL;
	float sumatorio = 0;
	unsigned int sumUint = 0;

	if( !Flies ) return sumUint;
	else if( Flies->numeroDeElementos == 0) return sumUint;
	else{
		for(int i = 0; i< Flies->numeroDeElementos ; i++){
			flyOut = (STFly*)obtener(i,Flies);
			sumatorio = flyOut->Vmed + sumatorio;
		}
	}
	sumUint = ( unsigned int)cvRound(sumatorio);
	return sumUint;
}

void mediaMovil(  STStatsCoef* Coef, tlcde* vector, STStatFrame* Stats ){

	valorSum* valor;
	valorSum* valorT;

	// Se suma el nuevo valor
	valorT = ( valorSum*)obtener( vectorSumFr->numeroDeElementos-1 , vectorSumFr );


	Coef->F1SSum = Coef->F1SSum + valorT->sum;
	Coef->F1SSum2 = Coef->F1SSum2 + valorT->sum*valorT->sum;

	Coef->F30SSum = Coef->F30SSum + valorT->sum;
	Coef->F30SSum2 = Coef->F30SSum2 + valorT->sum*valorT->sum;

	Coef->F1Sum = Coef->F1Sum + valorT->sum;
	Coef->F1Sum2 = Coef->F1Sum + valorT->sum*valorT->sum;

	Coef->F10Sum = Coef->F10Sum + valorT->sum;
	Coef->F10Sum2 = Coef->F10Sum2 +valorT->sum*valorT->sum;

	Coef->F1HSum = Coef->F1HSum + valorT->sum;
	Coef->F1HSum2 = Coef->F1HSum2 + valorT->sum*valorT->sum;

	Coef->F2HSum = Coef->F2HSum + valorT->sum;
	Coef->F2HSum2 = Coef->F2HSum2 + valorT->sum*valorT->sum;

	Coef->F4HSum = Coef->F4HSum + valorT->sum;
	Coef->F4HSum2 = Coef->F4HSum2 + valorT->sum*valorT->sum;

	Coef->F8HSum = Coef->F48HSum + valorT->sum;
	Coef->F8HSum2 = Coef->F48HSum2 + valorT->sum*valorT->sum;

	Coef->MediaSum = Coef->MediaSum + valorT->sum;
	Coef->MediaSum2 = Coef->MediaSum2 + valorT->sum*valorT->sum;

	Coef->FTot++;
	Stats->CMovMed   =  Coef->MediaSum / Coef->FTot; // cálculo de la media
	Stats->CMovDes  =  sqrt( abs(Coef->MediaSum2 / Coef->FTot - Stats->CMovMed*Stats->CMovMed ) ); // cálculo de la desviación


	if((unsigned)vector->numeroDeElementos > Coef->F1S){
	// los que hayan alcanzado el valor máximo restamos al sumatorio el primer valor
		valor = ( valorSum*)obtener( vectorSumFr->numeroDeElementos-1 - Coef->F1S, vectorSumFr );
		Coef->F1SSum = Coef->F1SSum - valor->sum; // sumatorio para la media
		Coef->F1SSum2 = Coef->F1SSum2 - (valor->sum*valor->sum);  // sumatorio para la varianza
		Stats->CMov1SMed   =  Coef->F1SSum / Coef->F1S; // cálculo de la media
		Stats->CMov1SDes  =  sqrt( abs(Coef->F1SSum2 / Coef->F1S - Stats->CMov1SMed*Stats->CMov1SMed ) ); // cálculo de la desviación
	}
	if((unsigned)vector->numeroDeElementos > Coef->F30S){
		valor = ( valorSum*)obtener( vectorSumFr->numeroDeElementos-1 - Coef->F30S, vectorSumFr );
		Coef->F30SSum = Coef->F30SSum - valor->sum; // sumatorio para la media
		Coef->F30SSum2 = Coef->F30SSum2 - (valor->sum*valor->sum);  // sumatorio para la varianza
		Stats->CMov30SMed  =  Coef->F30SSum / Coef->F30S;
		Stats->CMov30SDes  =  sqrt( abs(Coef->F30SSum2 / Coef->F30S - Stats->CMov30SMed*Stats->CMov30SMed ));
	}
	if((unsigned)vector->numeroDeElementos > Coef->F1){
		valor = ( valorSum*)obtener( vectorSumFr->numeroDeElementos-1 - Coef->F1, vectorSumFr );
		Coef->F1Sum = Coef->F1Sum - valor->sum;
		Coef->F1Sum2 = Coef->F1Sum2 - (valor->sum*valor->sum);
		Stats->CMov1Med    =  Coef->F1Sum / Coef->F1;
		Stats->CMov1Des    =  sqrt( abs(Coef->F1Sum2 / Coef->F1 -  Stats->CMov1Med *Stats->CMov1Med )) ;
	}
	if((unsigned)vector->numeroDeElementos > Coef->F10){
		valor = ( valorSum*)obtener( vectorSumFr->numeroDeElementos-1 - Coef->F10, vectorSumFr );
		Coef->F10Sum = Coef->F10Sum - valor->sum;
		Coef->F10Sum2 = Coef->F10Sum2 - (valor->sum*valor->sum);
		Stats->CMov10Med   =  Coef->F10Sum / Coef->F10;
		Stats->CMov10Des   =  sqrt( abs(Coef->F10Sum2 / Coef->F10 - Stats->CMov10Med*Stats->CMov10Med));
	}
	if((unsigned)vector->numeroDeElementos > Coef->F1H){
		valor = ( valorSum*)obtener( vectorSumFr->numeroDeElementos-1 - Coef->F1H, vectorSumFr );
		Coef->F1HSum = Coef->F1HSum - valor->sum;
		Coef->F1HSum2 = Coef->F1HSum2 - (valor->sum*valor->sum);
		Stats->CMov1HMed   =  Coef->F1HSum / Coef->F1H;
		Stats->CMov1HDes   =  sqrt( abs(Coef->F1HSum2 / Coef->F1H - Stats->CMov1HMed*Stats->CMov1HMed));
	}
	if((unsigned)vector->numeroDeElementos > Coef->F2H){
		valor = ( valorSum*)obtener( vectorSumFr->numeroDeElementos-1 - Coef->F2H, vectorSumFr );
		Coef->F2HSum = Coef->F2HSum - valor->sum;
		Coef->F2HSum2 = Coef->F2HSum2 - (valor->sum*valor->sum);
		Stats->CMov2HMed   =  Coef->F2HSum / Coef->F2H;
		Stats->CMov2HDes   =  sqrt( abs(Coef->F2HSum2 / Coef->F2H - Stats->CMov2HMed*Stats->CMov2HMed));
	}
	if((unsigned)vector->numeroDeElementos > Coef->F4H){
		valor = ( valorSum*)obtener( vectorSumFr->numeroDeElementos-1 - Coef->F4H, vectorSumFr );
		Coef->F4HSum = Coef->F4HSum - valor->sum;
		Coef->F4HSum2 = Coef->F4HSum2- (valor->sum*valor->sum);
		Stats->CMov4HMed   =  Coef->F4HSum / Coef->F4H;
		Stats->CMov4HDes   =  sqrt( abs(Coef->F4HSum2 / Coef->F4H - Stats->CMov4HMed*Stats->CMov4HMed));
	}
	if((unsigned)vector->numeroDeElementos > Coef->F8H){
		valor = ( valorSum*)obtener( vectorSumFr->numeroDeElementos-1 - Coef->F8H, vectorSumFr );
		Coef->F8HSum = Coef->F48HSum - valor->sum;
		Coef->F8HSum2 = Coef->F8HSum2 - (valor->sum*valor->sum);
		Stats->CMov8HMed   = Coef->F8HSum / Coef->F8H;
		Stats->CMov8HDes   =  sqrt( abs(Coef->F8HSum2 / Coef->F8H - Stats->CMov8HMed*Stats->CMov8HMed));
	}
	if((unsigned)vector->numeroDeElementos > Coef->F16H){
		valor = ( valorSum*)obtener( vectorSumFr->numeroDeElementos-1 - Coef->F16H, vectorSumFr );
		Coef->F16HSum = Coef->F16HSum - valor->sum;
		Coef->F16HSum2 = Coef->F16HSum2 - (valor->sum*valor->sum);
		Stats->CMov16HMed   = Coef->F16HSum / Coef->F16H;
		Stats->CMov16HDes  =  sqrt( abs(Coef->F16HSum2 / Coef->F16H - Stats->CMov16HMed*Stats->CMov16HMed));
	}
	if((unsigned)vector->numeroDeElementos > Coef->F24H){
		valor = ( valorSum*)obtener( vectorSumFr->numeroDeElementos-1 - Coef->F24H, vectorSumFr );
		Coef->F24HSum = Coef->F24HSum - valor->sum;
		Coef->F24HSum2 = Coef->F24HSum2 - (valor->sum*valor->sum);
		Stats->CMov24HMed   = Coef->F24HSum / Coef->F24H;
		Stats->CMov24HDes  =  sqrt( abs(Coef->F24HSum2 / Coef->F24H - Stats->CMov24HMed*Stats->CMov24HMed));
	}
	if((unsigned)vector->numeroDeElementos > Coef->F48H){
		valor = ( valorSum*)obtener( vectorSumFr->numeroDeElementos-1 - Coef->F48H, vectorSumFr );
		Coef->F48HSum = Coef->F48HSum - valor->sum;
		Coef->F48HSum2 = Coef->F48HSum2 - (valor->sum*valor->sum);
		Stats->CMov48HMed   = Coef->F48HSum / Coef->F48H;
		Stats->CMov48HDes  =  sqrt( abs(Coef->F48HSum2 / Coef->F48H - Stats->CMov48HMed*Stats->CMov48HMed));
	}

}

void iniciarMedias( STStatFrame* Stats, unsigned int valor ){

	Coef->F30SSum = 0;
	Coef->F1Sum   = 0;
	Coef->F10Sum  = 0;
	Coef->F1HSum  = 0;
	Coef->F2HSum  = 0;
	Coef->F4HSum  = 0;
	Coef->F8HSum  = 0;
	Coef->F16HSum = 0;
	Coef->F24HSum = 0;
	Coef->F48HSum = 0;
	Coef->F30SSum2 = 0;
	Coef->F1Sum2   = 0;
	Coef->F10Sum2  = 0;
	Coef->F1HSum2  = 0;
	Coef->F2HSum2  = 0;
	Coef->F4HSum2  = 0;
	Coef->F8HSum2  = 0;
	Coef->F16HSum2 = 0;
	Coef->F24HSum2 = 0;
	Coef->F48HSum2 = 0;
	Coef->MediaSum = 0;
	Coef->MediaSum2 = 0;

}

void normalizeStatsBlobS( float k){


	Stats->CMov1SMed = Stats->CMov1SMed * k;  //!< Cantidad de movimiento medio en los últimos 30 seg.
	Stats->CMov1SDes = Stats->CMov1SDes * k;
	Stats->CMov30SMed = Stats->CMov30SMed * k;  //!< Cantidad de movimiento medio en los últimos 30 seg.
	Stats->CMov30SDes =Stats->CMov30SDes * k;
	Stats->CMov1Med = Stats->CMov1Med * k;  //!< Cantidad de movimiento medio en el último min.
	Stats->CMov1Des = Stats->CMov1Des * k;
	Stats->CMov10Med = Stats->CMov10Med * k;  //!< Cantidad de movimiento medio en los últimos 10 min.
	Stats->CMov10Des = Stats->CMov10Des * k;
	Stats->CMov1HMed = Stats->CMov1HMed * k;  //!< Cantidad de movimiento medio en la última hora.
	Stats->CMov1HDes = Stats->CMov1HDes * k;
	Stats->CMov2HMed = Stats->CMov2HMed * k;	//!< Cantidad de movimiento medio en  últimas 2 horas.
	Stats->CMov2HDes = Stats->CMov2HDes * k;
	Stats->CMov4HMed = Stats->CMov4HMed * k ;
	Stats->CMov4HDes = Stats->CMov4HDes * k;
	Stats->CMov8HMed = Stats->CMov8HMed * k ; //!<//!< Cantidad de movimiento medio en  últimas 2 horas.
	Stats->CMov8HDes = Stats->CMov8HDes * k;
	Stats->CMov16HMed = Stats->CMov16HMed * k;
	Stats->CMov16HDes = Stats->CMov16HDes * k;
	Stats->CMov24HMed = Stats->CMov24HMed * k;
	Stats->CMov24HDes = Stats->CMov24HDes * k;
	Stats->CMov48HMed = Stats->CMov48HMed * k;
	Stats->CMov48HDes = Stats->CMov48HDes * k;
	Stats->CMovMed = Stats->CMovMed * k;
	Stats->CMovDes = Stats->CMovDes * k;

}



void normalizeStatsFlies( tlcde* Flies, ConvUnits* convUnits ){

	STFly* Fly;

	if( Flies->numeroDeElementos > 0) irAlPrincipio( Flies );
	for(int i = 0; i< Flies->numeroDeElementos ;i++ ){

		Fly = (STFly*)obtenerActual( Flies );
		if(Fly->etiqueta>0){
			Fly->Stats->EstadoBlobCount = Fly->Stats->EstadoBlobCount * convUnits->fTOsec;
			Fly->Stats->CountActiva = Fly->Stats->CountActiva * convUnits->fTOsec;
			Fly->Stats->CountPasiva = Fly->Stats->CountPasiva * convUnits->fTOsec;
			Fly->dstTotal = Fly->dstTotal * convUnits->pixelTOmm / 1000;  // a m
			Fly->Stats->CMovMed =  Fly->Stats->CMovMed * convUnits->pfTOmms;
			Fly->Stats->CMovDes =  Fly->Stats->CMovDes * convUnits->pfTOmms;
		}
		irAlSiguiente( Flies);
	}

}

void SetStatsParams( int FPS ){

    //init parameters
	config_t cfg;
	config_setting_t *setting;
	char settingName[30];
	char configFile[30];
	char settingFather[30];

	int EXITO;
	int DEFAULT = false;

	// Reservar memoria
	if( !statsParams){
		statsParams = ( STStatsParams *) malloc( sizeof( STStatsParams) );
		if(!statsParams) {error(4); return;}
	}

	fprintf(stderr, "\nCargando parámetros para cálculos estadísticos...");
	config_init(&cfg);

	sprintf( configFile, "config.cfg");
	sprintf( settingFather,"Estadisticas" );

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
	}

	if( DEFAULT ) SetDefaultStatsParams(  );
	/* Valores leídos del fichero de configuración. Algunos valores puedes ser establecidos por defecto si se indica
	 * expresamente en el fichero de configuración. Si el valor es erroneo o no se encuentra la variable, se establecerán
	 * a los valores por defecto.
	 */
	else{
		 /* Get the store name. */
		sprintf(settingName,"CalcStatsMov");
		if(! config_setting_lookup_bool ( setting, settingName, &statsParams->CalcStatsMov  )  ){
			statsParams->CalcStatsMov = true;
			fprintf(stderr, "No se encuentra la variable %s en el archivo de configuración o el tipo de dato es incorrecto.\n "
										"Establecer por defecto a %d \n",settingName,statsParams->CalcStatsMov);
		}

		sprintf(settingName,"MaxBufferTime");
		if(! config_setting_lookup_int ( setting, settingName, &statsParams->MaxBufferTime )  ){
			statsParams->MaxBufferTime = 8;
			fprintf(stderr, "No se encuentra la variable %s en el archivo de configuración o el tipo de dato es incorrecto.\n "
							"Establecer por defecto a %d \n",settingName,statsParams->MaxBufferTime);

		}

		sprintf(settingName,"MuestrearLinea");
		if(! config_setting_lookup_bool ( setting, settingName, &statsParams->MuestrearLinea  )  ){
			statsParams->MuestrearLinea = false;
			fprintf(stderr, "No se encuentra la variable %s en el archivo de configuración o el tipo de dato es incorrecto.\n "
										"Establecer por defecto a %d \n",settingName,statsParams->CalcStatsMov);
		}

	}

	SetPrivateStatsParams( FPS );

	ShowStatsParams( settingFather );
	config_destroy(&cfg);
}

void SetDefaultStatsParams(   ){

	statsParams->CalcStatsMov = true; 		// Switch true/false para activar el cálculo de estadísticas de movimiento
	statsParams->MaxBufferTime = 8;
	statsParams->MuestrearLinea = false;
}

void ShowStatsParams( char* Campo ){

	printf(" \nVariables para el campo %s : \n", Campo);
	printf(" -CalcStatsMov = %d \n", statsParams->CalcStatsMov);
	printf(" -MaxBufferTime = %d \n",statsParams->MaxBufferTime );

}

void SetPrivateStatsParams( int fps ){

	if( statsParams->CalcStatsMov){
		if (!Stats){
			Stats = ( STStatFrame *) malloc( sizeof(STStatFrame));
			if(!Stats) {error(4); exit(1) ;}
		}

		// iniciar vector de cantidad de movimiento por frame
		if(!vectorSumFr) {
			vectorSumFr = ( tlcde * )malloc( sizeof(tlcde ));
			if( !vectorSumFr ) {error(4);exit(1);}
			iniciarLcde( vectorSumFr );
		}

		if(!Coef){
			Coef =  ( STStatsCoef *) malloc( sizeof(STStatsCoef));
			calcCoef( fps );
		}
//		if(!Val){
//			Val = ( tlcde * )malloc( sizeof(tlcde ));
//			if( !Val ) {error(4);exit(1);}
//			iniciarLcde( Val );
//		}
	}

}

void releaseStats(  ){

	if( statsParams->CalcStatsMov ){
		// Borrar todos los elementos de la lista
		valorSum *valor = NULL;
		// Comprobar si hay elementos
		if(vectorSumFr == NULL || vectorSumFr->numeroDeElementos<1)  return;
		// borrar: borra siempre el elemento actual

		valor = (valorSum *)borrar(vectorSumFr);
		while (valor)
		{
			free(valor);
			valor = NULL;
			valor = (valorSum *)borrar(vectorSumFr);
		}
		free( Stats );
		free(Coef);
	}
	free( statsParams);
}
