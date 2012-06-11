/*
 * Stats.hpp
 *
 *  Created on: 31/01/2012
 *      Author: chao
 */

#ifndef STATS_HPP_
#define STATS_HPP_

#include "Libreria.h"

typedef struct{
	unsigned int sum;
}valorSum;
// coeficientes

typedef struct{
	float Sum;
	float Sum2;
	int frames;
	char periodo;
}StatsCoefs;



typedef struct{

	float SumFrame; //!< Suma del total de velocidad del frame
	unsigned int AbsCount;//!< Contador absoluto de frames para la media total
	unsigned int FMax; //!< Número máximo de valores a almacenar en el vector

	float F1SSum; //!< Sumatorio de las velocidades de los frames correspondientes a 30 s
	float F1SSum2; //!< Sumatorio del cuadrado de las velocidades de los frames correspondientes a 30 s
	unsigned int F1S;	//!< Número de frames necesarios para calcular la media de 30 s

	float F30SSum; //!< Sumatorio de las velocidades de los frames correspondientes a 30 s
	float F30SSum2; //!< Sumatorio del cuadrado de las velocidades de los frames correspondientes a 30 s
	unsigned int F30S;	//!< Número de frames necesarios para calcular la media de 30 s

	float F1Sum;
	float F1Sum2;
	unsigned int F1;

	float F5Sum;
	float F5Sum2;
	unsigned int F5;

	float F10Sum;
	float F10Sum2;
	unsigned int F10;

	float F15Sum;
	float F15Sum2;
	unsigned int F15;


	float F30Sum;
	float F30Sum2;
	unsigned int F30;

	float F1HSum;
	float F1HSum2;
	unsigned int F1H;


	float F2HSum;
	float F2HSum2;
	unsigned int F2H;

	float F4HSum;
	float F4HSum2;
	unsigned int F4H;

	float F8HSum;
	float F8HSum2;
	unsigned int F8H;


	float F16HSum;
	float F16HSum2;
	unsigned int F16H;

	float F24HSum;
	float F24HSum2;
	unsigned int F24H;


	float F48HSum;
	float F48HSum2;
	unsigned int F48H;


}STStatsCoef;

typedef struct{

	// public
	int CalcStatsMov ; 		// Switch true/false para activar el cálculo de estadísticas de movimiento


	int MaxBufferTime ;				// Establece el periodo máximo de la media móvil simple para el cálculo de
								// la cantidad de movimiento de los blobs en conjunto, o lo que es lo mismo,
								// el tamaño máximo del vector  que almacena las cantidades de movimiento
								// de cada frame.

								// VALORES: 0,1,2,4,8,16,24,48

								// Esto es, un valor de 1 indica que se calcularán estadísticas de la última hora como máximo.
								// y así sucesivamente hasta el máximo de 48 horas. Un valor de 0 indirá el máximo.

								// ACLARACIÓN: Suponiendo un Frame Rate de 30, un valor de 48 horas supondrá almacenar
								// en memoria un vector de 7020000 elementos con el consiguiente coste computacional
								// y de memoria.

	int MuestrearLinea;			// Realiza un muestreo del brillo de los píxels de una línea horizontal durante NumMuestras frames
								// Su uso está indicado para realizar una calibración precisa del modelo de fondo tras un análisis estadístico
								// de los datos. Se muestrea una línea horizontal de 100 píxels ( centro +-50 )

}STStatsParams;

void SetStatsParams( int FPS );

void SetDefaultStatsParams(   );

void SetPrivateStatsParams( int fps );

void ShowStatsParams( char* Campo );

void CalcStatsFrame( STFrame* frameDataOut , ConvUnits* convUnits);

STGlobStatF* SetGlobalStats( int NumFrame, timeval tif, timeval tinicio, int TotalFrames, float FPS);

STStatFrame* InitStatsFrame(  int fps);

void statsBloB( STFrame* frameDataOut );

void statsBlobS( STFrame* frameDataOut );

void calcCoef( int FPS );

unsigned int sumFrame( tlcde* Flies );

void mediaMovil(  STStatsCoef* Coef, tlcde* vector,  STStatFrame* Stats );

void iniciarMedias( STStatFrame* Stats, unsigned int valor );

void normalizeStatsBlobS( float k);

void normalizeStatsFlies( tlcde* Flies, ConvUnits* convUnits );

void releaseStats(  );

#endif /* STATS_HPP_ */
