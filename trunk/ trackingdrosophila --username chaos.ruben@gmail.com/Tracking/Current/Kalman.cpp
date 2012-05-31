/*
 * Kalman.cpp
 *
 *  Created on: 18/11/2011
 *      Authores: Rubén Chao Chao, Germán Garcia Vázquez
 *
 */


#include "Kalman.hpp"



CvMat* H ;

// componentes del vector Zk con las nuevas medidas
static float x_Zk;
static float y_Zk;
static float vx_Zk;
static float vy_Zk;
static float phiZk;

// componente del vector Xk. Dirección filtrada
static float phiXk ;

// variables para establecer la nueva medida
static int Ax; //incremento de x
static int Ay; //incremento de y
static float distancia; // sqrt( Ax² + Ay² )

// Parámetros
FilterParams* filterParams;
//Imagenes
IplImage* ImKalman = NULL;
int g_slider_ = 10;

void Kalman(STFrame* frameData, tlcde* lsIds,tlcde* lsTracks, TrackingParams* trackParams) {

	tlcde* Flies;
	STTrack* Track;
	STFly* Fly = NULL;

	// cargar datos del frame
	Flies=frameData->Flies;
	if( frameData->num_frame == 530 ){
						printf("hola");
					}

	// Inicializar imagenes y parámetros
	if( !ImKalman ){
		ImKalman = cvCreateImage(cvGetSize(frameData->ImKalman),8,3);
		cvZero(ImKalman);

	}
	cvZero(frameData->ImKalman);
	if( lsTracks->numeroDeElementos > 0){
		irAlPrincipio(lsTracks);
		for(int i = 0;i < lsTracks->numeroDeElementos ; i++){
			// obtener Track
			Track = (STTrack*)obtenerActual(lsTracks);
			// Esablece estado del track según haya no nueva/s medida/s
			Track->Stats->Estado = SetStateTrack( Track, Track->Flysig );

			////////////////////// GENERAR MEDIDA ////////////////////
			// Generar la nueva medida en función del estado
			generarMedida( Track, Track->Stats->Estado );
			// Generar ruido asociado a la nueva medida
			generarRuido( Track, Track->Stats->Estado );

			////////////////////// FASE DE CORRECCIÓN ////////////////////////
			// corregir kalman
			if( Track->Stats->Estado != SLEEPING ){
				cvKalmanCorrect( Track->kalman, Track->z_k);
			}
			irAlSiguiente( lsTracks );
		}
		/////////////// ACTUALIZACIÓN DE TRACKS CON LOS NUEVOS DATOS //////////////
		updateTracks( lsTracks,frameData->Flies, trackParams );
	}


	// CREAR NUEVOS TRACKS, si es necesario.
	// si hay mosca/s sin asignar a un track/s (id = -1), se crea un nuevo track para esa mosca
	if( Flies->numeroDeElementos > 0) irAlPrincipio( Flies );
	for(int i = 0; i< Flies->numeroDeElementos ;i++ ){
		Fly = (STFly*)obtenerActual( Flies );
		if( Fly->etiqueta == -1 ){
			Track = initTrack( Fly, lsIds , 1 );
			anyadirAlFinal( Track , lsTracks );
		}
		irAlSiguiente( Flies);
	}

	////////////////////// FASE DE PREDICCION ////////////////////////
	//Recorremos cada track y hacemos la predicción para el siguiente frame
	if( lsTracks->numeroDeElementos > 0) irAlPrincipio( lsTracks);
	for(int i = 0;i < lsTracks->numeroDeElementos ; i++){
		Track = (STTrack*)obtenerActual(lsTracks);
		if(Track->Stats->Estado != SLEEPING ){
			if( Track->x_k_Pre != NULL ){
				// guardamos los datos de la predicción anterior
				cvCopy(Track->x_k_Pre,Track->x_k_Pre_);
				cvCopy(Track->P_k_Pre,Track->P_k_Pre_);
			}
			// nueva predicción
			Track->x_k_Pre = cvKalmanPredict( Track->kalman, 0);
		}
		irAlSiguiente(lsTracks);
	}

	///////////////////// VISUALIZAR RESULTADOS ////////////////
	// si modo completo activado y mostrar kalman activado o bien SHOW_KALMAN_DATA activado
	if( (obtenerVisParam( MODE ) && obtenerVisParam( SHOW_KALMAN ))||trackParams->ShowKalmanData ) {
		 visualizarKalman( frameData, lsTracks, trackParams->ShowKalmanData );
	}



	return;
}// Fin de Kalman



// etiquetar fly e iniciar nuevo track

STTrack* initTrack( STFly* Fly ,tlcde* ids, float fps ){

	STTrack* Track = NULL;


	Track = (STTrack*)malloc(sizeof(STTrack));
	if(!Track) {error(4); exit(1);}

	asignarNuevaId( Track , ids );
	//TRACK
	// iniciar estadísticas
	Track->Stats = ( STStatTrack * )malloc( sizeof(STStatTrack ));
	if( !Track->Stats ) {error(4);exit(1);}

	Track->Stats->CMovMed = 0;  //!< Cantidad de movimiento medio mm/s.
	Track->Stats->CMovDes = 0;  //!< Cantidad de movimiento medio.mm/min
	Track->Stats->TOn = 0;  //!< Tiempo en movimiento desde el inicio.
	Track->Stats->TOff = 0; //!< Tiempo parado desde el inicio.
	Track->Stats->SumatorioMed = 0;
	Track->Stats->SumatorioDes = 0;

	Track->Stats->VectorSumB = ( tlcde * )malloc( sizeof(tlcde ));
	if( !Track->Stats->VectorSumB ) {error(4);exit(1);}
	iniciarLcde( Track->Stats->VectorSumB );
	Track->Stats->InitTime =Fly->num_frame;
	Track->Stats->InitPos =  Fly->posicion;

	// añadir al final en Fly->Tracks el track

	Track->Stats->Estado = CAM_CONTROL;
	Track->Stats->EstadoCount = 1;
	Track->Stats->FrameCount = 1;
	Track->Stats->EstadoBlob = 1;
	Track->Stats->EstadoBlobCount = 0;
	Track->Stats->dstTotal = 0;
	Track->Stats->TimeBlobOff = 0;
	Track->Stats->TimeBlobOn = 1;
	// iniciamos parámetros del track
	// iniciar kalman
	Track->VectorSumVx = ( tlcde * )malloc( sizeof(tlcde ));
	if( !Track->VectorSumVx ) {error(4);exit(1);}
	iniciarLcde( Track->VectorSumVx );

	Track->VectorSumVy = ( tlcde * )malloc( sizeof(tlcde ));
	if( !Track->VectorSumVy ) {error(4);exit(1);}
	iniciarLcde( Track->VectorSumVy );

	Track->SumatorioVx = 0; //! sumatorio para la velocidad media en T seg
	Track->SumatorioVy = 0; //! sumatorio para la velocidad media en T seg

	Track->kalman = initKalman( Fly , fps);

	Track->x_k_Pre_ = cvCreateMat( 5,1, CV_32FC1 );
	Track->P_k_Pre_ = cvCreateMat( 5,5, CV_32FC1 );
	cvZero(Track->x_k_Pre_);
	cvZero(Track->P_k_Pre_);

	Track->x_k_Pos = Track->kalman->state_post ;
	Track->P_k_Pos = Track->kalman->error_cov_post;

	Track->x_k_Pre = NULL; // es creada por kalman
	Track->P_k_Pre = Track->kalman->error_cov_pre;

	Track->Medida = cvCreateMat( 5, 1, CV_32FC1 );
	Track->Measurement_noise = cvCreateMat( 5, 1, CV_32FC1 );
	Track->Measurement_noise_cov = Track->kalman->measurement_noise_cov;
	Track->z_k = cvCreateMat( 5, 1, CV_32FC1 );
	Track->VInst = 0;
	Track->Vxmed = 0;
	Track->Vymed = 0;

	cvZero(Track->Medida);
	cvZero(Track->z_k );

	Fly->etiqueta = Track->id;
	Fly->Color = Track->Color;
	Fly->Estado = 1;
	Fly->Stats->EstadoBlobCount = Track->Stats->EstadoBlobCount;
	Fly->Stats->CMovMed = Track->Stats->CMovMed;
	Fly->Stats->CMovDes = Track->Stats->CMovDes;
	Fly->Stats->EstadoTrack = Track->Stats->Estado;
	Fly->Stats->EstadoTrackCount = Track->Stats->EstadoCount;
	Fly->Stats->CountActiva = Track->Stats->TimeBlobOn;
	Fly->Stats->CountPasiva =Track->Stats->TimeBlobOff;
	//Asignamos la fly al track
	Track->FlyActual = Fly;
	Track->Flysig = NULL;

	return Track;
	//		direccionR = (flyData->direccion*CV_PI)/180;
	// Calcular e iniciar estado

}

