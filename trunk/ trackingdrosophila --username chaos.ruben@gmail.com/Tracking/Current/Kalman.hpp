/*
 * Kalman.hpp
 *
 *	Suposiciones:
*	Tras aplicar asignaridentidades:
*	- Tenemos en cada track un puntero al blob del frame t+1 con mayor probabilidad de asignación.*
*	  	En caso de que la máxima probabilidad de la asignación en base a la predicción de kalman haya sido menor que un umbral; esto es la mínima probabilidad de asignación
*		será la probabilidad correspondiente a la máxima distancia permitida o máximo salto ( medido en cuerpos y con algo de exceso ) que pueda dar una mosca, dicho
* 		track quedará sin posible asignación => FlySig = NULL. Pasará a estado SLEEPING; se ha perdido el objetivo a rastrear.
*	- Asimismo en cada fly disponemos una lista con el o los tracks candidatos a ser asignados a dicha fly.
*  En general primero se suele hacer la predicción para frame t ( posible estado del blob en el frame t) en base
* al modelo y luego se incorpora la medida de t con la cual se hace la corrección. En nuestro caso el proceso es:
* Se incorpora la medida del frame t. Se hace la corrección y se predice para el frame t+1, de modo que en cada track y para cada
* frame disponemos por un lado de la predicción para el frame t hecha en el t-1, con la cual se hace la corrección, y por otro lado de
* la predicción para el frame t+1. Así usamos el dato de la predicción para realizar la asignación de identidades.
 *
 *  Created on: 18/11/2011
 *      Authors: Rubén Chao Chao
 *      		German Macía Vázquez
 */

#ifndef KALMAN_HPP_
#define KALMAN_HPP_

//#include "VideoTracker.hpp"
#include "Tracking.hpp"

// indican qué parametros se desean obtener del filtro.
#define VELOCIDAD  1
#define V_ANGULAR 2
#define MAX_JUMP 3

#define SLEEPING 0
#define CAM_CONTROL 1
#define KALMAN_CONTROL 2

#define IN_BG 0
#define IN_FG 1
#define MISSED 2

#define UMBRAL_ALTO 1
#define UMBRAL_MEDIO 2
#define UMBRAL_BAJO 3
#define MAX_BLOBS 4

typedef struct{

				int ShowKalmanData; 	// Muesta por consola los resultados de cada filtro de kalman en cada frame

				int MaxBlobs  ;			// Número máximo de elementos a rastrear

				int MaxBuffer;			// Longitud del buffer. El módulo de tracking mantiene en memoria un número de frames
													// igual al valor introducido. Supondría almacenar MAX_BUFFER / FPS segundos de video.
													// Cuanta mayor sea la longitud del buffer, más eficaz será la fase de corrección y
													//  a la vez mayores serán las necesidades de memoria, con lo que un valor demasiado alto podría
													// afectar seriamente al rendimiento del equipo y por consiguiente, de la aplicación.
													// Para un con procesador Intel(R) Pentium(R) M a 1.73GHz y 1 Gb de RAM
													// corriendo Ubuntu 10.10 y GNOME 2.32.0 se aconseja un máximo de 80

				int MaxTimeSlept;		// Marca un tiempo máximo en farmes a partir del cual se eliminará un track. Dicho track ha
													// de cumplir ciertas condiciones para ser eliminado:
													// 1) Haber perdido el objeto a rastrear y no encontrarlo durante MAX_TIME_SLEPT frames
													// 2) Ser un track no prioritario, es decir, un track cuya de id sea superior al número máximo de blobs

				int NumberOfIdentities;	// Número máximo de identidades que se podrán asignar a los tracks. Cada vez que se crea un track, se le
													// asigna una identidad. Durante la ejecución pueden crearse tracks para falsos blobs ( por ejemplo reflejos)
													// que serán posteriormente eliminados.
				int PeriodoVelMed  ; 		// Establece el período de tiempo (en frames) para el cual se calcula la media móvil de
													// la velocidad de cada blob. Un valor demasiado alto afectaría al rendimiento.

													// ACLARACIÓN: Suponiendo un Frame Rate de 30 y un período de 30 , se calcularía la
													// velocidad media de un segundo. ( pixels/s ). En caso de realizarse el calibrado
													// ésta será en mm/s.

				double MediumActivityTh	;		// Establece el umbral de velocidad media (en mm/s) si es inferior se considerará
													// que un blob tiene actividad Media.

				double LowActivityTh	 ;		// Establece el umbral de velocidad media (en mm/s) a partir de la cual se considerará
													// que un blob tiene actividad BAJA

				double NullActivityTh ;		// Establece el umbral de velocidad media (en mm/s) a partir de la cual se considerará
													// que un blob tiene actividad INACTIVO
				// Private

//				ConvUnits* convUnits;
				float mmTOpixel;			/// Unidad de conversión de mm a pixels.

				float pixelTOmm;			/// Unidad de conversión de píxels a mm.

				float fTOsec;				/// Unidad de conversión de frames a segundos.

				float FPS;					/// Unidad de conversión de segundos a frames.

				float pfTOmms;				/// Unidad de conversión de pixels/frame a mm/s.

				float mmsTOpf;				/// Unidad de conversión de mm/s a pixels/frame.

}TrackingParams;

