/*
 * Kalman.cpp
 *
 *  Created on: 18/11/2011
 *      Authores: Rubén Chao Chao, Germán Garcia Vázquez
 *
 */


#include "Kalman.hpp"

// componentes del vector Zk con las nuevas medidas
// variables para establecer la nueva medida
static float distancia; // sqrt( Ax² + Ay² )

// Parámetros
FilterParams* filterParams = NULL;
//Imagenes
IplImage* ImKalman = NULL;

void Kalman(STFrame* frameData, tlcde* lsIds,tlcde* lsTracks, TrackingParams* trackParams) {

	tlcde* Flies;
	STTrack* Track;
	STFly* Fly = NULL;

	// cargar datos del frame
	Flies=frameData->Flies;

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
			if( Track->Stats->Estado != SLEEPING ){
				////////////////////// GENERAR MEDIDA ////////////////////
				// Generar la nueva medida en función del estado
				generarMedida( frameData->FG,Track, Track->Stats->Estado);
				// Generar ruido asociado a la nueva medida
				generarRuido( Track, Track->Stats->Estado );

				////////////////////// FASE DE CORRECCIÓN ////////////////////////
				cvKalmanCorrect( Track->kalman, Track->z_k);

			}
			irAlSiguiente( lsTracks );
		}
		/////////////// ACTUALIZACIÓN DE TRACKS CON LOS NUEVOS DATOS //////////////
		updateTracks( lsTracks,frameData->Flies, trackParams );

	}


	// CREAR NUEVOS TRACKS si es necesario y limpiar lista flies
	// si hay mosca/s sin asignar a un track/s (id = -1), se crea un nuevo track para esa mosca
	if( Flies->numeroDeElementos > 0) irAlPrincipio( Flies );
	for(int i = 0; i< Flies->numeroDeElementos ;i++ ){
		Fly = (STFly*)obtenerActual( Flies );
		if( Fly->etiqueta == -1 ){
			Track = initTrack( Fly, lsIds , 1 );
			anyadirAlFinal( Track , lsTracks );
		}
		if( Fly->etiqueta == 0){
			Fly = (STFly*) borrar( Flies );
			deadFly( Fly );
			irAlAnterior( Flies);
		}
		irAlSiguiente( Flies);
	}
	ordenarListaFlies( Flies );

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
	Track->Stats->TShape = 200;
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
	Track->VectorV = ( tlcde * )malloc( sizeof(tlcde ));
	if( !Track->VectorV ) {error(4);exit(1);}
	iniciarLcde( Track->VectorV );

	Track->SumatorioVx = 0; //! sumatorio para la velocidad media en T seg
	Track->SumatorioVy = 0; //! sumatorio para la velocidad media en T seg

	Track->kalman = initKalman( Fly , fps);

	Track->x_k_Pre_ = cvCreateMat( 6,1, CV_32FC1 );
	Track->P_k_Pre_ = cvCreateMat( 6,6, CV_32FC1 );
	cvZero(Track->x_k_Pre_);
	cvZero(Track->P_k_Pre_);

	Track->x_k_Pos = Track->kalman->state_post ;
	Track->P_k_Pos = Track->kalman->error_cov_post;

	Track->x_k_Pre = NULL; // es creada por kalman
	Track->P_k_Pre = Track->kalman->error_cov_pre;

	Track->Medida = cvCreateMat( 6, 1, CV_32FC1 );
	Track->Measurement_noise = cvCreateMat( 6, 1, CV_32FC1 );
	Track->Measurement_noise_cov = Track->kalman->measurement_noise_cov;
	Track->z_k = cvCreateMat( 6, 1, CV_32FC1 );
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


	CvKalman* Kalman = cvCreateKalman(6,6,0);

	// Inicializar las matrices parámetros para el filtro de Kalman
	// en la primera iteración iniciamos la dirección con la orientación

	Vx= filterParams->Velocidad*cos(Fly->orientacion*CV_PI/180);
	Vy=-filterParams->Velocidad*sin(Fly->orientacion*CV_PI/180);

	// x = x0 + Vx dt + Wk
	// y = y0 + Vy dt + Wk
	// Vx = Vmed*cos(phi) + Wk
	// Vy = -Vmed*sin(phi) + Wk
	// phi = phi0 + w dt + Wk
	// w = wphi0 + Wk;
	// con Wk->N(0,Q): Q = f(F,B y H).

	const float F[] = {	1,0,dt,0,0,0,
						0,1,0,dt,0,0,
						0,0,1, 0,0,0,
						0,0,0, 1,0,0,
						0,0,0, 0,1,dt,
						0,0,0, 0,0,1 }; // Matriz de transición F


	filterParams->R_x = 10;	// implementar método de cuantificación del ruido.
	filterParams->R_y = 10;

	//2) Velocidad
	filterParams->R_Vx = 20;
	filterParams->R_Vy = 20;
	filterParams->R_phiZk = 180;
	filterParams->R_w = 360 ;

	// Matriz R inicial. Errores en medidas. ( Rx, Ry,RVx, RVy, Rphi ). Al inicio el error en el angulo es de +-180º
	const float R_inicial[] = {	filterParams->R_x,0,0,0,0,0,
								0,filterParams->R_y,0,0,0,0,
								0,0,filterParams->R_Vx,0,0,0,
								0,0,0,filterParams->R_Vy,0,0,
								0,0,0,0,filterParams->R_phiZk,0,
								0,0,0,0,0,filterParams->R_w};//{50,50,50,50,180};

	const float X_k[] = {	Fly->posicion.x,
							Fly->posicion.y,
							Vx,
							Vy,
							Fly->orientacion,
							filterParams->V_Angular};// Matiz de estado inicial

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