CvKalman* initKalman( STFly* Fly, float dt ){

	static float Vx=0;// componente X de la velocidad.
	static float Vy=0;// componente Y de la velociad.

	// Crear el flitro de Kalman

	CvKalman* Kalman = cvCreateKalman(5,5,0);

	// Inicializar las matrices parámetros para el filtro de Kalman
	// en la primera iteración iniciamos la dirección con la orientación

	Vx= VELOCIDAD*cos(Fly->orientacion*CV_PI/180);
	Vy=-VELOCIDAD*sin(Fly->orientacion*CV_PI/180);

	// x = x0 + Vx dt
	const float F[] = {	1,0,dt,0,0,
						0,1,0,dt,0,
						0,0,1,0,0,
						0,0,0,1,0,
						0,0,0,0,1}; // Matriz de transición F

	filterParams->R_x = 0;	// implementar método de cuantificación del ruido.
	filterParams->R_y = 0;

	//2) Velocidad
	filterParams->R_Vx = 0;
	filterParams->R_Vy = 0;
	filterParams->R_phiZk = 0;

	// Matriz R inicial. Errores en medidas. ( Rx, Ry,RVx, RVy, Rphi ). Al inicio el error en el angulo es de +-180º
	const float R_inicial[] = {	filterParams->R_x,0,0,0,0,
								0,filterParams->R_y,0,0,0,
								0,0,filterParams->R_Vx,0,0,
								0,0,0,filterParams->R_Vy,0,
								0,0,0,0,filterParams->R_phiZk};//{50,50,50,50,180};

	const float X_k[] = {	Fly->posicion.x,
							Fly->posicion.y,
							Vx,
							Vy,
							Fly->orientacion };// Matiz de estado inicial
	// establecer parámetros
	memcpy( Kalman->transition_matrix->data.fl, F, sizeof(F)); // F

	cvSetIdentity( Kalman->measurement_matrix,cvRealScalar(1) ); // H: Matriz de medida
	cvSetIdentity( Kalman->process_noise_cov,cvScalar( filterParams->Q ) ); // Q: error asociado al modelo del sistema Wk->N(0,Q): Q = f(F,B y H).

	cvSetIdentity( Kalman->error_cov_pre, cvScalar(1) );		 // P_k': Incertidumbre en predicción. (P_k' = F P_k-1 Ft) + Q
	cvSetIdentity( Kalman->error_cov_post,cvRealScalar(1));		 // P_k: Incertidumbre tras añadir medida. P_k = ( I - K_k H) P_k'

	memcpy( Kalman->measurement_noise_cov->data.fl, R_inicial, sizeof(R_inicial)); // Incertidumbre en la medida Vk; Vk->N(0,R)
//	cvSetIdentity( Kalman->error_cov_post,cvRealScalar(1)); // Incertidumbre tras añadir medida. P_k = ( I - K_k H) P_k'
	//copyMat(&x_k, Kalman->state_post);
	memcpy( Kalman->state_post->data.fl, X_k, sizeof(X_k)); // Estado inicial

	return Kalman;
}


//Establecer el estado del Track según haya o no medida/s. en caso de cambio de estado inicia los contadores de estado.
int SetStateTrack( STTrack* Track, STFly* flySig ){


	if ( !flySig || !Track->FlyActual ){ // si no hay flysig o fly actual SLEEPING
		if(!flySig && Track->FlyActual){ // Si cambio de estado: iniciar contador de estado
			Track->Stats->EstadoCount = 0;
		}
		return SLEEPING;
	}
	else if( flySig->Tracks->numeroDeElementos == 1 ){
		if( Track->Stats->Estado != CAM_CONTROL){
			Track->Stats->EstadoCount = 0; // Cambio de estado
		}
		return CAM_CONTROL;
	}

	else if(flySig->Tracks->numeroDeElementos > 1){
		if( Track->Stats->Estado != KALMAN_CONTROL){
			Track->Stats->EstadoCount = 0;
		}
		return KALMAN_CONTROL;
	}
	else return -1;

}


void generarMedida( STTrack* Track, int EstadoTrack ){

	STFly* flyActual = Track->FlyActual;
	STFly* flySig = Track->Flysig;
	STFly* flyAnterior = NULL;

	//Caso 0: No se ha encontrado ninguna posible asignación. No hay medida.
	if ( EstadoTrack == SLEEPING )	return ;

	//Obtener dirección usando las posiciones de los frames anteriores
	if( flyActual->anterior ){
		flyAnterior = (STFly*)Track->FlyActual->anterior;
		float errorV;
		for( int i = filterParams->PeriodoVmed-1; i > 0; i--){
			Ax = flySig->posicion.x - flyAnterior->posicion.x;
			Ay = flySig->posicion.y - flyAnterior->posicion.y;
			errorV = errorR_PhiZkV( Ax, Ay );
			if( errorV < 10) break;
			else if(flyAnterior->anterior) flyAnterior = (STFly*)flyAnterior->anterior;
		}
	}
	if(flyAnterior)	EUDistance( Ax, Ay, &Track->PhiMed, NULL );

	else Ax = Ay = Track->PhiMed = 0;

	x_Zk = flySig->posicion.x;
	y_Zk = flySig->posicion.y;

	Ax = x_Zk - flyActual->posicion.x;
	Ay = y_Zk - flyActual->posicion.y;

	vx_Zk = Ax / 1;
	vy_Zk = Ay / 1;

	//Establecemos la dirección phi y el modulo del vector de desplazamiento
	// esta dirección no se usa para la predicción, ya que está sin filtrar.
	// usaremos la dirección filtrada obtenida en t-1.
	// asumimos que la dirección varia poco de un frame al siguiente
	EUDistance( vx_Zk, vy_Zk, &phiZk, &distancia );
	Track->VInst = distancia;

	// calculamos la velocidad en T seg
	  // al ser siempre  t = 1 frame, la distancia en pixels coincide con Vt
	// Obtener el nuevo valor
	valorSumVx* Vx =  ( valorSumVx *) malloc( sizeof(valorSumVx));
	valorSumVy* Vy =  ( valorSumVy *) malloc( sizeof(valorSumVy));
	Vx->sum = Ax;
	Vy->sum = Ay;

	anyadirAlFinal(Vx, Track->VectorSumVx );
	anyadirAlFinal( Vy, Track->VectorSumVy );
//	calculo de la velocidad media de los  frames equivalentes al periodo establecido
	mediaMovilVkalman( Track, Track->VectorSumVx->numeroDeElementos);

		printf("\nVector Media movil Vx:\t");
		mostrarVMedia(Track->VectorSumVx);
		printf("\nVector Media movil Vy:\t");
		mostrarVMedia(Track->VectorSumVy);
		printf("\n");
	// si el vector llega a fps elementos eliminamos el primero.
	if( Track->VectorSumVx->numeroDeElementos == filterParams->PeriodoVmed+1){
		Vx = (valorSumVx*)liberarPrimero(  Track->VectorSumVx );
		Vy = (valorSumVy*)liberarPrimero(  Track->VectorSumVy );
		free(Vx);
		free(Vy);
	}


	EUDistance( Track->Vxmed, Track->Vymed, NULL, &Track->Vmed );

	//Caso 1: Se ha encontrado la siguiente posición. la medida se genera a partir de los datos obtenidos de los sensores
	if( EstadoTrack == CAM_CONTROL ) {

		// si no hay movimiento la dirección es la orientación.( o la dirección anterior? o la filtrada??? )
		if( Track->Vmed < 0.5 ){ // establecer un umbral para el ruido.Puede ser el de actividad.
			// si no se ha el minimo error medio
			//phiZk = flyActual->dir_filtered;
			//else
			phiZk = flyActual->orientacion;

		}
		else{
			// CORREGIR DIRECCIÓN.
			//Sumar ó restar n vueltas a la nueva medida para que la dif entre ella
			// y el valor filtrado sea siempre <=180

			phiXk = Track->x_k_Pre->data.fl[4]; // valor filtrado de la direccion
			//phiZk = corregirTita( phiXk,  phiZk );
			if( abs(phiXk - phiZk ) > 180 ){
				int n = (int)phiXk/360;
				if( n == 0) n = 1;
				if( phiXk > phiZk ) phiZk = phiZk + n*360;
				else phiZk = phiZk - n*360;
			}
			
		}

	}
	//Caso 2:Un blob contiene varias flies.La medida se genera a partir de las predicciones de kalman
		// La incertidumbre en la medida de posición será más elevada
	else if( EstadoTrack == KALMAN_CONTROL ){
		phiZk = flySig->orientacion;
	}

	// ESTABLECER Z_K
	H = Track->kalman->measurement_matrix;

	const float Zk[] = { x_Zk, y_Zk, vx_Zk, vy_Zk, phiZk };
	memcpy( Track->Medida->data.fl, Zk, sizeof(Zk)); // Medida;

	const float V[] = {0,0,0,0,0}; // media del ruido
	memcpy( Track->Measurement_noise->data.fl, V, sizeof(V));

	cvGEMM(H, Track->Medida,1, Track->Measurement_noise, 1, Track->z_k,0 ); // Zk = H Medida + V


	return ;
}