typedef struct{
	float sum;
}valorSumB;
typedef struct{
	float Vx;
	float Vy;
}valorV;


typedef struct {
	float dstTotal; // distancia total recorrida por el blob que está siendo rastreado

	int TShape; // contador del periodo de la media
	float a; // Tamaño medio del eje menor de la elipse del blob rastreado.
	float b; // Tamaño medio del eje mayor de la elipse del blob rastreado.

	tlcde* VectorSumB;//!< vector que contiene las velocidades instantáneas en T frames
	float SumatorioMed; //! sumatorio para la velocidad media en 1 seg
	float SumatorioDes; //! sumatorio para la desviación en la velocidad

	float CMovMed;  //!< Cantidad de movimiento medio pixels/frame.
	float CMovDes;
	float TOn;  //!< Tiempo en movimiento desde el inicio.
	float TOff; //!< Tiempo parado desde el inicio.

	unsigned int Estado;  //!< Indica el estado en que se encuentra el track: KALMAN_CONTROL(0) CAMERA_CONTROL (1) o SLEEP (2).
	unsigned int InitTime; //!< Numero de frame en el que se creó el track.
	CvPoint InitPos; //!< posición en la que  se inició el track
	unsigned int EstadoCount;
	unsigned int FrameCount; //! Contador de frames desde que se detectó el blob
	unsigned int EstadoBlob; //!< Indica el estado de actividad en que se encuentra el blob: pasivo(0) actividad baja (1),media (2),alta(3), oculto(4).
	unsigned int EstadoBlobCount;
	unsigned int TimeBlobOn;
	unsigned int TimeBlobOff;


}STStatTrack;

typedef struct{

	int id;
	CvScalar Color;
	bool validez; // Flag que indica que el track es válido con una alta probabilidad.
	float VInst; //!< Velocidad instantánea
	float VxInst;
	float VyInst;
	float PhiInst;
	float errorPhiInst;

	float PhiMed;
	float errorVPhi;
	float errorPhidif;
	float errorPhi;
	float wPhi;

	tlcde* VectorV;//!< lista que contiene vector con las velocidades instantáneas en T frames y
	float SumatorioVx; //! sumatorio para la velocidad media en T seg
	float SumatorioVy; //! sumatorio para la velocidad media en T seg
	float Vmed;
	float Vxmed;
	float errorVx;
	float Vymed;
	float errorVy;

	CvPoint KalInitPos;
	float KalInitPhi;
	int TimeToPhase1;	//! contador de tiempo restante para entrar en la fase 1


	CvKalman* kalman ; // Estructura de kalman para la linealizada

    CvMat* x_k_Pre_; // Predicción de k hecha en k-1
    CvMat* P_k_Pre_; // incertidumbre en la predicción en k-1

    const CvMat* x_k_Pre; // Predicción de k+1 hecha en k
    const CvMat* P_k_Pre; // incertidumbre en la predicción

	CvMat* x_k_Pos; // Posición tras corrección
	CvMat* P_k_Pos; // Incertidumbre en la posición tras corrección

	CvMat* Measurement_noise; // V->N(0,R) : Incertidumbre en la medida. Lo suponemos de media 0.
	CvMat* Measurement_noise_cov; // R
	CvMat* Medida;	// Valor real ( medido )
	CvMat* z_k; // Zk = H Medida + V. Valor observado

	STStatTrack* Stats; //! estadísticas del track
	STFly* Flysig; // Fly asignada en t+1. Obtenemos de aqui la nueva medida
	STFly* FlyActual; // Fly asignada en t.

}STTrack;

