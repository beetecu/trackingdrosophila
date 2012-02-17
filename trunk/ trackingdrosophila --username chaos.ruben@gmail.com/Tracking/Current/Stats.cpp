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

#ifdef MEDIR_TIEMPOS gettimeofday(&ti, NULL);
#endif
	printf("\n3)Cálculo de estadísticas en tiempo de ejecución:\n");

	printf( "Rastreo finalizado con éxito ..." );
	printf( "Comenzando análisis estadístico de los datos obtenidos ...\n" );
	printf( "Análisis finalizado ...\n" );
#ifdef MEDIR_TIEMPOS
	TiempoParcial = obtenerTiempo( ti , NULL);
	printf("Cálculos realizados. Tiempo total %5.4g ms\n", TiempoParcial);
#endif
}


STStatFrame* AllocateDataStats(  );