void generarRuido( STTrack* Track, int EstadoTrack ){


	//0) No hay nueva medida
	if (EstadoTrack == SLEEPING ) return ;

	crearTrackBars();
	// GENERAR RUIDO.
	// 1) Caso normal. Un blob corresponde a un fly
	if (EstadoTrack == CAM_CONTROL ) {
		if( Track->Stats->EstadoCount < (unsigned)filterParams->InitTime){ // < por que aún no se ha actualizado el contador para el estado

			filterParams->R_x =  	filterParams->Cam->alpha_Rk0 	* 1 ;	// implementar método de cuantificación del ruido.
			filterParams->R_y =  	filterParams->Cam->alpha_Rk0 	* 1 ;
			filterParams->R_Vx = 	2*filterParams->R_x ;
			filterParams->R_Vy = 	2*filterParams->R_y ;
			filterParams->R_phiZk = filterParams->Cam->AlphaR_phi0	* generarR_PhiZk( );

		}
		else{
			filterParams->R_x =  	filterParams->Cam->alpha_Rk 	* 1 ;	// implementar método de cuantificación del ruido.
			filterParams->R_y =  	filterParams->Cam->alpha_Rk 	* 1 ;
			filterParams->R_Vx = 	2*filterParams->R_x ;
			filterParams->R_Vy = 	2*filterParams->R_y ;
			filterParams->R_phiZk = filterParams->Cam->AlphaR_phi	* generarR_PhiZk( );
		}
	}
	//1) Varias flies en un mismo blob
	else if( EstadoTrack == KALMAN_CONTROL ){

		filterParams->R_x	  = filterParams->Kal->alpha_Rk * Track->Flysig->Roi.width/2;
		filterParams->R_y	  = filterParams->Kal->alpha_Rk * Track->Flysig->Roi.height/2;
		filterParams->R_Vx 	  = 2*filterParams->R_x;
		filterParams->R_Vy    = 2*filterParams->R_y;
		filterParams->R_phiZk = filterParams->Kal->R_phiZk;
	}

	// INSERTAR RUIDO en kalman
	// cargamos el error en la medida en el filtro para calcular la ganancia de kalman
	const float R[] = { filterParams->R_x,0,0,0,0,
				  0,filterParams->R_y,0,0,0,
				  0,0,filterParams->R_Vx,0,0,
				  0,0,0,filterParams->R_Vy,0,
				  0,0,0,0,filterParams->R_phiZk};//covarianza del ruido
	memcpy( Track->kalman->measurement_noise_cov->data.fl, R, sizeof(R)); // V->N(0,R);

	return ;
}

void EUDistance( int a, int b, float* direccion, float* distancia){

	float phi;
	if( a == 0){
		if( b == 0 ) phi = -1; // No hay movimiento.
		if( b > 0 ) phi = 270; // Hacia abajo.
		if( b < 0 ) phi = 90; // Hacia arriba.
	}
	else if ( b == 0) {
		if (a < 0) phi = 180; // hacia la izquierda.
		else phi = 0;			// hacia la derecha.
	}
	else	phi = atan2( (float)-b , (float)a )* 180 / CV_PI;
	// calcular modulo del desplazamiento.
	if (distancia) *distancia = sqrt( pow( a ,2 )  + pow( b ,2 ) ); // sqrt( a² + b² )

	if( direccion == NULL ) return;
	else *direccion = phi;
}

// Al inicio queremos una convergencia rápida de la dirección. En las primeras iteraciones
// penalizaremos en función del error medio entre phiZK y phiXK ( si fuese el error instantáneo se vería
// muy afectado por el ruido). Inicialmente cuanto mayor sea el error absoluto medio menor será la penalización. A medida que se reduzca
// dicho error las penalizaciónes se irán incrementando hasta tomar los valores normales.
// menor penalización. Una vez que la diferencia media entre phiZK y phiXK se ha reducido
// lo suficiente,

float generarR_PhiZk( ){
	// Error debido a la velocidad. Esta directamente relaccionada con la resolución

	float phiDif; // error debido a la diferencia entre el nuevo valor y el filtrado
	float errorV; // incertidumbre en la dirección debido a la velocidad

	errorV = errorR_PhiZkV( vx_Zk, vy_Zk );

	// Error debido a la diferencia entre el valor corregido y el medido. Si es mayor de PI/2 penalizamos
	phiDif = abs( phiXk - phiZk );

	if( phiDif > 90 ) {

		if ( vx_Zk >= 2 || vy_Zk >= 2) phiDif = phiDif/4; // si la velocidad es elevada, penalizamos poco
												//a pesar de ser mayor de 90
		return phiDif + errorV; // el error es la suma del error devido a la velocidad y a la diferencia
	}
	else {

		if ( vx_Zk <=1 && vy_Zk <= 1) errorV = 90; // si la velocidad es de 1 pixel penalizamos al máximo
		else if ( vx_Zk <=2 && vy_Zk <= 2) errorV = 45; // si es de dos pixels penalizamos con 45º

		return errorV; // si la dif es menor a 90 el error es el debido a la velocidad
	}
}

float corregirTita( float phiXk, float tita ){

	if( abs(phiXk - tita ) > 180 ){
		int n = (int)phiXk/360;
		if( n == 0) n = 1;
		if( phiXk > tita ) tita = tita + n*360;
		else tita = tita - n*360;
	}

	if( abs(phiXk - tita ) > 90 ){
		if( phiXk > tita ) tita = tita + 180;
		else tita = tita - 180;
	}
	return tita;
}