void generarMedida( IplImage* FG, STTrack* Track, int EstadoTrack ){

	STFly* flyActual = Track->FlyActual;
	STFly* flySig = Track->Flysig;

	//Caso 0: No se ha encontrado ninguna posible asignación. No hay medida.
	if ( EstadoTrack == CAM_CONTROL ){
		// asumimos que la dirección varia poco de un frame al siguiente

		// velocidad instantánea
		Track->VxInst =  (flySig->posicion.x - flyActual->posicion.x) / 1;
		Track->VyInst = (flySig->posicion.y - flyActual->posicion.y) / 1;
		EUDistance( Track->VxInst, Track->VyInst, &Track->PhiInst, &Track->VInst );
		distancia = Track->VInst;
		if( Track->PhiInst == -1 ) Track->PhiInst = flyActual->orientacion;

		//Obtener dirección usando las posiciones de los frames anteriores
		//Se intenta disminuir el error retrasando el origen del vector
		//velocidad hasta un límite. Se calcula el error debido a la resolución.
		// si la velocidad es baja, se busca disminuir el error retrasando el origen
		// del vector velocidad hasta reducir el error por debajo de un punto o
		// bien alcanzar el número máximo de frames de retroceso.
		obtenerDir( Track, &Track->PhiMed, &Track->errorVPhi );

		corregirDir( Track->x_k_Pre->data.fl[4],&Track->PhiMed );
		// Media móvil para Velocidad en T seg
		CalcularVmed( Track, &Track->Vmed,&Track->Vxmed, &Track->Vymed, &Track->errorVx,&Track->errorVy);
		// si la velocidad es nula
		if( Track->Vmed < 1 ){
			Track->PhiMed = flyActual->orientacion;
			Track->errorVPhi = 0;
		}

		// wphi = phi(t) - phi(t-1)
		Track->wPhi = Track->PhiMed - Track->z_k->data.fl[4];
		const float Zk[] = { flySig->posicion.x, flySig->posicion.y, Track->Vxmed, Track->Vymed, Track->PhiMed, Track->wPhi };
		memcpy( Track->Medida->data.fl, Zk, sizeof(Zk)); // Medida;
		// ESTABLECER Z_K
		const float V[] = {0,0,0,0,0,0}; // media del ruido
		memcpy( Track->Measurement_noise->data.fl, V, sizeof(V));

		cvGEMM(Track->kalman->measurement_matrix, Track->Medida,1, Track->Measurement_noise, 1, Track->z_k,0 ); // Zk = H Medida + V

	}
	else if( EstadoTrack == KALMAN_CONTROL){
		static int Out;
		if( Track->Stats->EstadoCount == 0 ){
			Track->KalInitPos = cvPoint(Track->x_k_Pre->data.fl[0],Track->x_k_Pre->data.fl[1]);
			Track->KalInitPhi = Track->PhiMed;
			Track->TimeToPhase1 = filterParams->Kal->MaxTimeToPhase1;
		}
		// Si la predicción de la posición del objetivo ha salido del área del área de mayor incertidumbre,
		// comenzar la cuenta para regresar la fly al centro del blob
		Out = verificarSalida( FG, Track->x_k_Pre->data.fl[0],Track->x_k_Pre->data.fl[1]);
		if( Out ){
			if( Track->TimeToPhase1 > 0 ) Track->TimeToPhase1 --;
		}

		cvSetIdentity( Track->kalman->process_noise_cov,cvScalar( filterParams->Q ) ); // Q: error asociado al modelo del sistema Wk->N(0,Q): Q = f(F,B y H).

		// S la predicción no ha salido del área del blob
		if(  Track->TimeToPhase1 ){
			// si no es así
			Track->Medida->data.fl[0] = Track->x_k_Pre->data.fl[0];
			Track->Medida->data.fl[1] =  Track->x_k_Pre->data.fl[1];
			Track->Medida->data.fl[2] =  Track->Vmed*cos(Track->x_k_Pre->data.fl[4]*CV_PI/180);
			Track->Medida->data.fl[3] =  -Track->Vmed*sin(Track->x_k_Pre->data.fl[4]*CV_PI/180);
			Track->Medida->data.fl[4] =  Track->x_k_Pre->data.fl[4];
			if(Track->x_k_Pre->data.fl[5] > filterParams->w_max) Track->Medida->data.fl[5] = filterParams->w_max;
			else  Track->Medida->data.fl[5] = Track->x_k_Pre->data.fl[5];

		}else{

			//Si ha salido, la nueva medida dependerá del eje mayor de la elipse del blob.
			//Si el area del blob es mayor de dos veces el área de la fly, se retrocederá por la
			// trayectoria de la fly hasta la posición de entrada.
//			if( Track->Flysig->b > 2*Track->Stats->b ){
//
//			}else{
				// la posición será el centro del blob
				Track->Medida->data.fl[0] = Track->Flysig->posicion.x;
				Track->Medida->data.fl[1] =  Track->Flysig->posicion.y;

//			}
			// disminuimos la velocidad
			Track->Medida->data.fl[2] =  Track->Vmed*cos(Track->x_k_Pre->data.fl[4]*CV_PI/180);
			Track->Medida->data.fl[3] =  -Track->Vmed*sin(Track->x_k_Pre->data.fl[4]*CV_PI/180);
			Track->Medida->data.fl[4] =  Track->KalInitPhi;
			Track->Medida->data.fl[5] =  0;

		}
		// en caso contrario establecer la pos anterior inmediatamente anterior.
		// ESTABLECER Z_K
		const float V[] = {0,0,0,0,0,0}; // media del ruido
//		cvCopy( V, Track->Measurement_noise );
		memcpy( Track->Measurement_noise->data.fl, V, sizeof(V));

		cvGEMM(Track->kalman->measurement_matrix, Track->Medida,1, Track->Measurement_noise, 1, Track->z_k,0 ); // Zk = H Medida + V

	}
	else if( EstadoTrack == SLEEPING){

	}
	return ;
}

void obtenerDir( STTrack* Track, float *PhiMed, float* errorVPhi ){

	//Obtener dirección usando las posiciones de los frames anteriores

	STFly* flySig = Track->Flysig;
	STFly* flyAnterior = NULL;


	float Ax;
	float Ay;
	float errorV;
	float phiMed;
	flyAnterior = (STFly*)Track->FlyActual;
	for( int i = filterParams->T_Vmed-1; i > 0; i--){
		Ax = flySig->posicion.x - flyAnterior->posicion.x;
		Ay = flySig->posicion.y - flyAnterior->posicion.y;
		errorV = errorR_PhiZkV( Ax, Ay );
		if( errorV < filterParams->MaxVPhiError) break;
		else if(flyAnterior->anterior) flyAnterior = (STFly*)flyAnterior->anterior;
		else break;
	}
	EUDistance( Ax, Ay, &phiMed, NULL );
	*errorVPhi = errorV;
	*PhiMed = phiMed;

}