typedef struct{

	float Q;
	int g_slider_Q;
	// componentes del vector Rk de covarianzas. Incertidumbre en la medida inicial : V->N( 0, Rk )
	float alpha_Rk;
	int g_slider_alpha_Rk;
	float AlphaR_phi;
	int g_slider_AlphaR_phi;
}CamParams;

typedef struct{
	float Q;

	int g_slider_Q;
	int MaxTimeToPhase1;

	// componentes del vector Rk de covarianzas. Incertidumbre en la medida: V->N( 0, Rk )
	float alpha_Rk;	// factor que multiplica a la incertidumbre en la medida
	int g_slider_alpha_Rk;
	float R_phiZk;	// incertidumbre en la medida de la ángulo
	int g_slider_R_phiZk;

}KalParams;

typedef struct{

	float Velocidad;
	float V_Angular;
	float MaxJump;
	int w_max;

	int MaxBack;
	int MaxVPhiError;
	int T_Vmed;
	float Q;	 // factor Q: error asociado al modelo del sistema Wk->N(0,Q): Q = f(F,B y H)

	int createTrackbars;

	CamParams* Cam;
	KalParams* Kal;

	// componentes del vector Rk. Ruido en la medida
	float R_x;
	float R_y;
	float R_Vx;
	float R_Vy;
	float R_w;
	float R_phiZk;

}FilterParams;


/*!\brief En la primera iteración
 *-# Genera un track por cada blob ( un track por cada nueva etiqueta = -1 ) y los introduce en una lista doblemente enlazada
 *-# Inicia un filtro de kalman para cada track.
 *-# Realiza la primera predicción ( que posteriormente será usada por asignarIdentidades para establecer la nueva medida.
 * En cada iteración:
 *-#Crea un track para cada blob si es necesario.
 *-#Inicia un filtro de kalman por cada track.
 *-#Genera nueva medida en base a los datos obtenidos de la camara y de asignar identidades.
 *-#Filtra de la dirección y resuelve ambigüedad en la orientación.
 *-#Actualiza de los parámetros de rastreo: Track y fly (en base a los nuevos datos).
 *-#Realiza la predicción para t+1.
 *
 * Algoritmo:
 *-# Para cada track i:
 *	-# Generar medida:
 * 				- Establece el estado del track dependiendo de si hay o no nuevos datos
 * 					-# SLEEPING: Sin nueva medida
 * 					-# CAM_CONTROL: Asignación única entre Track y nueva medida.
 * 					-# KALMAN_CONTROL: Varios Tracks comparten una nueva medida.
 * 				- Genera o no la nueva medida. en caso afirmativo ésta dependerá del estado del Track.\n
 * 					 Z_{K}(i) = { x_Zk, y_Zk, vx_Zk, vy_Zk, phiZk } \n
 * 				- Genera el ruido asociado a la nueva medida.
 * 					 RZ_{k}(i) = { R_x, R_y, R_vx, R_vy, RphiZk }
 * 				- Actualiza los parámetros del track y de fly con la nueva medida
 *	-# Corrección:
 * 				- Realiza la corrección con la nueva medida.\n
 * 					 X_k_pos(i) = X_kpre(i) + K(i)( Z_k(i)- X_kpre(i) ) \n
 * 					 P_k_pos(i) = P_kpre(i) + K(i)( R_k(i)- P_kpre(i) ) \n
 * 					donde:\n
 * 					 K(i) = Pk_pre H^T ( H Pk_pre H^T + R_Zk)^(-1)
 * 				- Resuelve la ambiguedad en la orientación y establede la dirección ya filtrada.
 *-# Genera nuevos tracks si fly->etiqueta == -1 (Fly sin etiquetar).
 *-# Realiza la predicción para (t+1) de cada track\n
 * 		 X_k_pre(i) = F(k) X_k_pre(k-1)(i) + V(i)
 * 		 P_k_pre(i) = (F P_k_pre(k-1)(i) F(k)) + Q(i)
 * 		 donde:\n
 * 				\f[ Q(i) = f(F,B y H) \f]  Ruido asociado al sistema.\n
 * 				\f[ V(i) = N(0, R_Zk(i)) \f]  Ruido asociado a la medida.
 *
 * @param frameData Datos del nuevo frame. Imagenes y momentos de primer orden de cada blob.
 * @param lsIds  lista de identidades para asignar al nuevo track.
 * @param lsTracks  Lista que un track de cada blob.
 * @param Frames por segundo
 */