float errorR_PhiZkV( float vx_Zk, float vy_Zk ){

	float errorV; // incertidumbre en la dirección debido a la velocidad
	float errIz; // error debido a la velocidad por la izqd del ángulo
	float errDr; // error debido a la velocidad por la drcha del ángulo

	errIz = atan2( abs(vy_Zk), abs( vx_Zk) - 0.5 )  - atan2( abs(vy_Zk) , abs( vx_Zk) );
	errDr = atan2( abs(vy_Zk) , abs( vx_Zk) ) - atan2((abs(vy_Zk) - 0.5) , abs( vx_Zk) );

	if (errIz >= errDr ) errorV = errIz*180/CV_PI;
	else errorV = errDr*180/CV_PI;

	return errorV ;

}




void updateTracks( tlcde* lsTracks,tlcde* Flies, TrackingParams* trackParams ){

	STTrack* Track;

	if( lsTracks->numeroDeElementos < 1) return;
	irAlPrincipio( lsTracks);
	for(int i = 0;i < lsTracks->numeroDeElementos ; i++){
		// obtener Track

		Track = (STTrack*)obtenerActual( lsTracks);
		// ACTUALIZAR TRACK
		// actualizar estadísticas del track en base a su estado
		updateStatsTrack( Track, trackParams );
		// ETIQUETAR Y ACTUALIZAR FLY DEL TRACK
		// en caso de Kalman_control se crea una mosca para simular la posición
		updateFlyTracked( Track , Flies);

		if ( Track->Stats->Estado == SLEEPING){
			Track->FlyActual = NULL;
			Track->Flysig = NULL;
		}
		else{
			// Actualizar fly actual.
			Track->FlyActual->siguiente = (STFly*)Track->Flysig;
			// fly actual pasa a ser fly siguiente.
			Track->FlyActual = Track->Flysig;
			Track->Flysig = NULL;
		}
		irAlSiguiente( lsTracks);
	}
}

void updateStatsTrack( STTrack* Track, TrackingParams* trackParams ){

	valorSumB* valor;

	// actualizar contadores
	if ( Track->Stats->Estado == SLEEPING){
		Track->Stats->FrameCount ++;
		Track->Stats->EstadoCount ++;
		return;
	}
	Track->Stats->EstadoCount ++;
	Track->Stats->FrameCount ++;
	// calcular distancia y media movil para la velocidad

	// para el caso kalman control, establecemos la distancia en base a la pos anterior y a la corregida (o a la predicha??)
	if( Track->Stats->Estado == KALMAN_CONTROL){

		float Ax = Track->x_k_Pos->data.fl[0] - Track->FlyActual->posicion.x;
		float Ay = Track->x_k_Pos->data.fl[1] - Track->FlyActual->posicion.y;

		//Establecemos la dirección phi y el modulo del vector de desplazamiento
//		EUDistance( Track->x_k_Pos->data.fl[2], Track->x_k_Pos->data.fl[3], NULL, &Track->VInst );

		EUDistance( Ax, Ay, NULL, &Track->VInst );
	}
	Track->Stats->dstTotal = Track->Stats->dstTotal + Track->VInst;
	// calculamos la velocidad en T seg
	  // al ser siempre  t = 1 frame, la distancia en pixels coincide con Vt
	// Obtener el nuevo valor
	valor =  ( valorSumB *) malloc( sizeof(valorSumB));
	valor->sum = Track->VInst;
	anyadirAlFinal( valor, Track->Stats->VectorSumB );
//	calculo de la velocidad media de los  frames equivalentes al periodo establecido
	mediaMovilStats( Track->Stats, trackParams->PeriodoVelMed);
	// si el vector llega a fps elementos eliminamos el primero.
	if( Track->Stats->VectorSumB->numeroDeElementos > trackParams->PeriodoVelMed){
		valor = (valorSumB*)liberarPrimero(  Track->Stats->VectorSumB );
		free(valor);
	}

	// en función de la velocidad media anterior establecemos el estado del blob y actualizamos los contadores
	SetStateBlobTracked( Track, trackParams );
}

void SetStateBlobTracked( STTrack* Track, TrackingParams* trackParams ){

	float MovMed ;
	float MovDes;
	// si se ha calibrado, medidas en mm/s
	if( trackParams->pfTOmms){
		MovMed = Track->Stats->CMovMed*trackParams->pfTOmms;// de pixels/frame a mm/s
		MovDes = Track->Stats->CMovDes*trackParams->pfTOmms;
	}
	else{
		MovMed = Track->Stats->CMovMed;
		MovDes = Track->Stats->CMovDes;
	}


	if( (MovMed >= 0)&&(MovMed <= trackParams->NullActivityTh) ) {
		if( Track->Stats->EstadoBlob != 0 ){
			Track->Stats->EstadoBlobCount = 0;
		}
		Track->Stats->TimeBlobOff ++;
		Track->Stats->EstadoBlob = 0; // actividad nula
		Track->Stats->EstadoBlobCount ++;
	}
	else if( (MovMed > trackParams->NullActivityTh)&&(MovMed <= trackParams->LowActivityTh) ){
		if( Track->Stats->EstadoBlob != 1 ){
			Track->Stats->EstadoBlobCount = 0;
		}
		Track->Stats->TimeBlobOn ++;
		Track->Stats->EstadoBlob = 1;  // actividad baja
		Track->Stats->EstadoBlobCount ++;
	}
	else if(( MovMed > trackParams->LowActivityTh)&&(MovMed <= trackParams->MediumActivityTh) ){
		if( Track->Stats->EstadoBlob != 2 ){
			Track->Stats->EstadoBlobCount = 0;
		}
		Track->Stats->TimeBlobOn ++;
		Track->Stats->EstadoBlob = 2;  // actividad Media
		Track->Stats->EstadoBlobCount ++;
	}
	else if( MovMed > trackParams->MediumActivityTh){
		if( Track->Stats->EstadoBlob != 3 ){
			Track->Stats->EstadoBlobCount = 0; // Actividad alta
		}
		Track->Stats->TimeBlobOn ++;
		Track->Stats->EstadoBlob = 3;
		Track->Stats->EstadoBlobCount ++;
	}

	if( Track->Stats->Estado== KALMAN_CONTROL ){
		Track->Stats->EstadoBlob = 4; // oclusión
		Track->Stats->EstadoBlobCount ++;
	}

}
void updateFlyTracked( STTrack* Track, tlcde* Flies ){

	if( Track->Stats->Estado == CAM_CONTROL ){

		Track->Flysig->etiqueta = Track->id;
		Track->Flysig->Color = Track->Color;

		Track->Flysig->direccion = Track->z_k->data.fl[4];
		Track->Flysig->dir_med = Track->PhiMed;
		Track->Flysig->Vmed = Track->Vmed;
		Track->Flysig->dir_filtered =Track->x_k_Pos->data.fl[4];
		Track->Flysig->orientacion = corregirTita( Track->Flysig->dir_filtered, Track->Flysig->orientacion );
		Track->Flysig->VInst = Track->VInst;
		Track->Flysig->dstTotal = Track->Stats->dstTotal;
		Track->Flysig->Estado = Track->Stats->EstadoBlob;

		Track->Flysig->Stats->EstadoBlobCount = Track->Stats->EstadoBlobCount;
		Track->Flysig->Stats->CMovMed = Track->Stats->CMovMed;
		Track->Flysig->Stats->CMovDes = Track->Stats->CMovDes;
		Track->Flysig->Stats->EstadoTrack = Track->Stats->Estado;
		Track->Flysig->Stats->EstadoTrackCount = Track->Stats->EstadoCount;
		// contadores
		Track->Flysig->Stats->CountActiva = Track->Stats->TimeBlobOn;
		Track->Flysig->Stats->CountPasiva = Track->Stats->TimeBlobOff;

		Track->Flysig->anterior = (STFly*)Track->FlyActual;
		Track->FlyActual->siguiente = (STFly*)Track->Flysig;
	}
	if( Track->Stats->Estado ==  KALMAN_CONTROL){
		// actualizar fly ( Eliminarla??? )
		Track->Flysig->etiqueta = 0;
		Track->Flysig->Color = cvScalar( 255, 255, 255);

		Track->Flysig->direccion = Track->Flysig->orientacion;
		Track->Flysig->dstTotal = 0;

		Track->Flysig->Stats = NULL;  /// todo mirar esto

		generarFly( Track,Flies);

	}
}
/*!brief Genera flies ficticias en el buffer para los tracks que están en estado KALMAN_CONTROL ( prediccion )
 *
 *
 * @param lsTracks
 * @param Flies
 */