void corregirDir( float phiXk,float* phiZk ){

	if( abs(phiXk - *phiZk ) > 180 ){
		int n = (int)phiXk/360;
		if( n == 0) n = 1;
		if( phiXk > *phiZk ) *phiZk = *phiZk + n*360;
		else *phiZk = *phiZk - n*360;
	}

}
void CalcularVmed( STTrack* Track, float* VMed, float* Vxmed, float* Vymed, float* errorVx, float* errorVy  ){

	STFly* flyActual = Track->FlyActual;
	STFly* flySig = Track->Flysig;

	static float vxdes;
	static float vydes;
	// Media mǘvil para V en T seg
	// calcular Vt
	  // al ser siempre  t = 1 frame, la distancia en pixels coincide con Vt
	// Obtener el nuevo valor
	valorV* V =  ( valorV *) malloc( sizeof(valorV));

	V->Vx = flySig->posicion.x - flyActual->posicion.x;
	V->Vy = flySig->posicion.y - flyActual->posicion.y;

	anyadirAlFinal(V, Track->VectorV );

//	calculo de la velocidad media de los  frames equivalentes al periodo establecido
	mediaMovilVkalman( Track, Track->VectorV->numeroDeElementos);
	// si el vector llega a fps elementos eliminamos el primero.
	obtenerDes(Track->VectorV,Track->Vxmed, Track->Vymed, &vxdes, &vydes);

	EUDistance( Track->Vxmed, Track->Vymed, NULL, &Track->Vmed );

	*errorVx= vxdes;
	*errorVy = vydes;
}

void obtenerDes( tlcde* Vector, float Vxmed, float Vymed, float* errorVx, float* errorVy){

	float sum2x = 0;
	float sum2y = 0;

	valorV* V;

	irAlPrincipio( Vector);
	for(int i = 0;i < Vector->numeroDeElementos; i++){
		V = ( valorV*)obtenerActual(  Vector );
		sum2x = sum2x + V->Vx*V->Vx;
		sum2y = sum2y + V->Vy*V->Vy;
		irAlSiguiente( Vector);
	}
	*errorVx = sqrt( abs( sum2x/Vector->numeroDeElementos) - Vxmed*Vxmed);
	*errorVy = sqrt( abs( sum2y/Vector->numeroDeElementos) - Vymed*Vymed);
}

int verificarSalida( IplImage* FG, int x, int y){

			uchar* ptr1 = (uchar*) ( FG->imageData + y*FG->widthStep );
			if ( ptr1[x] == 255 ) return 0;
			else return 1;
}

void generarRuido( STTrack* Track, int EstadoTrack ){


	//0) No hay nueva medida
	if (EstadoTrack == SLEEPING ) return ;

	crearTrackBarsTracking();
	// GENERAR RUIDO.//
	// 1) Caso normal. Un blob corresponde a un fly
	if (EstadoTrack == CAM_CONTROL ) {
			filterParams->R_x =  	filterParams->Cam->alpha_Rk * 1 ;	// implementar método de cuantificación del ruido.
			filterParams->R_y =  	filterParams->Cam->alpha_Rk * 1 ;
			filterParams->R_Vx = 	filterParams->Cam->alpha_Rk + Track->errorVx;
			filterParams->R_Vy = 	filterParams->Cam->alpha_Rk + Track->errorVy;
			filterParams->R_phiZk = filterParams->Cam->AlphaR_phi * Track->errorVPhi; //errorPhiDif( Track );
			filterParams->R_w	  = 2*filterParams->R_phiZk;
	}
	//1) Varias flies en un mismo blob
	// si la fly no ha salido del área de incertidumbre en un tiempo determinado, regresar fly al centro del blob
	// manteniendo su dirección y estableciendo V y w a 0.
	else if( EstadoTrack == KALMAN_CONTROL ){
		if(  !Track->TimeToPhase1 ){
			filterParams->R_x	  = filterParams->Kal->alpha_Rk * Track->Flysig->Roi.width/2;
			filterParams->R_y	  = filterParams->Kal->alpha_Rk * Track->Flysig->Roi.height/2;
			filterParams->R_Vx 	  = 0;
			filterParams->R_Vy    = 0;
			filterParams->R_phiZk = 0;
			filterParams->R_w 	  = 0;

		}

	}

	// INSERTAR RUIDO en kalman
	// cargamos el error en la medida en el filtro para calcular la ganancia de kalman
	const float R[] = {	filterParams->R_x,0,0,0,0,0,
								0,filterParams->R_y,0,0,0,0,
								0,0,filterParams->R_Vx,0,0,0,
								0,0,0,filterParams->R_Vy,0,0,
								0,0,0,0,filterParams->R_phiZk,0,
								0,0,0,0,0,filterParams->R_w};//{50,50,50,50,180};
//	cvCopy( R, Track->kalman->measurement_noise_cov);
	memcpy( Track->kalman->measurement_noise_cov->data.fl, R, sizeof(R)); // V->N(0,R);

	return ;
}