void Kalman(STFrame* frameData,tlcde* lsIds,tlcde* lsTracks, TrackingParams* trackParams);

/*! \brief Reserva memoria e inicializa un track: Le asigna una id, un color e inicia el filtro de kalman
 * Se le asigna al blob la id del track y se establece éste como fly Actual.
*
	  \param Fly : Objeto a rastrear sin track previo.
	  \param ids: lista de identidades para asignar al nuevo track
	  \param fps: tiempo ( dt ). tipicamente el frame rate.

	   \return Track.
		*/

STTrack* initTrack( STFly* Fly ,tlcde* ids, float fps );

//! \brief Inicia el filtro de kalman para el nuevo blob.\n
//! Inicializar las matrices parámetros para el filtro de Kalman
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

	   \return kalman. Filtro de kalman iniciado.
*/

CvKalman* initKalman( STFly* fly, float dt );


//! \brief Establece el estado en función de si hay o no nueva medida y en caso de que la haya, del tipo que sea.
//! - 0) Si no hay datos => Estado track = SLEEPING
//! - 1) Un track apunta a un blob. CAM_CONTROL
//! - 2) Varios tracks que apuntan al mismo blob KALMAN_CONTROL
/*!
	  \param Track : Track
	  \param FlySig: Nueva medida
	  \return : El estado del Track que dependerá de si hay a o no medida/s 0 para SLEEPING, 1 para CAM_CONTROL, 2 para KALMAN_CONTROL
	*/
int SetStateTrack( STTrack* Track, STFly* flySig );

/*!\brief  Establece el vector Z_K con la nueva medida.
 *  Genera la medida a partir de los nuevos datos obtenidos de la cámara ( flySig (t+1) );
 *	es decir, rellena las variables del vector \n Z_k. Z_K = { x_Zk, y_Zk, vx_Zk, vy_Zk, phiZk } \n en función del
 * estado del Track y del blob y establece dicho vector.
 *
 *Se supone que el estrado del trak ha sido establecido previamente de la siguiente forma
 *-# Si no hay datos => Estado track = SLEEPING (0)
 *-# Correspondencia uno a uno entre blob y track. CAM_CONTROL (1)
 *-# Varios tracks que apuntan al mismo blob KALMAN_CONTROL (2).
 *
 *
 * @param FG
 * @param Track
 * @param EstadoTrack
 */

void generarMedida( IplImage* FG,STTrack* Track, int EstadoTrack );

void CalcularVmed(  STTrack* Track, float* VMed, float* Vxmed, float* Vymed, float* errorVx, float* errorVy );

void obtenerDes( tlcde* Vector, float Vxmed, float Vymed,float* errorVx, float* errorVy);