void generarFly( STTrack* Track, tlcde* Flies ){

		STFly* fly = NULL;
		fly = ( STFly *) malloc( sizeof( STFly));
		if ( !fly ) {error(4);	exit(1);}
		fly->Tracks = NULL;
		fly->Stats = NULL;

		fly->Tracks = ( tlcde * )malloc( sizeof(tlcde ));
		if ( !fly->Tracks ) {error(4);	exit(1);}
		iniciarLcde( fly->Tracks );
		anyadirAlFinal( Track, fly->Tracks);

		fly->Stats = ( STStatFly * )malloc( sizeof(STStatFly ));

		fly->siguiente = NULL;

		fly->etiqueta = Track->id; // Identificación del blob
		fly->Color = Track->Color; // Color para dibujar el blob
		fly->a = Track->FlyActual->a; // el tamaño de la actual
		fly->b = Track->FlyActual->b;


		fly->direccion = Track->x_k_Pos->data.fl[4]; // la dirección es la filtrada
		fly->dir_filtered = fly->direccion;
		fly->dir_med = fly->direccion;
		fly->Vmed = Track->Vmed;
		fly->orientacion = fly->direccion;
		fly->posicion.x = Track->x_k_Pos->data.fl[0];
		fly->posicion.y = Track->x_k_Pos->data.fl[1];
	//		fly->perimetro = cv::arcLength(contorno,0);
//		fly->Roi = rect;
		  // Flag para indicar que si el blob permanece estático ( 0 ) o en movimiento (1) u oculta (2)
		fly->num_frame = Track->Flysig->num_frame;
		fly->salto = false;	//!< Indica que la mosca ha saltado

		fly->Zona = 0; //!< Si se seleccionan zonas de interes en el plato,
		fly->failSeg = false;
		fly->flag_seg = false;
		fly->Px = 0;
		fly->areaElipse = CV_PI*fly->b*fly->a;

		// estadisticas
		fly->dstTotal = Track->Stats->dstTotal;
		fly->Estado = Track->Stats->EstadoBlob ; //
		fly->Stats->EstadoTrack = Track->Stats->Estado;
		fly->Stats->EstadoTrackCount = Track->Stats->EstadoCount;
		fly->Stats->EstadoBlobCount = Track->Stats->EstadoCount;
		fly->Stats->CMovMed = Track->Stats->CMovMed;
		fly->Stats->CMovDes = Track->Stats->CMovDes;

		fly->VInst = Track->VInst;

		fly->Stats->CountActiva = Track->Stats->TimeBlobOn;
		fly->Stats->CountPasiva = Track->Stats->TimeBlobOff;

		Track->Flysig->anterior = (STFly*)Track->FlyActual;
		Track->FlyActual->siguiente = (STFly*)Track->Flysig;

		Track->Flysig = fly;
		anyadirAlFinal(( STFly*) fly, Flies );
}

void mediaMovilVkalman( STTrack* Track, int Periodo ){

	valorSumVx* valorVx0;
	valorSumVx* valorVxT;

	valorSumVy* valorVy0;
	valorSumVy* valorVyT;

	// Para Vx
	irAlFinal(Track->VectorSumVx);
	irAlFinal(Track->VectorSumVy);
	valorVxT = ( valorSumVx*)obtenerActual( Track->VectorSumVx );
	valorVyT = ( valorSumVy*)obtenerActual( Track->VectorSumVy );
	Track->SumatorioVx = Track->SumatorioVx + valorVxT->sum;
	Track->SumatorioVy = Track->SumatorioVy + valorVyT->sum;
	if( Periodo>filterParams->PeriodoVmed) Periodo--;
	if(Track->VectorSumVx->numeroDeElementos > Periodo) {
	// los que hayan alcanzado el valor máximo restamos al sumatorio el primer valor
		irAlPrincipio( Track->VectorSumVx);
		irAlPrincipio( Track->VectorSumVy);
		valorVx0 = ( valorSumVx*)obtenerActual(  Track->VectorSumVx );
		valorVy0 = ( valorSumVy*)obtenerActual(  Track->VectorSumVy );

		Track->SumatorioVx =Track->SumatorioVx - valorVx0->sum; // sumatorio para la media
		Track->SumatorioVy =Track->SumatorioVy - valorVy0->sum; // sumatorio para la media


	}
	Track->Vxmed   =  Track->SumatorioVx / Periodo;
	Track->Vymed   =  Track->SumatorioVy / Periodo; // cálculo de la media
}

void mediaMovilStats( STStatTrack* Stats, int Periodo ){

	valorSumB* valor;
	valorSumB* valorT;

	// Se suma el nuevo valor
	irAlFinal(Stats->VectorSumB);
	valorT = ( valorSumB*)obtenerActual( Stats->VectorSumB );
	Stats->SumatorioMed = Stats->SumatorioMed + valorT->sum;
	Stats->SumatorioDes = Stats->SumatorioDes + valorT->sum*valorT->sum;

	if(Stats->VectorSumB->numeroDeElementos > Periodo){
	// los que hayan alcanzado el valor máximo restamos al sumatorio el primer valor
		irAlPrincipio( Stats->VectorSumB);
		valor = ( valorSumB*)obtenerActual(  Stats->VectorSumB );

		Stats->SumatorioMed = Stats->SumatorioMed - valor->sum; // sumatorio para la media
		Stats->SumatorioDes = Stats->SumatorioDes - valor->sum*valor->sum;

		Stats->CMovMed   =  Stats->SumatorioMed / Periodo; // cálculo de la media
		Stats->CMovDes =  sqrt( abs(Stats->SumatorioDes / Periodo - Stats->CMovMed*Stats->CMovMed ) ); // cálculo de la desviación
	}
}


void mostrarVMedia( tlcde* Vector){

	valorSumB* valor;
 	irAlFinal(Vector);
 	for(int i = 0; i <Vector->numeroDeElementos ; i++ ){
 		valor = (valorSumB*)obtener(i, Vector);
 		printf("%0.f\t", valor->sum);
 	}

 }





int deadTrack( tlcde* Tracks, int id ){

	STTrack* Track = NULL;

	valorSumB* val = NULL;
	valorSumVx* valVx = NULL;
	valorSumVy* valVy = NULL;
	Track = (STTrack*)borrarEl( id , Tracks);
	if(Track) {
		// matrices de kalman
		cvReleaseKalman(&Track->kalman);
		cvReleaseMat( &Track->x_k_Pre_ );
		cvReleaseMat( &Track->P_k_Pre_ );
		cvReleaseMat( &Track->Medida );
		cvReleaseMat(&Track->z_k );
		cvReleaseMat(&Track->Measurement_noise );
		if(Track->Stats->VectorSumB->numeroDeElementos > 0){
			val = (valorSumB *)borrar(Track->Stats->VectorSumB);
			while (val)
			{
				free(val);
				val = NULL;
				val = (valorSumB *)borrar(Track->Stats->VectorSumB);
			}
		}
		if(Track->VectorSumVx->numeroDeElementos > 0){
			valVx = (valorSumVx *)borrar(Track->VectorSumVx);
			while (valVx)
			{
				free(valVx);
				valVx = NULL;
				valVx = (valorSumVx *)borrar(Track->VectorSumVx);
			}
		}
		if(Track->VectorSumVy->numeroDeElementos > 0){
			valVy = (valorSumVy *)borrar(Track->VectorSumVy);
			while (valVy)
			{
				free(valVy);
				valVy = NULL;
				valVy = (valorSumVy *)borrar(Track->VectorSumVy);
			}
		}

		free( Track->Stats);

		free(Track);
		Track = NULL;
		return 1;
	}
	else return 0;
}

