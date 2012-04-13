/*
 * Kalman.hpp
 *
 *  Created on: 18/11/2011
 *      Author: german
 */

#ifndef KALMAN_HPP_
#define KALMAN_HPP_

#include "VideoTracker.hpp"
#include "Tracking.hpp"

#define VELOCIDAD  5.3//10.599 // velocidad del blob en pixeles por frame
#define V_ANGULAR 2.6//5.2 // velocidad angular del blob en pixeles por frame
#define NUMBER_OF_MATRIX 6

#define SLEEPING 0
#define CAM_CONTROL 1
#define KALMAN_CONTROL 2



#define IN_BG 0
#define IN_FG 1
#define MISSED 2

typedef struct{
	int id;
	CvScalar Color;
	float dstTotal; // distancia total recorrida por el blob que está siendo rastreado

	CvKalman* kalman ; // Estructura de kalman para la linealizada

    CvMat* x_k_Pre_; // Predicción de k hecha en k-1
    CvMat* P_k_Pre_; // incertidumbre en la predicción en k-1

    const CvMat* x_k_Pre; // Predicción de k+1 hecha en k
    const CvMat* P_k_Pre; // incertidumbre en la predicción

	const CvMat* x_k_Pos; // Posición tras corrección
	const CvMat* P_k_Pos; // Incertidumbre en la posición tras corrección

	CvMat* Measurement_noise; // V->N(0,R) : Incertidumbre en la medida. Lo suponemos de media 0.
	CvMat* Measurement_noise_cov; // R
	CvMat* Medida;	// Valor real ( medido )
	CvMat* z_k; // Zk = H Medida + V. Valor observado

	unsigned int Estado;  //!< Indica el estado en que se encuentra el track: KALMAN_CONTROL(0) CAMERA_CONTROL (1) o SLEEP (2).
	unsigned int EstadoCount;
	unsigned int FrameCount; //! Contador de frames desde que se detectó el blob
	unsigned int EstadoBlob; //!< Indica el estado en que se encuentra el blob: FG(0) BG (1) o MISSED (2).
	unsigned int EstadoBlobCount;

	tlcde* listFlyNext;
	STFly* Flysig; // Fly asignada en t+1. Obtenemos de aqui la nueva medida
	STFly* FlyActual; // Fly asignada en t.

}STTrack;

//!/brief En la primera iteración:
//!<	1) Genera un track por cada blob ( un track por cada nueva etiqueta = -1 ) y los introduce en una lista doblemente enlazada
//!<	2) Inicia un filtro de kalman para cada track.
//!<	3) Realiza la primera predicción ( que posteriormente será usada por asignarIdentidades para establecer la nueva medida.
//!< bucle :
//!< 	1) Para cada track
//!<		1) Establece el estado del track dependiendo de si hay o no nuevos datos
//!<		2) Genera o no la nueva medida dependiendo del estado del Track.
//!<		3) Genera el ruido asociado a la nueva medida.
//!<		4) Actualiza los parámetros del track y de kalman con la nueva medida.
//!<		2) Realiza la corrección con la nueva medida.
//!<	2) Genera nuevos tracks si es necesario añadiendolos a la lista.
//!<	3) Realiza la predicción para (t+1) de cada track.


void Kalman(STFrame* frameData,tlcde* lsIds,tlcde* lsTracks);

//! \brief reserva memoria e inicializa un track: Le asigna una id, un color e inicia el filtro de kalman
//! Se le asigna al blob la id del track y se establece éste como fly Actual.
/*!
	  \param Fly : Objeto a rastrear sin track previo.
	  \param ids: lista de identidades para asignar al nuevo track
	  \param fps: tiempo ( dt ). tipicamente el frame rate.

	   \return : Track.
		*/

STTrack* initTrack( STFly* Fly ,tlcde* ids, float fps );