void obtenerDir( STTrack* Track, float *PhiMed, float* errorVPhi );

void corregirDir( float phiXk,float* phiZk );

/*!\brief Incertidumbre en la medida Vk; Vk->N(0,R_Zk).
 * Genera el ruido a partir del estado del track
 * es decir, rellena las variables del vector R_Zk = { R_x, R_y, R_vx, R_vy, RphiZk }.
 *
 *Se supone que el estrado del trak ha sido establecido previamente de la siguiente forma
 *-# Si no hay datos => Estado track = SLEEPING (0)
 *-# Correspondencia uno a uno entre blob y track. CAM_CONTROL (1)
 *-# Varios tracks que apuntan al mismo blob KALMAN_CONTROL (2).
 *
 *	- Para SLEEPING 0: No se hace nada.
 *
 *  - Para CAM_CONTROL\n
 *		R_Zk(i) = { 1, R_x, 2R_x, 2R_y, RphiZk }\n
 *		donde:\n R_phiZk(i) = generarR_PhiZk( );\n
 * 		El error en la dirección dependerá de la velocidad y de la velocidad angular de la diferencia
 * 		entre la nueva medida y la filtrada.
 *
 *	- Para KALMAN_CONTROL ( type 0 ) el ruido será elevado (1000)\n
 *		R_Zk(i) = { Roi.width/2, Roi.height/2, 2R_x, 2R_y , 1000 }
 *
 * @param Track Track.
 * @param EstadoTrack Situación del Track: SLEEPING(0), CAM_CONTROL(1), KALMAN_CONTROL(2)
 */


void generarRuido( STTrack* Track, int EstadoTrack );



/*! \brief
*	-# Haya la distancia euclidea entre dos puntos.
*	-# Establece el modulo y la dirección del desplazamiento ( en grados ).
* 	-# Resuelve embiguedades en los signos de la atan.
*
	  \param a :  posición inicial
	  \param b :  poscición final
	  \param direccion :  argumento del vector desplazamiento: atan(b/a) corregida
	  \param distancia :  módulo del vector desplazamiento sqrt( a² + b² )
*/

void EUDistance( float a, float b, float* direccion, float* distancia);

/*! \brief Calcula el error en la medida del ángulo
 *
 * El error en la dirección dependerá de la velocidad y de la velocidad angular de la diferencia
 * entre la nueva medida y la filtrada.
* A medida que aumentamos la resolución ó aumenta la velocidad del blob, la precisión en la medida del ángulo aumenta.
*-# Calculamos el error en la precisión.
 \f[
 		errorV = max{  atan( |vy|/(|vx|- 1/2 ) -  atan( |vy|/|vx| ), atan( |vy| / |vx|) -  atan( (|vy|- 1/2) /|vx|)}*180/PI
 \f]
*-# Establece el error debido a la diferencia entre la nueva medida y la filtrada
	\f[
		phiDif = abs( phiXk - phiZk );
	\f]
*
*En base a los dos errores anteriores, devuelve el error
\f[	|phiXk-phiZk| > PI/2 and (Vx||Vy) <= 1  => R_phiZk = 90  \f]

\f[ |phiXk-phiZk| > PI/2 and (Vx||Vy) <= 2  => R_phiZk = 45  \f]

\f[	|phiXk-phiZk| > PI/2 and (Vx||Vy)  > 2  => R_phiZk = |phiXk-phiZk| + errorV  \f]

\f[ |phiXk-phiZk| <= PI/2 		         => R_phiZk = errorV \f]

*/

float generarR_PhiZk( );