void visualizarKalman( STFrame* frameData, tlcde* lsTracks, bool dataOn) {

	STTrack* Track;


		for(int i = 0; i < lsTracks->numeroDeElementos ; i++){
			Track = (STTrack*)obtener(i, lsTracks);
			// EN CONSOLA
			if(dataOn ){
				printf("\n*********************** FRAME %d; BLOB NUMERO %d ************************",frameData->num_frame,Track->id);
				showKalmanData( Track );
			}
			if( obtenerVisParam( SHOW_KALMAN ) ){
				double magnitude = 30;
				// EN IMAGEN
				cvCircle(ImKalman,cvPoint(cvRound(Track->z_k->data.fl[0]),cvRound(Track->z_k->data.fl[1] )),1,CVX_BLUE,-1,8); // observado
				cvLine( frameData->ImKalman,
					cvPoint( Track->z_k->data.fl[0],Track->z_k->data.fl[1]),
					cvPoint( cvRound( Track->z_k->data.fl[0] + magnitude*cos(Track->z_k->data.fl[4]*CV_PI/180)),
							 cvRound( Track->z_k->data.fl[1] - magnitude*sin(Track->z_k->data.fl[4]*CV_PI/180))  ),
					CVX_RED,
					1, CV_AA, 0 );
				cvCircle(ImKalman,cvPoint(cvRound(Track->x_k_Pre_->data.fl[0]),cvRound(Track->x_k_Pre_->data.fl[1] )),1,CVX_RED,-1,8); // predicción en t
				cvCircle(ImKalman,cvPoint(cvRound(Track->x_k_Pos->data.fl[0]),cvRound(Track->x_k_Pos->data.fl[1])),1,CVX_WHITE,-1,8); // Corrección en t
				// dirección de kalman (dirección fliltrada/corregida )
				cvLine(frameData->ImKalman,
						cvPoint( Track->x_k_Pos->data.fl[0],Track->x_k_Pos->data.fl[1]),
					cvPoint( cvRound( Track->x_k_Pos->data.fl[0] + magnitude*cos(Track->x_k_Pos->data.fl[4]*CV_PI/180)),
							 cvRound( Track->x_k_Pos->data.fl[1] - magnitude*sin(Track->x_k_Pos->data.fl[4]*CV_PI/180))  ),
					CVX_GREEN,
					1, CV_AA, 0 );
				cvCircle(ImKalman,cvPoint(cvRound(Track->x_k_Pre->data.fl[0]),cvRound(Track->x_k_Pre->data.fl[1])),1,CVX_GREEN,-1,8); // predicción t+1

				visualizarId( frameData->ImKalman,cvPoint( Track->z_k->data.fl[0],Track->z_k->data.fl[1]), Track->id, CVX_WHITE);

			}

		}
		if( obtenerVisParam( SHOW_KALMAN ) ) cvAdd(ImKalman,frameData->ImKalman,frameData->ImKalman );



}

void showKalmanData( STTrack *Track){

	printf("\n\nReal State: ");
	printf("\n\tCoordenadas: ( %f , %f )",Track->Medida->data.fl[0],Track->Medida->data.fl[1]);
	printf("\n\tVelocidad: ( %f , %f )",Track->Medida->data.fl[2],Track->Medida->data.fl[3]);
	printf(" Vector Media movil Vx:\t");
	mostrarVMedia(Track->VectorSumVx);
	printf(" Vector Media movil Vy:\t");
	mostrarVMedia(Track->VectorSumVy);
	printf("\n\tVMedia: ( %f , %f )",Track->Vxmed,Track->Vymed);
	printf("\n\t|VMedia|:  %f ",Track->Vmed );
	printf("\n\tDirección:  %f ",Track->Medida->data.fl[4]);
	printf("\n\tDirMed:  %f ",Track->PhiMed);



	printf("\n\n Observed State: "); // sumandole el error de medida.
	printf("\n\tCoordenadas: ( %f , %f )",Track->z_k->data.fl[0],Track->z_k->data.fl[1]);
	printf("\n\tVelocidad: ( %f , %f )",Track->z_k->data.fl[2],Track->z_k->data.fl[3]);
	printf("\n\tDirección:  %f ",Track->z_k->data.fl[4]);

	printf("\n Media Measurement noise:\n\n\t%0.f\n\t%0.f\n\t%0.f\n\t%0.f\n\t%0.f",
			Track->Measurement_noise->data.fl[0],
			Track->Measurement_noise->data.fl[1],
			Track->Measurement_noise->data.fl[2],
			Track->Measurement_noise->data.fl[3],
			Track->Measurement_noise->data.fl[4]);

	printf("\n Error Measurement noise:\n\n\t%0.3f\t0\t0\t0\t0\n\t0\t%0.3f\t0\t0\t0\n\t0\t0\t%0.3f\t0\t0\n\t0\t0\t0\t%0.3f\t0\n\t0\t0\t0\t0\t%0.3f",
			Track->Measurement_noise_cov->data.fl[0],
			Track->Measurement_noise_cov->data.fl[6],
			Track->Measurement_noise_cov->data.fl[12],
			Track->Measurement_noise_cov->data.fl[18],
			Track->Measurement_noise_cov->data.fl[24]);

	printf("\n Error Process noise:\n\n\t%0.3f\t0\t0\t0\t0\n\t0\t%0.3f\t0\t0\t0\n\t0\t0\t%0.3f\t0\t0\n\t0\t0\t0\t%0.3f\t0\n\t0\t0\t0\t0\t%0.3f",
			Track->kalman->process_noise_cov->data.fl[0],
			Track->kalman->process_noise_cov->data.fl[6],
			Track->kalman->process_noise_cov->data.fl[12],
			Track->kalman->process_noise_cov->data.fl[18],
			Track->kalman->process_noise_cov->data.fl[24]);


	printf("\n\n Predicted State t:");
	printf("\n\tCoordenadas: ( %f , %f )",Track->x_k_Pre_->data.fl[0],Track->x_k_Pre_->data.fl[1]);
	printf("\n\tVelocidad: ( %f , %f )",Track->x_k_Pre_->data.fl[2],Track->x_k_Pre_->data.fl[3]);
	printf("\n\tDirección:  %f ",Track->x_k_Pre_->data.fl[4]);
	printf("\n Error Cov Predicted:\n\n\t%0.3f\t0\t0\t0\t0\n\t0\t%0.3f\t0\t0\t0\n\t0\t0\t%0.3f\t0\t0\n\t0\t0\t0\t%0.3f\t0\n\t0\t0\t0\t0\t%0.3f",
			Track->P_k_Pre_->data.fl[0],
			Track->P_k_Pre_->data.fl[6],
			Track->P_k_Pre_->data.fl[12],
			Track->P_k_Pre_->data.fl[18],
			Track->P_k_Pre_->data.fl[24]);

	printf("\n\n Corrected State:" );
	printf("\n\tCoordenadas: ( %f , %f )",Track->x_k_Pos->data.fl[0],Track->x_k_Pos->data.fl[1]);
	printf("\n\tVelocidad: ( %f , %f )",Track->x_k_Pos->data.fl[2],Track->x_k_Pos->data.fl[3]);
	printf("\n\tDirección:  %f ",Track->x_k_Pos->data.fl[4]);
	printf("\n Error Cov Corrected:\n\n\t%0.3f\t0\t0\t0\t0\n\t0\t%0.3f\t0\t0\t0\n\t0\t0\t%0.3f\t0\t0\n\t0\t0\t0\t%0.3f\t0\n\t0\t0\t0\t0\t%0.3f",
			Track->P_k_Pos->data.fl[0],
			Track->P_k_Pos->data.fl[6],
			Track->P_k_Pos->data.fl[12],
			Track->P_k_Pos->data.fl[18],
			Track->P_k_Pos->data.fl[24]);

	printf("\n\n Predicted State t+1:");
	printf("\n\tCoordenadas: ( %f , %f )",Track->x_k_Pre->data.fl[0],Track->x_k_Pre->data.fl[1]);
	printf("\n\tVelocidad: ( %f , %f )",Track->x_k_Pre->data.fl[2],Track->x_k_Pre->data.fl[3]);
	printf("\n\tDirección:  %f ",Track->x_k_Pre->data.fl[4]);
	printf("\n Error Cov Predicted:\n\n\t%0.3f\t0\t0\t0\t0\n\t0\t%0.3f\t0\t0\t0\n\t0\t0\t%0.3f\t0\t0\n\t0\t0\t0\t%0.3f\t0\n\t0\t0\t0\t0\t%0.3f",
			Track->P_k_Pre->data.fl[0],
			Track->P_k_Pre->data.fl[6],
			Track->P_k_Pre->data.fl[12],
			Track->P_k_Pre->data.fl[18],
			Track->P_k_Pre->data.fl[24]);
	printf("\n\n");
}