//! \brief Inicia el filtro de kalman para el nuevo blob.
//! 1)Inicializar las matrices parámetros para el filtro de Kalman
//! 	- Matriz de transición F
//!		- Matriz R inicial. Errores en medidas. R_Zk = { R_, R_y, R_vx, R_vy, RphiZk }
//!		- Vector de estado inicial X_K = { x_k, y_k, vx_k, vy_k, phiXk }
//!		- Matriz de medida H
//!		- Error asociado al modelo del sistema Q = f(F,B y H).
//!		- Incertidumbre en predicción. (P_k' = F P_k-1 Ft) + Q
//!		- Incertidumbre tras añadir medida. P_k = ( I - K_k H) P_k'
/*!
	  \param Fly : Objeto a rastrear .
	  \param fps: tiempo ( dt ). tipicamente el frame rate.

	   \return : kalman. Filtro de kalman iniciado.
		*/

CvKalman* initKalman( STFly* fly, float dt );

void ReInitTrack( STTrack* Track, STFly* Fly , float fps );
//! \brief // Establece el estado en función de si hay o no nueva medida y en caso de que la haya, del tipo que sea.
//! - 0) Si no hay datos => Estado track = SLEEPING
//! - 1) Un track apunta a un blob. CAM_CONTROL
//! - 2) Varios tracks que apuntan al mismo blob KALMAN_CONTROL
/*!
	  \param Track : Track
	  \param FlySig: Nueva medida
	  \return : El estado del Track que dependerá de si hay a o no medida/s 0 para SLEEPING, 1 para CAM_CONTROL, 2 para KALMAN_CONTROL
	*/
int establecerEstado( STTrack* Track, STFly* flySig );


//! \brief Establece el vector Z_K con la nueva medida.
//! Genera la medida a partir de los nuevos datos obtenidos de la cámara ( flySig (t+1) );
//!	es decir, rellena las variables del vector Z_k. Z_K = { x_Zk, y_Zk, vx_Zk, vy_Zk, phiZk } en función del
//! estado del Track y establece dicho vector.
//!	Se supone que el estrado del trak ha sido establecido previamente de la siguiente forma
//! - 0) Si no hay datos => Estado track = SLEEPING
//! - 1) Correspondencia uno a uno entre blob y track. CAM_CONTROL
//! - 2) Varios tracks que apuntan al mismo blob KALMAN_CONTROL
//! Caso 0:
//! - No se hace nada.
//! Casos 1 y 2:
//! - En ambos casos la velocidad, la dirección y el desplazamiento se establecen a partir del vector posición.
//! - Para CAM_CONTROL
//! Si la dirección y la predicción difieren en más de 180º, se suma a la dirección tantas
//! vueltas como sean necesarias para que ésta difiera en menos de 180º
//! si |phiXk-phiZk| > PI y phiXk > phiZk => phiZk = phiZk + n2PI
//! si |phiXk-phiZk| > PI y phiXk < phiZk => phiZk = phiZk - n2PI
//! - Para KALMAN_CONTROL ( type 0 )
//! la nueva dirección establece conmo la orientación del blob. Cuando se genere el ruido, éste será elevado
/*!
	  \param Track : Track
	  \param EstadoTrack: Situación del Track
		*/
void generarMedida( STTrack* Track, int EstadoTrack );

//! \brief Incertidumbre en la medida Vk; Vk->N(0,R_Zk)
//! Genera el ruido a partir del estado del track ( flySig (t+1))
//!	es decir, rellena las variables del vector R_Zk = { R_, R_y, R_vx, R_vy, RphiZk }
//!	Se supone que el estrado del trak ha sido establecido previamente de la siguiente forma
//! - 0) Si no hay datos => Estado track = SLEEPING
//! - 1) Correspondencia uno a uno entre blob y track. CAM_CONTROL
//! - 2) Varios tracks que apuntan al mismo blob KALMAN_CONTROL
//! Caso 0:
//! - No se hace nada.
//! Casos 1 y 2:
//! - En ambos casos la velocidad, la dirección y el desplazamiento se establecen a partir del vector posición.
//! - Para CAM_CONTROL
//!  El error en la dirección dependerá de la velocidad y de la velocidad angular de la diferencia
//!  entre la nueva medida y la filtrada.

//! - Para KALMAN_CONTROL ( type 0 )
/*!
	  \param Track : Track
	  \param EstadoTrack: Situación del Track: SLEEPING (0), CAM_CONTROL(1), KALMAN_CONTROL(2)
		*/