/*!\brief
*-# Corrige la diferencia de vueltas.\n
* Si la dif en valor absoluto es mayor de 180 sumamos o restamos tantas vueltas
* como sea necesario para que la diferencia sea < ó = a 180
*-# Corrige la incertidumbre en la orientación sumando PI para que la diferencia
* entre phi y tita sea siempre menor de 90º
*\n
*
	\f[	n = phiX_k / 360 \f]
	\f[	if|phiX_k-tita| > 180  and phiX_k > tita => tita = tita + n*360 \f]
	\f[	if|phiX_k-tita| > 180  and phiX_k < tita => tita = tita - n*360 \f]
	\f[	if|phiX_k-tita| > 90 tita = tita + 180 \f]

	  \param phiXk : dirección filtrada
	  \param tita: Orientación
	  \return : Orientación corregida
	*/
float corregirTita( float phiXk, float tita );

/*!brief Calcula la media de la velocidad instantánea para los  frames correspondientes al
 * último segundo. *
 *
 * @param Stats
 * @param FPS
 */

/*!\brief
 - Actualiza los parámetros de Track con la nueva medida
 - Actualiza los parámetros de  fly a partir de los nuevos datos

 Dependiendo del estado del track se actualizará de una forma o de otra:
 -# Se actualiza el track con la nueva fly:
	- Si Estado Track = SELEEPING(0) se establece Fly Actual y Fly Sig a NULL.
	- Si no la fly siguiente pasa a ser fly actual y fly siguiente se pone a NULL .
 -# Se actualiza y etiqueta flyActual en función del estado del track:
	- Correspondencia uno a uno entre blob y track. CAM_CONTROL(1).
		-# Se identifica a la fly con la id y el color del track.
		-# Se actualizan otros parámetros del track como distancia total, numframe, etc
		-# Se actualizan otros parámetros de fly actual como el estado (background o foreground), etc
	- Varios tracks que apuntan al mismo blob  KALMAN_CONTROL(2)
		-# Se identifica a la fly con id = 0 y color blanco.
		-# Se establece la dirección del blob como su orientación.
		-# Se establece la distancia total en 0;

	  \param Track Track
	  \param EstadoTrack Situación del Track: SLEEPING(0), CAM_CONTROL(1), KALMAN_CONTROL(2)

*/

float errorR_PhiZkV( float vx_Zk, float vy_Zk );

int verificarSalida( IplImage* FG, int x, int y);

void updateTracks( tlcde* lsTracks,tlcde* Flies, TrackingParams* trackParams );

void updateStatsTrack( STTrack* Track, TrackingParams* trackParams );

void SetStateBlobTracked( STTrack* Track, TrackingParams* trackParams );

void updateFlyTracked( STTrack* Track, tlcde* Flies );

void generarFly( STTrack* Track, tlcde* Flies);

void mediaMovilVkalman( STTrack* Track, int Periodo );

void mediaMovilStats( STStatTrack* Stats, int Periodo );

void mostrarVMedia( tlcde* Vector);



int dejarId( STTrack* Track, tlcde* identities );

void asignarNuevaId( STTrack* Track, tlcde* identities);

int falsoTrack( STTrack* Track, int numFrame );

int deadTrack( tlcde* Tracks, int id );

void visualizarKalman( STFrame* frameData, tlcde* lsTracks, bool dataOn);

void showKalmanData( STTrack *Track);

void verMatriz( CvMat* Mat);

void verVector( CvMat* Mat);

void SetKalmanFilterParams( TrackingParams* trackParams );

void SetDefaultKFilterParams( TrackingParams* trackParams  );

void ShowKalmanFilterParams( char* Campo );

void SetPrivateKFilterParams(  );



int obtenerFilterParam( int param );

void crearTrackBarsTracking();

void onTrackbarTVmed(int val );

void onTrackbarMaxBack(int val );

void onTrackbarMaxVPhiError(int val );

void onTrackbarQCam(int val );

void onTrackbarAlphaRkCam(int val );

void onTrackbarAlphaRPhiCam(int val );

void onTrackbarQKal(int val );

void onTrackbarAlphaRkKal(int val );

void onTrackbarRPhiKal(int val );


void DeallocateKalman( tlcde* lista );

void liberarTracks( tlcde* lista);

void copyMat (CvMat* source, CvMat* dest);

#endif /* KALMAN_HPP_ */