void liberarTracks( tlcde* lista){
	// Borrar todos los elementos de la lista
	// Comprobar si hay elementos
	int track;
	if(lista == NULL || lista->numeroDeElementos<1)  return;
	track = deadTrack( lista, 0);
	while( track) track = deadTrack( lista, 0);

}

int dejarId( STTrack* Track, tlcde* identities ){
	Identity *Id = NULL;
	Id = ( Identity* )malloc( sizeof(Identity ));
	if( !Id){error(4);return 0 ;}
	Id->etiqueta = Track->id;
	Id->color = Track->Color;
	anyadirAlFinal( Id , identities );
	return 1;
}

void asignarNuevaId( STTrack* Track, tlcde* identities){
	Identity *id;
	id = (Identity* )borrarEl( identities->numeroDeElementos - 1, identities);
	Track->id = id->etiqueta;
	Track->Color = id->color;
	free(id);
	id = NULL;
}

void SetKalmanFilterParams( TrackingParams* trackParams ){

    //init parameters
	config_t cfg;
	config_setting_t *setting;
	char settingName[30];
	char configFile[30];
	char settingFather[30];



	int EXITO;
	int DEFAULT = false;

	// Reservar memoria
	if( !filterParams){
		filterParams = ( FilterParams *) malloc( sizeof( FilterParams) );
		if(!filterParams) {error(4); return;}
		filterParams->Cam = ( CamParams *) malloc( sizeof( CamParams) );
		if(!filterParams->Cam) {error(4); return;}
		filterParams->Kal = ( KalParams *) malloc( sizeof( KalParams) );
		if(!filterParams->Kal) {error(4); return;}
	}

	fprintf(stderr, "\nCargando parámetros del filtro de Kalman...");
	config_init(&cfg);

	sprintf( configFile, "config.cfg");
	sprintf( settingFather,"Kalman" );
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
			// comprobar los hijos
			sprintf( settingFather,"Kalman.Inicializacion" );
			setting = config_lookup(&cfg, settingFather);
			if(setting) {
				sprintf( settingFather,"Kalman.CamControl" );
				setting = config_lookup(&cfg, settingFather);
				if(setting){
					sprintf( settingFather,"Kalman.KalmanControl" );
					setting = config_lookup(&cfg, settingFather);
				}
			}
			if( !setting ) {
				DEFAULT = true;
				fprintf(stderr, "Error.No se ha podido leer el campo %s.\n"
							" Estableciendo valores por defecto.\n",settingFather);
			}

		}
		else {

			DEFAULT = true;
			fprintf(stderr, "Error.No se ha podido leer el campo %s.\n"
							" Estableciendo valores por defecto.\n",settingFather);
		}
	}

	if( DEFAULT ) {
		sprintf( settingFather,"Kalman" );
		SetDefaultKFilterParams( trackParams );
	}
	/* Valores leídos del fichero de configuración. Algunos valores puedes ser establecidos por defecto si se indica
	 * expresamente en el fichero de configuración. Si el valor es erroneo o no se encuentra la variable, se establecerán
	 * a los valores por defecto.
	 */
	else{
		double val;
		 /* Get the store name. */
		sprintf( settingFather,"Kalman.Inicializacion" );
		setting = config_lookup(&cfg, settingFather);
		sprintf(settingName, "Velocidad");
		// si no se encuentra la setting
		if (!config_setting_lookup_float(setting, settingName,
				&val)) {
			// si hay medida de calibración se establece la vel en mm/s y se pasa a pixels frame
			if( trackParams->mmTOpixel) filterParams->Velocidad = 10.6*trackParams->mmsTOpf ;
			// si no se establece en pixels/frame
			else filterParams->Velocidad = 5.6;
			fprintf(
					stderr,
					"No se encuentra la variable %s en el archivo de configuración o el tipo de dato es incorrecto.\n "
						"Establecer por defecto a %0.2f \n",
					settingName, filterParams->Velocidad);

		} // si el valor está fuera de límites
		else if(!val	|| val < 0 ){
			// si se ha calibrado se establece la vel en mm/s y se pasa a pixels frame
			if( trackParams->mmTOpixel) filterParams->Velocidad = 10.6*trackParams->mmsTOpf ;
			// si no se establece en pixels/frame
			else filterParams->Velocidad = 5.6;
			fprintf(stderr,
					"El valor de %s está fuera de límites\n "
						"Establecer por defecto %s a %0.1f \n",
					settingName, settingName,
					filterParams->Velocidad);
		}
		// valor correcto
		else{
			// si se ha calibrado
			if( trackParams->mmsTOpf) filterParams->Velocidad = val*trackParams->mmsTOpf; //de mm/s a pixels/frame
			// en caso contrario
			else filterParams->Velocidad = (float)val;
		}

		sprintf(settingName, "V_Angular");
		if (!config_setting_lookup_float(setting, settingName,
				&val)) {
			filterParams->V_Angular = 0;
			fprintf(
					stderr,
					"No se encuentra la variable %s en el archivo de configuración o el tipo de dato es incorrecto.\n "
						"Establecer por defecto a %0.2f \n",
					settingName, filterParams->V_Angular);

		} else 	filterParams->V_Angular = (float)val;

		sprintf(settingName, "MaxJump" );
		if (!config_setting_lookup_float(setting, settingName,
				&val)) {
			if( trackParams->mmTOpixel) filterParams->MaxJump = cvRound(50*trackParams->mmTOpixel) ; // en mm
			else filterParams->MaxJump = 50; // en pixels
			fprintf(
					stderr,
					"No se encuentra la variable %s en el archivo de configuración o el tipo de dato es incorrecto.\n "
						"Establecer por defecto a %0.2f \n",
					settingName, filterParams->MaxJump);

		} else if(!val	|| val < 0 ){
			if( trackParams->mmTOpixel) filterParams->MaxJump = cvRound(50*trackParams->mmTOpixel) ;
			else filterParams->MaxJump = 50;
			fprintf(stderr,
					"El valor de %s está fuera de límites\n "
						"Establecer por defecto %s a %0.1f \n",
					settingName, settingName,
					filterParams->MaxJump);
		}
		else{
			if( trackParams->mmsTOpf) filterParams->MaxJump = cvRound((float)val*trackParams->mmTOpixel) ; //de mm a pixels
			else filterParams->MaxJump = (float)val;
		}

		sprintf(settingName, "InitTime");
		if (!config_setting_lookup_float(setting, settingName,
				&val)) {
			if( trackParams->mmTOpixel) filterParams->InitTime = cvRound(1*trackParams->FPS) ; // de seg a frames
			else filterParams->InitTime = 15; // en frames

			filterParams->InitTime = 15;
			fprintf(
					stderr,
					"No se encuentra la variable %s en el archivo de configuración o el tipo de dato es incorrecto.\n "
						"Establecer por defecto a %d frames \n",
					settingName, filterParams->InitTime );

		} else if( val < 0 ){

			if( trackParams->mmTOpixel) filterParams->InitTime = cvRound(1*trackParams->FPS) ; // de seg a frames
			else filterParams->InitTime = 15; // en frames
				fprintf(stderr,
						"El valor de %s está fuera de límites\n "
							"Establecer por defecto %s a %d \n",
						settingName, settingName,
						filterParams->InitTime );
		}
		else{
			filterParams->InitTime = cvRound( (float)val * trackParams->FPS); // a frames
		}


	}

	SetPrivateKFilterParams(  );

	ShowKalmanFilterParams( settingFather );
	config_destroy(&cfg);
}