void generarRuido( STTrack* Track, int EstadoTrack );

//! \brief Haya la distancia euclidea entre dos puntos. Establece el modulo y la dirección del desplazamiento ( en grados ).
//!  Resuelve embiguedades en los signos de la atan.
/*!
	  \param a :  posición inicial
	  \param b :  poscición final
	  \param direccion :  argumento del vector desplazamiento: atan(b/a) corregida
	  \param b :  módulo del vector desplazamiento sqrt( a² + b² )
*/

void EUDistance( int a, int b, float* direccion, float* distancia);

//!< A medida que aumentamos la resolución ó aumenta la velocidad del blob, la precisión en la medida del ángulo aumenta
//!< - Calculamos el error en la precisión.
//!< - errorV = max{  atan( |vy|/(|vx|- 1/2 ) -  atan( |vy|/|vx| ), atan( |vy| / |vx|) -  atan( (|vy|- 1/2) /|vx|)}*180/PI
//!< - Establece el error debido a la diferencia entre la nueva medida y la filtrada
//!< 	si |phiXk-phiZk| > PI/2 y (Vx||Vy) <= 1  => R_phiZk = 90
//!< 	si |phiXk-phiZk| > PI/2 y (Vx||Vy) <= 2  => R_phiZk = 45
//!< 	si |phiXk-phiZk| > PI/2 y (Vx||Vy)  > 2  => R_phiZk = |phiXk-phiZk| + errorV
//!< 	si |phiXk-phiZk| <= PI/2 		         => R_phiZk = errorV

float generarR_PhiZk( );

//!\brief -Introduce la correspondiente nueva medida  ya generada en kalman. Zk = H Medida + V
//! -Actualiza los parámetros de Track con la nueva medida
//! -Actualiza los parámetros de  fly a partir de los nuevos datos
//! Dependiendo del estado del track se actualizará de una forma o de otra:
//! - 1)  Se actualiza el track con la nueva fly:
//!		  Si Estado Track = SELEEPING(0) se establece Fly Actual y Fly Sig a NULL.
//!	 	  Si no la fly siguiente pasa a ser fly actual y fly siguiente se pone a NULL .//!
//! - 2) Se actualiza y etiqueta flyActual en función del estado del track:
//!		a)Correspondencia uno a uno entre blob y track. CAM_CONTROL(1).
//!		- Se identifica a la fly con la id y el color del track.
//!		- Se actualizan otros parámetros del track como distancia total, numframe, etc
//!		- Se actualizan otros parámetros de fly actual como el estado (background o foreground), etc
//! 	b) Varios tracks que apuntan al mismo blob  KALMAN_CONTROL(2)
//!		Se actualizan track y flyActual para este caso:
//!		- Se identifica a la fly con id = 0 y color blanco.
//!		- Se establece la dirección del blob como su orientación.
//!		- Se establece la distancia total en 0;

void updateTrack( STTrack* Track, int EstadoTrack );

//!\brief Corrige la diferencia de vueltas
//! si la dif en valor absoluto es mayor de 180 sumamos o restamos tantas vueltas
//! como sea necesario para que la diferencia sea < ó = a 180
//! Corrige la incertidumbre en la orientación sumando PI para que la diferencia
//! entre phi y tita sea siempre menor de 90º
//!\n
/*!Mediana :
	\f[
		si |phiX_k-tita| > 180  y phiX_k > tita => tita = tita + n*360
		si |phiX_k-tita| > 180  y phiX_k < tita => tita = tita - n*360
		si |phiX_k-tita| > 90º tita = tita + 180
	\f]

/*!
	  \param phiXk : dirección filtrada
	  \param tita: Orientación
	  \return : Orientación corregida
	*/
float corregirTita( float phiXk, float tita );

int deadTrack( tlcde* Tracks, int id );

void visualizarKalman( STFrame* frameData, tlcde* lsTracks);

void showKalmanData( STTrack *Track);

void DeallocateKalman( tlcde* lista );

void liberarTracks( tlcde* lista);

void copyMat (CvMat* source, CvMat* dest);

#endif /* KALMAN_HPP_ */