void EUDistance( float a, float b, float* direccion, float* distancia){

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


float errorPhiDif( float phiXk, float phiZk, float vx_Zk, float vy_Zk ){
	// Error debido a la velocidad. Esta directamente relaccionada con la resolución

	static float phiDif; // error debido a la diferencia entre el nuevo valor y el filtrado


	// Error debido a la diferencia entre el valor corregido y el medido. Si es mayor de PI/2 penalizamos
	phiDif = abs( phiXk - phiZk );

	if( phiDif > 90 ) {

		if ( vx_Zk >= 2 || vy_Zk >= 2) phiDif = phiDif/2; // si la velocidad es elevada, penalizamos poco
												//a pesar de ser mayor de 90
		// el error es la suma del error devido a la velocidad y a la diferencia
		return phiDif/2;
	}
	else {

		if ( vx_Zk <=1 && vy_Zk <= 1) phiDif = phiDif + 90; // si la velocidad es de 1 pixel penalizamos al máximo
		else if ( vx_Zk <=2 && vy_Zk <= 2) phiDif = phiDif + 45; // si es de dos pixels penalizamos con 45º

		return 0; // si la dif es menor a 90 el error es el debido a la velocidad
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
	static int contador = 0;
	// actualizar contadores
	if ( Track->Stats->Estado == SLEEPING){
		Track->Stats->FrameCount ++;
		Track->Stats->EstadoCount ++;
		return;
	}
	Track->Stats->EstadoCount ++;
	Track->Stats->FrameCount ++;

	// actualizamos datos de areas. Se estimará la media con 200 muestras.
	if( Track->Stats->Estado == CAM_CONTROL && Track->Stats->TShape){
		if( contador == 0 ){
			Track->Stats->a = Track->Flysig->a;
			Track->Stats->b = Track->Flysig->b;
		}

		if( Track->Flysig->a < Track->Stats->a) Track->Stats->a--;
		else if( Track->Flysig->a > Track->Stats->a) Track->Stats->a++;
		if( Track->Flysig->b < Track->Stats->b) Track->Stats->b--;
		else if( Track->Flysig->b > Track->Stats->b) Track->Stats->b++;

		if(Track->Stats->TShape > 0) Track->Stats->TShape--;
	}


	// para el caso kalman control, establecemos la distancia en base a la pos anterior y a la corregida (o a la predicha??)
	if( Track->Stats->Estado == KALMAN_CONTROL){

		float Ax = Track->x_k_Pos->data.fl[0] - Track->FlyActual->posicion.x;
		float Ay = Track->x_k_Pos->data.fl[1] - Track->FlyActual->posicion.y;

		//Establecemos la dirección phi y el modulo del vector de desplazamiento
//		EUDistance( Track->x_k_Pos->data.fl[2], Track->x_k_Pos->data.fl[3], NULL, &Track->VInst );
		EUDistance( Ax, Ay, NULL, &Track->VInst );
	}
	Track->Stats->dstTotal = Track->Stats->dstTotal + Track->Vmed;
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

		Track->Flysig->direccion = Track->PhiInst;
		Track->Flysig->dir_med = Track->PhiMed;
		Track->Flysig->Vmed = Track->Vmed;
		Track->Flysig->dir_filtered =Track->x_k_Pos->data.fl[4];
		Track->Flysig->orientacion = corregirTita( Track->Flysig->dir_filtered, Track->Flysig->orientacion );
		Track->Flysig->VInst = Track->VInst;
		Track->Flysig->w = Track->wPhi;
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
		// Etiquetar como 0 para su posterior eliminación
		Track->Flysig->etiqueta = 0;
		Track->Flysig->Color = cvScalar( 255, 255, 255);
		// generar fly
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
		// estimación del tamaño medio del blob del blob
		fly->a = Track->Stats->a;
		fly->b = Track->Stats->b;



		fly->direccion = Track->x_k_Pos->data.fl[4]; // la dirección es la filtrada
		fly->dir_filtered = fly->direccion;
		fly->dir_med = fly->direccion;
		fly->Vmed = Track->Vmed;
		fly->VInst = Track->VInst;
		fly->w = Track->wPhi;
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



		fly->Stats->CountActiva = Track->Stats->TimeBlobOn;
		fly->Stats->CountPasiva = Track->Stats->TimeBlobOff;


		fly->anterior = (STFly*)Track->FlyActual;

		Track->Flysig = fly;

		Track->Flysig->anterior = (STFly*)Track->FlyActual;
		Track->FlyActual->siguiente = (STFly*)Track->Flysig;

		anyadirAlFinal(( STFly*) fly, Flies );
}

void mediaMovilVkalman( STTrack* Track, int Periodo ){

	valorV* V;

	// Para Vx
	irAlFinal(Track->VectorV);
	V = ( valorV*)obtenerActual( Track->VectorV );

	Track->SumatorioVx = Track->SumatorioVx + V->Vx;
	Track->SumatorioVy = Track->SumatorioVy + V->Vy;
	if( Periodo>filterParams->T_Vmed) Periodo--;
	if(Track->VectorV->numeroDeElementos > Periodo) {
	// los que hayan alcanzado el valor máximo restamos al sumatorio el primer valor
		irAlPrincipio( Track->VectorV);

		V = ( valorV*)obtenerActual(  Track->VectorV );

		Track->SumatorioVx =Track->SumatorioVx - V->Vx; // sumatorio para la media
		Track->SumatorioVy =Track->SumatorioVy - V->Vy; // sumatorio para la media
	}
	Track->Vxmed   =  Track->SumatorioVx / Periodo;
	Track->Vymed   =  Track->SumatorioVy / Periodo; // cálculo de la media
	while( Track->VectorV->numeroDeElementos >= filterParams->T_Vmed+1){
		V = (valorV*)liberarPrimero(  Track->VectorV );
		free(V);
	}

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

void mostrarVMediaKalman( tlcde* Vector){
	valorV* valor;
 	irAlFinal(Vector);
 	printf(" \nVector Media movil Vx :\t");
 	for(int i = 0; i <Vector->numeroDeElementos ; i++ ){
 		valor = (valorV*)obtener(i, Vector);
 		printf("%0.f\t", valor->Vx);
 	}
	printf(" \nVector Media movil Vy :\t");
 	for(int i = 0; i <Vector->numeroDeElementos ; i++ ){
 		valor = (valorV*)obtener(i, Vector);
 		printf("%0.f\t", valor->Vy);
 	}

}



int deadTrack( tlcde* Tracks, int id ){

	STTrack* Track = NULL;

	valorSumB* val = NULL;
	valorV* valV = NULL;

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
		if(Track->VectorV->numeroDeElementos > 0){
			valV = (valorV *)borrar(Track->VectorV);
			while (valV)
			{
				free(valV);
				valV = NULL;
				valV = (valorV *)borrar(Track->VectorV);
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

		cvZero( ImKalman);
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
//				cvLine( frameData->ImKalman,
//					cvPoint( Track->z_k->data.fl[0],Track->z_k->data.fl[1]),
//					cvPoint( cvRound( Track->z_k->data.fl[0] + magnitude*cos(Track->z_k->data.fl[4]*CV_PI/180)),
//							 cvRound( Track->z_k->data.fl[1] - magnitude*sin(Track->z_k->data.fl[4]*CV_PI/180))  ),
//					CVX_RED,
//					1, CV_AA, 0 );
//				cvLine( frameData->ImKalman,
//						cvPoint( Track->x_k_Pos->data.fl[0],Track->x_k_Pos->data.fl[1]),
//											cvPoint( cvRound( Track->x_k_Pos->data.fl[0] + magnitude*cos(Track->PhiMed*CV_PI/180)),
//													 cvRound( Track->x_k_Pos->data.fl[1] - magnitude*sin(Track->PhiMed*CV_PI/180))  ),
//													 CVX_WHITE,
//											1, CV_AA, 0 );
//
				cvLine(frameData->ImKalman,
						cvPoint( Track->x_k_Pos->data.fl[0],Track->x_k_Pos->data.fl[1]),
					cvPoint( cvRound( Track->x_k_Pre->data.fl[0] + magnitude*cos(Track->x_k_Pos->data.fl[4]*CV_PI/180)),
							 cvRound( Track->x_k_Pre->data.fl[1] - magnitude*sin(Track->x_k_Pos->data.fl[4]*CV_PI/180))  ),
					CVX_GREEN,
					1, CV_AA, 0 );


				cvCircle(ImKalman,cvPoint(cvRound(Track->z_k->data.fl[0]),cvRound(Track->z_k->data.fl[1] )),10,CVX_BLUE,1,8); // observado
				cvCircle(ImKalman,cvPoint(cvRound(Track->z_k->data.fl[0]),cvRound(Track->z_k->data.fl[1] )),1,CVX_BLUE,-1,8); // observado
//				cvCircle(ImKalman,cvPoint(cvRound(Track->x_k_Pre_->data.fl[0]),cvRound(Track->x_k_Pre_->data.fl[1] )),1,CVX_RED,-1,8); // predicción en t
				cvCircle(ImKalman,cvPoint(cvRound(Track->x_k_Pos->data.fl[0]),cvRound(Track->x_k_Pos->data.fl[1])),10,CVX_WHITE,1,8); // Corrección en t
				cvCircle(ImKalman,cvPoint(cvRound(Track->x_k_Pos->data.fl[0]),cvRound(Track->x_k_Pos->data.fl[1])),1,CVX_WHITE,-1,8); // Corrección en t

				// dirección de kalman (dirección predicha )

				cvCircle(ImKalman,cvPoint(cvRound(Track->x_k_Pre->data.fl[0]),cvRound(Track->x_k_Pre->data.fl[1])),10,CVX_GREEN,1,8); // predicción t+1
				cvCircle(ImKalman,cvPoint(cvRound(Track->x_k_Pre->data.fl[0]),cvRound(Track->x_k_Pre->data.fl[1])),1,CVX_GREEN,-1,8); // predicción t+1

				visualizarId( frameData->ImKalman,cvPoint( Track->z_k->data.fl[0],Track->z_k->data.fl[1]), Track->id, CVX_WHITE);

			}

		}
		if( obtenerVisParam( SHOW_KALMAN ) )
		{   cvZero( frameData->ImKalman );
			cvAdd(ImKalman,frameData->ImKalman,frameData->ImKalman );
		}



}

void showKalmanData( STTrack *Track){

	printf("\n\nReal State: ");
	printf("\n\tCoordenadas: ( %f , %f )",Track->Medida->data.fl[0],Track->Medida->data.fl[1]);
	printf("\n\tVelocidad: ( %f , %f )",Track->Medida->data.fl[2],Track->Medida->data.fl[3]);
	printf(" Vector Media movil Vx e Vy:\n");
	mostrarVMediaKalman(Track->VectorV);
	printf("\n\tVMedia: ( %f , %f )",Track->Vxmed,Track->Vymed);
	printf("\n\t|VMedia|:  %f ",Track->Vmed );
	printf("\n\tDirección:  %f ",Track->Medida->data.fl[4]);
	printf("\n\tDirMed:  %f ",Track->PhiMed);
	printf("\n\tw: %f ", Track->wPhi);

	printf("\n\n Observed State: "); // sumandole el error de medida que es de media 0 en este caso
	verVector( Track->z_k );

	printf("\n Media Measurement noise:\n\n\t%0.1f\n\t%0.1f\n\t%0.1f\n\t%0.1f\n\t%0.1f\n\t%0.1f",
			Track->Measurement_noise->data.fl[0],
			Track->Measurement_noise->data.fl[1],
			Track->Measurement_noise->data.fl[2],
			Track->Measurement_noise->data.fl[3],
			Track->Measurement_noise->data.fl[4],
			Track->Measurement_noise->data.fl[5]);


	printf("\n Error Measurement noise:");
	verMatriz( Track->Measurement_noise_cov );

	printf("\n Error Process noise:");
	verMatriz( Track->kalman->process_noise_cov);

	printf("\n\n Predicted State t:");
	verVector( Track->x_k_Pre_);

	printf("\n Error Cov Predicted:");
	verMatriz( Track->P_k_Pre_ );

	printf("\n\n Corrected State:" );
	verVector( Track->x_k_Pos);

	printf("\n Error Cov Corrected:");
	verMatriz( Track->P_k_Pos);

	printf("\n\n Predicted State t+1:");
			printf("\n\tCoordenadas: ( %f , %f )",Track->x_k_Pre->data.fl[0],Track->x_k_Pre->data.fl[1]);
			printf("\n\tVelocidad: ( %f , %f )",Track->x_k_Pre->data.fl[2],Track->x_k_Pre->data.fl[3]);
			printf("\n\tDirección:  %f ",Track->x_k_Pre->data.fl[4]);
			printf("\n\tV_Angular:  %f ",Track->x_k_Pre->data.fl[5]);




	printf("\n Error Cov Predicted:");
	printf("\n\n\t%0.3f\t0\t0\t0\t0\t0\n\t0\t%0.3f\t0\t0\t0\t0\n\t0\t0\t%0.3f\t0\t0\t0\n\t0\t0\t0\t%0.3f\t0\t0\n\t0\t0\t0\t0\t%0.3f\t0\n\t0\t0\t0\t0\t0\t%0.3f",
			Track->P_k_Pre->data.fl[0],
			Track->P_k_Pre->data.fl[7],
			Track->P_k_Pre->data.fl[14],
			Track->P_k_Pre->data.fl[21],
			Track->P_k_Pre->data.fl[28],
			Track->P_k_Pre->data.fl[35]);

	printf("\n Ganancia:");
	verMatriz(Track->kalman->gain);

	printf("\n\n");
}

void verMatriz( CvMat* Mat){

	printf("\n\n\t%0.3f\t0\t0\t0\t0\t0\n\t0\t%0.3f\t0\t0\t0\t0\n\t0\t0\t%0.3f\t0\t0\t0\n\t0\t0\t0\t%0.3f\t0\t0\n\t0\t0\t0\t0\t%0.3f\t0\n\t0\t0\t0\t0\t0\t%0.3f",
			Mat->data.fl[0],
			Mat->data.fl[7],
			Mat->data.fl[14],
			Mat->data.fl[21],
			Mat->data.fl[28],
			Mat->data.fl[35]);


}

void verVector( CvMat* Vec ){

	printf("\n\tCoordenadas: ( %f , %f )",Vec->data.fl[0],Vec->data.fl[1]);
	printf("\n\tVelocidad: ( %f , %f )",Vec->data.fl[2],Vec->data.fl[3]);
	printf("\n\tDirección:  %f ",Vec->data.fl[4]);
	printf("\n\tV_Angular:  %f ",Vec->data.fl[5]);

}
void liberarTracks( tlcde* lista){
	// Borrar todos los elementos de la lista
	// Comprobar si hay elementos
	int tracks;
	if(lista == NULL || lista->numeroDeElementos<1)  return;
	tracks = deadTrack( lista, 0);
	while( tracks){
		tracks = deadTrack( lista, 0);
	}

}

int dejarId( STTrack* Track, tlcde* identities ){
	Identity *Id = NULL;
	Identity *Id2 = NULL;
	Id = ( Identity* )malloc( sizeof(Identity ));
	if( !Id){error(4);return 0 ;}
	Id->etiqueta = Track->id;
	Id->color = Track->Color;
	irAlFinal( identities );
	// insertamos en orden
	Id2 = (Identity*)obtenerActual( identities );
	while( Id2->etiqueta< Id->etiqueta){
		irAlAnterior( identities );
		Id2 = (Identity*)obtenerActual( identities );
	}
	insertar( Id, identities );
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

		sprintf(settingName, "T_Vmed");
		if (!config_setting_lookup_float(setting, settingName,
				&val)) {
			if( trackParams->mmTOpixel) {
				if( trackParams->FPS == 15)	filterParams->T_Vmed = cvRound(0.33*trackParams->FPS) ; // de seg a frames
				else if( trackParams->FPS == 30 ) 	filterParams->T_Vmed = cvRound(0.17*trackParams->FPS) ;
				else filterParams->T_Vmed = 5 ;
			}
			else filterParams->T_Vmed = 5; // en frames

			fprintf(
					stderr,
					"No se encuentra la variable %s en el archivo de configuración o el tipo de dato es incorrecto.\n "
						"Establecer por defecto a %d frames \n",
					settingName, filterParams->T_Vmed );

		} else if( val <= 0 ){

			if( trackParams->mmTOpixel) {
				if( trackParams->FPS == 15)	filterParams->T_Vmed = cvRound(0.33*trackParams->FPS) ; // de seg a frames
				else if( trackParams->FPS == 30 ) 	filterParams->T_Vmed = cvRound(0.17*trackParams->FPS) ;
				else filterParams->T_Vmed = 5 ;
			}
			else filterParams->T_Vmed = 5; // en frames
			fprintf(stderr,
					"El valor de %s está fuera de límites\n "
						"Establecer por defecto %s a %d frames \n",
					settingName, settingName,
					filterParams->T_Vmed );
		}
		else{
			filterParams->T_Vmed = cvRound( (float)val * trackParams->FPS); // a frames
		}

		sprintf(settingName, "MaxBack");
		if (!config_setting_lookup_int(setting, settingName,
				&filterParams->MaxBack)) {

			if( trackParams->FPS == 15)	filterParams->MaxBack = 5 ; // de seg a frames
			else if( trackParams->FPS == 30 ) 	filterParams->MaxBack = 10 ;
			else filterParams->MaxBack = 5 ;
			fprintf(
					stderr,
					"No se encuentra la variable %s en el archivo de configuración o el tipo de dato es incorrecto.\n "
						"Establecer por defecto a %d frames \n",
					settingName, filterParams->MaxBack );

		}
		else if( filterParams->MaxBack <= 0 ){

			if( trackParams->FPS == 15)	filterParams->MaxBack = 5 ; // de seg a frames
			else if( trackParams->FPS == 30 ) 	filterParams->MaxBack = 10 ;
			else filterParams->MaxBack = 5 ;
			fprintf(stderr,
					"El valor de %s está fuera de límites\n "
						"Establecer por defecto %s a %d frames \n",
					settingName, settingName,
					filterParams->MaxBack );
		}

		sprintf(settingName, "MaxVPhiError");
		if (!config_setting_lookup_int(setting, settingName,
				&filterParams->MaxVPhiError)) {

			filterParams->MaxVPhiError = 10 ;
			fprintf(
					stderr,
					"No se encuentra la variable %s en el archivo de configuración o el tipo de dato es incorrecto.\n "
						"Establecer por defecto a %d grados \n",
					settingName, filterParams->MaxVPhiError );

		}
		else if( filterParams->MaxVPhiError <= 0 ){

			filterParams->MaxVPhiError = 10 ;
			fprintf(stderr,
					"El valor de %s está fuera de límites\n "
						"Establecer por defecto %s a %d grados \n",
					settingName, settingName,
					filterParams->MaxVPhiError );
		}

		sprintf(settingName, "Q");
		if (!config_setting_lookup_float(setting, settingName,
				&val)) {

			filterParams->Q = 2.0 ;
			fprintf(
					stderr,
					"No se encuentra la variable %s en el archivo de configuración o el tipo de dato es incorrecto.\n "
						"Establecer por defecto a %0.1f  \n",
					settingName, filterParams->Q );

		}
		else if( val <= 0 ){

			filterParams->Q = 2.0 ;
			fprintf(stderr,
					"El valor de %s está fuera de límites\n "
						"Establecer por defecto %s a %0.1f  \n",
					settingName, settingName,
					filterParams->Q );
		}
		else filterParams->Q = val;

		sprintf( settingFather,"Kalman.CamControl" );
		setting = config_lookup(&cfg, settingFather);

		sprintf(settingName, "Q");
		if (!config_setting_lookup_float(setting, settingName,
				&val)) {

			filterParams->Cam->Q = 2.0 ;
			fprintf(
					stderr,
					"No se encuentra la variable %s en el archivo de configuración o el tipo de dato es incorrecto.\n "
						"Establecer por defecto a %0.1f  \n",
					settingName, filterParams->Cam->Q );

		}
		else if( val <= 0 ){

			filterParams->Cam->Q = 2.0 ;
			fprintf(stderr,
					"El valor de %s está fuera de límites\n "
						"Establecer por defecto %s a %0.1f  \n",
					settingName, settingName,
					filterParams->Cam->Q );
		}
		else filterParams->Cam->Q = val;

		sprintf(settingName, "alpha_Rk");
		if (!config_setting_lookup_float(setting, settingName,
				&val)) {
			filterParams->Cam->alpha_Rk = 1;
			fprintf(
					stderr,
					"No se encuentra la variable %s en el archivo de configuración o el tipo de dato es incorrecto.\n "
						"Establecer por defecto a %0.1f  \n",
					settingName, filterParams->Cam->alpha_Rk );

		}
		else if( val <= 0 ){
			filterParams->Cam->alpha_Rk = 1 ;
			fprintf(stderr,
					"El valor de %s está fuera de límites\n "
						"Establecer por defecto %s a %0.1f  \n",
					settingName, settingName,
					filterParams->Cam->alpha_Rk );
		}
		else filterParams->Cam->alpha_Rk = val;

		sprintf(settingName, "AlphaR_phi");
		if (!config_setting_lookup_float(setting, settingName,
				&val)) {

			filterParams->Cam->AlphaR_phi= 1;
			fprintf(
					stderr,
					"No se encuentra la variable %s en el archivo de configuración o el tipo de dato es incorrecto.\n "
						"Establecer por defecto a %0.1f  \n",
					settingName, filterParams->Cam->AlphaR_phi );

		}
		else if( filterParams->Cam->AlphaR_phi <= 0 ){

			filterParams->Cam->AlphaR_phi = 1 ;
			fprintf(stderr,
					"El valor de %s está fuera de límites\n "
						"Establecer por defecto %s a %0.1f  \n",
					settingName, settingName,
					filterParams->Cam->AlphaR_phi );
		}
		else filterParams->Cam->AlphaR_phi = val;

		sprintf( settingFather,"Kalman.KalmanControl" );
		setting = config_lookup(&cfg, settingFather);

		sprintf(settingName, "MaxTimeToPhase1");
		if (!config_setting_lookup_float(setting, settingName,
				&val)) {
			if( trackParams->mmTOpixel) {
				if( trackParams->FPS == 15)	filterParams->T_Vmed = cvRound(0.2*trackParams->FPS) ; // de seg a frames
				else if( trackParams->FPS == 30 ) 	filterParams->T_Vmed = cvRound(0.4*trackParams->FPS) ;
				else filterParams->T_Vmed = 5 ;
			}
			else{
				if( trackParams->FPS == 15)	filterParams->Kal->MaxTimeToPhase1 = 5 ; // de seg a frames
				else if( trackParams->FPS == 30 ) 	filterParams->Kal->MaxTimeToPhase1 = 10 ;
				else filterParams->Kal->MaxTimeToPhase1 = 5 ;
			}

			fprintf(
					stderr,
					"No se encuentra la variable %s en el archivo de configuración o el tipo de dato es incorrecto.\n "
						"Establecer por defecto a %d frames \n",
					settingName, filterParams->Kal->MaxTimeToPhase1 );

		} else if( val <= 0 ){

			if( trackParams->mmTOpixel) {
				if( trackParams->FPS == 15)	filterParams->T_Vmed = cvRound(0.2*trackParams->FPS) ; // de seg a frames
				else if( trackParams->FPS == 30 ) 	filterParams->T_Vmed = cvRound(0.4*trackParams->FPS) ;
				else filterParams->T_Vmed = 5 ;
			}
			else{
				if( trackParams->FPS == 15)	filterParams->Kal->MaxTimeToPhase1 = 5 ; // de seg a frames
				else if( trackParams->FPS == 30 ) 	filterParams->Kal->MaxTimeToPhase1 = 10 ;
				else filterParams->Kal->MaxTimeToPhase1 = 5 ;
			}

			fprintf(stderr,
					"El valor de %s está fuera de límites\n "
						"Establecer por defecto %s a %d frames \n",
					settingName, settingName,
					filterParams->Kal->MaxTimeToPhase1 );
		}
		else{
			filterParams->Kal->MaxTimeToPhase1 = cvRound( (float)val * trackParams->FPS); // a frames
		}

		sprintf(settingName, "Q");
		if (!config_setting_lookup_float(setting, settingName,
				&val)) {

			filterParams->Kal->Q = 2.0 ;
			fprintf(
					stderr,
					"No se encuentra la variable %s en el archivo de configuración o el tipo de dato es incorrecto.\n "
						"Establecer por defecto a %0.1f  \n",
					settingName, filterParams->Kal->Q );

		}
		else if( val <= 0 ){

			filterParams->Kal->Q = 2.0 ;
			fprintf(stderr,
					"El valor de %s está fuera de límites\n "
						"Establecer por defecto %s a %0.1f  \n",
					settingName, settingName,
					filterParams->Kal->Q );
		}
		else filterParams->Kal->Q = val;

		sprintf(settingName, "alpha_Rk");
		if (!config_setting_lookup_float(setting, settingName,
				&val)) {

			filterParams->Kal->alpha_Rk= 1;
			fprintf(
					stderr,
					"No se encuentra la variable %s en el archivo de configuración o el tipo de dato es incorrecto.\n "
						"Establecer por defecto a %0.1f  \n",
					settingName, filterParams->Kal->alpha_Rk );

		}
		else if( val <= 0 ){

			filterParams->Kal->alpha_Rk = 1 ;
			fprintf(stderr,
					"El valor de %s está fuera de límites\n "
						"Establecer por defecto %s a %0.1f  \n",
					settingName, settingName,
					filterParams->Kal->alpha_Rk );
		}
		else filterParams->Kal->alpha_Rk= val;

		sprintf(settingName, "R_phiZk");
		if (!config_setting_lookup_float(setting, settingName,
				&val)) {

			filterParams->Kal->R_phiZk= 1000;
			fprintf(
					stderr,
					"No se encuentra la variable %s en el archivo de configuración o el tipo de dato es incorrecto.\n "
						"Establecer por defecto a %0.1f  \n",
					settingName, filterParams->Kal->alpha_Rk );

		}
		else if( val <= 0 ){

			filterParams->Kal->R_phiZk = 1000 ;
			fprintf(stderr,
					"El valor de %s está fuera de límites\n "
						"Establecer por defecto %s a %0.1f  \n",
					settingName, settingName,
					filterParams->Kal->R_phiZk );
		}
		else filterParams->Kal->R_phiZk = val;
	}

	SetPrivateKFilterParams(  );

	ShowKalmanFilterParams( settingFather );
	config_destroy(&cfg);
}

void SetDefaultKFilterParams( TrackingParams* trackParams  ){

	// si hay medida de calibración se establece la vel en mm/s y se pasa a pixels frame
	if( trackParams->mmTOpixel) filterParams->Velocidad = 10.6*trackParams->mmsTOpf;
	else filterParams->Velocidad = 5.6;// si no se establece en pixels/frame

	filterParams->V_Angular = 0;

	if( trackParams->mmTOpixel) filterParams->MaxJump = cvRound(20*trackParams->mmTOpixel) ; // en mm
	else filterParams->MaxJump = 50; // en pixels

	if( trackParams->mmTOpixel) filterParams->T_Vmed = cvRound(0.25*trackParams->FPS) ; // de seg a frames
	else filterParams->T_Vmed = 5; // en frames


	if( trackParams->mmTOpixel) {
		if( trackParams->FPS == 15)	filterParams->T_Vmed = cvRound(0.2*trackParams->FPS) ; // de seg a frames
		else if( trackParams->FPS == 30 ) 	filterParams->T_Vmed = cvRound(0.4*trackParams->FPS) ;
		else filterParams->T_Vmed = 5 ;
	}
	else{
		if( trackParams->FPS == 15)	filterParams->Kal->MaxTimeToPhase1 = 5 ; // de seg a frames
		else if( trackParams->FPS == 30 ) 	filterParams->Kal->MaxTimeToPhase1 = 10 ;
		else filterParams->Kal->MaxTimeToPhase1 = 5 ;
	}

	filterParams->MaxBack = 5;
	filterParams->MaxVPhiError = 10;
	filterParams->Q = 2;

	filterParams->Cam->Q = 2.0;
	filterParams->Cam->alpha_Rk = 1;
	filterParams->Cam->AlphaR_phi = 1;


	filterParams->Kal->Q = 2.0;
	filterParams->Kal->alpha_Rk = 1;
	filterParams->Kal->R_phiZk = 1000;

}

void ShowKalmanFilterParams( char* Campo ){

	printf(" \nVariables para el campo %s: \n", Campo);
	printf(" -Velocidad \t= %0.2f pixels/frame \n", filterParams->Velocidad);
	printf(" -V_Angular \t= %0.2f º/frame \n", filterParams->V_Angular);
	printf(" -T_Vmed \t= %d frames\n", filterParams->T_Vmed);
	printf(" -MaxJump \t= %0.1f pixels\n", filterParams->MaxJump);
	printf(" -MaxBack \t= %d frames \n", filterParams->MaxBack);
	printf(" -MaxVPhiError \t= %d grados \n", filterParams->MaxVPhiError);
	printf(" -Q Inicialización \t= %0.3f  \n", filterParams->Q);
	printf(" -Q CamControl \t= %0.1f \n", filterParams->Cam->Q);
	printf(" -alpha_Rk CamControl \t= %0.1f frames \n", filterParams->Cam->alpha_Rk);
	printf(" -AlphaR_phi CamControl \t= %0.1f frames \n", filterParams->Cam->AlphaR_phi);
	printf(" -Q KalControl \t= %0.1f \n", filterParams->Kal->Q);
	printf(" -MaxTimeToPhase1 \t= %d frames\n", filterParams->Kal->MaxTimeToPhase1);

	printf(" -alpha_Rk KalControl \t= %0.1f frames \n", filterParams->Kal->alpha_Rk);
	printf(" -AlphaR_phi KalControl \t= %0.1f frames \n", filterParams->Kal->R_phiZk);

}

void SetPrivateKFilterParams(  ){
	//void onTrackbarSlide(pos, BGModelParams* Param) {
	//   Param->ALPHA = pos / 100;
	//}
	filterParams->w_max = 3;

	filterParams->Cam->g_slider_Q = filterParams->Cam->Q*10;

	filterParams->Cam->g_slider_alpha_Rk = filterParams->Cam->alpha_Rk*10;

	filterParams->Cam->g_slider_AlphaR_phi = filterParams->Cam->AlphaR_phi*10;


	filterParams->Kal->g_slider_Q = filterParams->Kal->Q*10;

	filterParams->Kal->g_slider_alpha_Rk = filterParams->Kal->alpha_Rk*10;

	filterParams->Kal->g_slider_R_phiZk = filterParams->Kal->R_phiZk;

	if(obtenerVisParam( MODE )) filterParams->createTrackbars = true;



}

void crearTrackBarsTracking(){

	if( filterParams->createTrackbars ){

		cvCreateTrackbar( "MaxBack","Tracking Params", &filterParams->MaxBack, 20, onTrackbarMaxBack);
		cvCreateTrackbar( "TVmed","Tracking Params", &filterParams->T_Vmed, 60, onTrackbarTVmed);
		cvCreateTrackbar( "MaxVPhiError","Tracking Params", &filterParams->MaxVPhiError, 60, onTrackbarMaxVPhiError);
		// en el intervalo [ 1 y 10 ) => [0.1, 1) => disminuye ruido
		// en el intervalo ( 10, 100] => (1 , 10 ) => aumenta ruido hasta un factor de 10

		cvCreateTrackbar( "AlphaRk = trackVal/10","Tracking Params", &filterParams->Cam->g_slider_alpha_Rk, 100, onTrackbarAlphaRkCam);

		cvCreateTrackbar( "AlphaR_Phi = trackVal/10","Tracking Params", &filterParams->Cam->g_slider_AlphaR_phi, 100, onTrackbarAlphaRPhiCam);

		// en el intervalo [ 1 y 10 ) => [0.1, 1) => disminuye ruido
		// en el intervalo ( 10, 1000] => (1 , 100 ) => aumenta ruido hasta un factor de 100
		cvCreateTrackbar( "AlphaRk_Kal = trackVal/10","Tracking Params", &filterParams->Kal->g_slider_alpha_Rk, 1000, onTrackbarAlphaRkKal);

		cvCreateTrackbar( "Q_CamControl","Tracking Params", &filterParams->Cam->g_slider_Q, 100, onTrackbarQCam);
		// en el intervalo [ 1 y 10 ) => [100, 1000) => disminuye ruido
		// en el intervalo ( 10, 1000] => (1000 , 10000 ) => aumenta ruido hasta un factor de 100
		cvCreateTrackbar( "R_PhiKal","Tracking Params", &filterParams->Kal->g_slider_R_phiZk, 10000, onTrackbarRPhiKal);

		cvCreateTrackbar( "Q_kalmanControl","Tracking Params", &filterParams->Kal->g_slider_Q, 100, onTrackbarQKal);

		cvCreateTrackbar( "MaxTimeToPhase1","Tracking Params", &filterParams->Kal->MaxTimeToPhase1, 100);



	}

}

int obtenerFilterParam( int param ){

	if (param == MAX_JUMP) return filterParams->MaxJump;
	else return 0;
}

void onTrackbarTVmed(int val ){

	if(val == 0) val =1;

}
void onTrackbarMaxBack(int val ){

	if(val == 0) val =1;

}

void onTrackbarMaxVPhiError(int val ){

	if(val == 0) val =1;

}

void onTrackbarQCam(int val ){

	if(val == 0) val =1;
	filterParams->Cam->Q = (float)val/10;

}
void onTrackbarAlphaRkCam(int val ){

	if(val == 0) val =1;
	filterParams->Cam->alpha_Rk = (float)val/10;

}
void onTrackbarAlphaRPhiCam(int val ){

	if(val == 0) val =1;
	filterParams->Cam->AlphaR_phi = (float)val/10;

}

void onTrackbarQKal(int val ){

	if(val == 0) val =1;
	filterParams->Kal->Q = (float)val/10;

}

void onTrackbarAlphaRkKal(int val ){
	if(val == 0) val =1;
	filterParams->Kal->alpha_Rk = (float)val/10;
}

void onTrackbarRPhiKal(int val ){
	if(val == 0) val =1;
	filterParams->Kal->alpha_Rk = (float)val*100;
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