void SetDefaultKFilterParams( TrackingParams* trackParams  ){

	// si hay medida de calibración se establece la vel en mm/s y se pasa a pixels frame
	if( trackParams->mmTOpixel) filterParams->Velocidad = 10.6*trackParams->mmsTOpf;
	// si no se establece en pixels/frame
	else filterParams->Velocidad = 5.6;
	filterParams->V_Angular = 0;
	if( trackParams->mmTOpixel) filterParams->MaxJump = cvRound(20*trackParams->mmTOpixel) ; // en mm
	else filterParams->MaxJump = 50; // en pixels
	if( trackParams->mmTOpixel) filterParams->InitTime = cvRound(1*trackParams->FPS) ; // de seg a frames
	else filterParams->InitTime = 15; // en frames
	if( trackParams->mmTOpixel) filterParams->PeriodoVmed = cvRound(0.25*trackParams->FPS) ; // de seg a frames
	else filterParams->PeriodoVmed = 5; // en frames

	filterParams->Cam->alpha_Rk0 = 1;
	filterParams->Cam->AlphaR_phi0 = 0.5;
	filterParams->Cam->alpha_Rk = 1;
	filterParams->Cam->AlphaR_phi = 1;

	filterParams->Kal->alpha_Rk = 1;
	filterParams->Kal->R_phiZk = 10000;

}

void ShowKalmanFilterParams( char* Campo ){

	printf(" \nVariables para el campo %s : \n", Campo);
	printf(" -Velocidad = %0.2f pixels/mm \n", filterParams->Velocidad);
	printf(" -V_Angular = %0.2f º/frame \n", filterParams->V_Angular);
	printf(" -MaxJump = %0.1f pixels\n", filterParams->MaxJump);
	printf(" -InitTime = %d frames \n", filterParams->InitTime);

}

void SetPrivateKFilterParams(  ){
	//void onTrackbarSlide(pos, BGModelParams* Param) {
	//   Param->ALPHA = pos / 100;
	//}
	filterParams->Q = 2;

	filterParams->Cam->alpha_Rk0 = 1;
	filterParams->Cam->g_slider_alpha_Rk0 = 10;
	filterParams->Cam->alpha_Rk = 1;
	filterParams->Cam->g_slider_alpha_Rk = 10;
	filterParams->Cam->AlphaR_phi0 = 1;
	filterParams->Cam->g_slider_AlphaR_phi0 = 10;
	filterParams->Cam->AlphaR_phi = 1;
	filterParams->Cam->g_slider_AlphaR_phi = 10;

	filterParams->Kal->alpha_Rk = 1;
	filterParams->Kal->g_slider_alpha_Rk = 10;
	filterParams->Kal->R_phiZk = 1000;
	filterParams->Kal->g_slider_R_phiZk = 1000;
	filterParams->createTrackbars = true;

	filterParams->PeriodoVmed = 5 ;
}

void crearTrackBars(){

	if( filterParams->createTrackbars ){

		// en el intervalo [ 1 y 10 ) => [0.1, 1) => disminuye ruido
		// en el intervalo ( 10, 100] => (1 , 10 ) => aumenta ruido hasta un factor de 10
		cvCreateTrackbar( "AlphaRk0 = trackVal/10","Filtro de Kalman", &filterParams->Cam->g_slider_alpha_Rk0, 100, onTrackbarAlphaRkCam0);

		//  [ 0 y 10 ] => [0.1, 1] => disminuye ruido
		cvCreateTrackbar( "AlphaR_Phi0 = trackVal/10","Filtro de Kalman", &filterParams->Cam->g_slider_AlphaR_phi0, 10, onTrackbarAlphaRPhiCam0);

		cvCreateTrackbar( "AlphaRk = trackVal/10","Filtro de Kalman", &filterParams->Cam->g_slider_alpha_Rk, 100, onTrackbarAlphaRkCam);

		cvCreateTrackbar( "AlphaR_Phi = trackVal/10","Filtro de Kalman", &filterParams->Cam->g_slider_AlphaR_phi, 10, onTrackbarAlphaRPhiCam);

		// en el intervalo [ 1 y 10 ) => [0.1, 1) => disminuye ruido
		// en el intervalo ( 10, 1000] => (1 , 100 ) => aumenta ruido hasta un factor de 100
		cvCreateTrackbar( "AlphaRk_Kal = trackVal/10","Filtro de Kalman", &filterParams->Kal->g_slider_alpha_Rk, 1000, onTrackbarAlphaRkKal);

		// en el intervalo [ 1 y 10 ) => [100, 1000) => disminuye ruido
		// en el intervalo ( 10, 1000] => (1000 , 10000 ) => aumenta ruido hasta un factor de 100
		cvCreateTrackbar( "R_PhiKal","Filtro de Kalman", &filterParams->Kal->g_slider_R_phiZk, 10000, onTrackbarRPhiKal);

		cvCreateTrackbar( "T_Vmed","Filtro de Kalman", &filterParams->PeriodoVmed, 60, onTrackbarVmed);
	}

}

void onTrackbarAlphaRkCam0(int val ){

	if(val == 0) val =1;
	filterParams->Cam->alpha_Rk0 = (float)val/10;

}
void onTrackbarAlphaRPhiCam0(int val ){

	if(val == 0) val =1;
	filterParams->Cam->AlphaR_phi0 = (float)val/10;

}
void onTrackbarAlphaRkCam(int val ){

	if(val == 0) val =1;
	filterParams->Cam->alpha_Rk = (float)val/10;

}
void onTrackbarAlphaRPhiCam(int val ){

	if(val == 0) val =1;
	filterParams->Cam->AlphaR_phi = (float)val/10;

}

void onTrackbarAlphaRkKal(int val ){
	if(val == 0) val =1;
	filterParams->Kal->alpha_Rk = (float)val/10;
}

void onTrackbarRPhiKal(int val ){
	if(val == 0) val =1;
	filterParams->Kal->alpha_Rk = (float)val*100;
}
void onTrackbarVmed(int val ){

	if(val == 0) val =1;
}
/// Limpia de la memoria las imagenes usadas durante la ejecución
void DeallocateKalman( tlcde* lista ){

	cvReleaseImage( &ImKalman);
	free( filterParams->Cam);
	free( filterParams->Kal);
	free( filterParams );


	liberarTracks( lista);

}

void copyMat (CvMat* source, CvMat* dest){

	int i,j;

	for (i=0; i<source->rows; i++)
		for (j=0; j<source->cols;j++)
			dest->data.fl[i*source->cols+j]=source->data.fl[i*source->cols+j];

}
