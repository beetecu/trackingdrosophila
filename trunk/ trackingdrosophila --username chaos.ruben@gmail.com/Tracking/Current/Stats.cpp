/*
 * Stats.cpp
 *
 *  Created on: 31/01/2012
 *      Author: chao
 */

#include "Stats.hpp"

void CalcStatsFrame( STFrame* frameData,tlcde* Buffer ){

	struct timeval ti;
	float TiempoParcial;
	STStatFrame* stats;

	gettimeofday(&ti, NULL);
	printf("\n3)Cálculo de estadísticas en tiempo de ejecución:\n");

	printf( "Rastreo finalizado con éxito ..." );
	printf( "Comenzando análisis estadístico de los datos obtenidos ...\n" );
	printf( "Análisis finalizado ...\n" );

	TiempoParcial = obtenerTiempo( ti , NULL);
	printf("Cálculos realizados. Tiempo total %5.4g ms\n", TiempoParcial);
}


STStatFrame* AllocateDataStats(  );
