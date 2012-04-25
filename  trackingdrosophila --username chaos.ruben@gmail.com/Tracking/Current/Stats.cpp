/*
 * Stats.cpp
 *
 *  Created on: 31/01/2012
 *      Author: chao
 */

#include "Stats.hpp"

// Las estadisticas se calculan actualizando el último frameDataOut ( frameDataStats, frame de salida anterior)
// El cálculo se hace en el elemento recién salido del buffer ( ya procesado ).Éste será el siguiente frameDataStats.
// Solo se trabajará sobre frameDataOut. El frameDataStats se usará como memoria para actualizar los diferentes cálculos
// estadísticos.
void CalcStatsFrame( STFrame* frameDataStats,STFrame* frameDataOut ){

	struct timeval ti;
	float TiempoParcial;

	if( !frameDataStats) return;
#ifdef MEDIR_TIEMPOS gettimeofday(&ti, NULL);
#endif
	printf( "Rastreo finalizado con éxito ..." );
	printf( "Comenzando análisis estadístico de los datos obtenidos ...\n" );
	printf("\n3)Cálculo de estadísticas en tiempo de ejecución:\n");

	// estadísticas de cada blob
	statsBloB(frameDataStats, frameDataOut );
	// estadísticas generales
	statsBlobS( frameDataStats, frameDataOut );


	printf( "Análisis finalizado ...\n" );
#ifdef MEDIR_TIEMPOS
	TiempoParcial = obtenerTiempo( ti , NULL);
	printf("Cálculos realizados. Tiempo total %5.4g ms\n", TiempoParcial);
#endif
}


STStatFrame* InitStatsFrame( int NumFrame, timeval tif, timeval tinicio, int TotalFrames, float FPS  ){

	STStatFrame* Stats = NULL;

	Stats = ( STStatFrame *) malloc( sizeof(STStatFrame));
	if(!Stats) {error(4); return 0;}
		//FrameData->Stats->totalFrames = 0;
	// FRAME
	Stats->TiempoFrame = obtenerTiempo( tif, 0 );
	Stats->TiempoGlobal = obtenerTiempo( tinicio, 1);
	Stats->totalFrames = TotalFrames;
	Stats->numFrame = NumFrame;
	Stats->fps = FPS;
	// BLOBS
	Stats->TProces = 0;
	Stats->TTacking= 0;
	Stats->staticBlobs= 0; //!< blobs estáticos en tanto por ciento.
	Stats->dinamicBlobs= 0; //!< blobs en movimiento en tanto por ciento
	Stats->TotalBlobs= 0; //!< Número total de blobs.

	Stats-> CMov30Med = 0;  //!< Cantidad de movimiento medio en los últimos 30 min.
	Stats-> CMov30Des = 0;
	Stats->CMov1HMed = 0;  //!< Cantidad de movimiento medio en la última hora.
	Stats-> CMov1HDes = 0;
	Stats-> CMov2H = 0;	//!< Cantidad de movimiento medio en  últimas 2 horas.
	Stats->CMov4H = 0;
	Stats->CMov8H = 0;
	Stats->CMov16H = 0;
	Stats->CMov24H = 0;
	Stats->CMov48H = 0;
	Stats->CMovMedio = 0;

	return Stats;

}

void statsBloB( STFrame* frameDataStats,STFrame* frameDataOut ){

	STFly* fly = NULL;
	frameDataOut->Stats->TotalBlobs = frameDataOut->Flies->numeroDeElementos;
	for(int i = 0; i< frameDataOut->Stats->TotalBlobs ; i++){
		fly = (STFly*)obtener(i,frameDataOut->Flies);
		if( fly->Estado == 1 ) frameDataOut->Stats->dinamicBlobs +=1;
		else frameDataOut->Stats->staticBlobs +=1;

		// velocidad instantánea
		EUDistance( fly->Vx, fly->Vy, NULL, &fly->Stats->VInst);
	}
	frameDataOut->Stats->dinamicBlobs = (frameDataOut->Stats->dinamicBlobs/frameDataOut->Stats->TotalBlobs)*100;
	frameDataOut->Stats->staticBlobs = (frameDataOut->Stats->staticBlobs/ frameDataOut->Stats->TotalBlobs)*100;

}

void statsBlobS( STFrame* frameDataStats,STFrame* frameDataOut ){

}
