/*
 * Kalman.cpp
 *
 *  Created on: 18/11/2011
 *      Authores: Rubén Chao Chao, Germán Garcia Vázques
 *
 */


#include "Kalman.hpp"



//!< En la literatura en general primero se hace la predicción para frame t ( posible estado del blob en el frame t) en base
//!< al modelo y luego se incorpora la medida de t con la cual se hace la corrección. En nuestro caso el proceso es:
//!< Se incorpora la medida del frame t. Se hace la corrección y se predice para el frame t+1, de modo que en cada track y para cada
//!< frame disponemos por un lado de la predicción para el frame t hecha en el t-1, con la cual se hace la corrección, y por otro lado de
//!< la predicción para el frame t+1.
//!< 	Suposiciones:
//!<	Tras aplicar asignaridentidades:
//!<	- Tenemos en cada track un puntero al blob del frame t+1 con mayor probabilidad de asignación.
//!<	- Asimismo en cada fly disponemos una lista con el o los tracks candidatos a ser asignados a dicha fly.

CvMat* CoordReal;
CvMat* H ;

// componentes del vector Zk con las nuevas medidas
float x_Zk;
float y_Zk;
float vx_Zk;
float vy_Zk;
float phiZk;

// componentes del vector Rk de covarianzas. Incertidumbre en la medida: V->N( 0, Rk )
float R_x;
float R_y;
float R_Vx;
float R_Vy;
float R_phiZk;

// componentes del vector Xk. Predicciones
float x_Xk ;
float y_Xk ;
float vx_Xk ;
float vy_Xk ;
float phiXk ;

// variables para establecer la nueva medida
float Vx ;
float Vy ;
int Ax; //incremento de x
int Ay; //incremento de y
float distancia; // sqrt( Ax² + Ay² )

//Imagenes
IplImage* ImKalman = NULL;

void Kalman(STFrame* frameData, tlcde* lsIds,tlcde* lsTracks) {

	tlcde* Flies;
	STTrack* Track;
	STFly* Fly = NULL;

	// cargar datos del frame
	Flies=frameData->Flies;

	// Inicializar imagenes y parámetros
	if( !ImKalman ){
		ImKalman = cvCreateImage(cvGetSize(frameData->ImKalman),8,3);
		cvZero(ImKalman);
		CoordReal = cvCreateMat(2,1,CV_32FC1);
	}
	cvZero(frameData->ImKalman);

	////////////////////// CORRECCION ////////////////////////
	for(int i = 0;i < lsTracks->numeroDeElementos ; i++){
		// obtener Track
		Track = (STTrack*)obtener(i, lsTracks);
		// Esablece estado del track según haya no nueva/s medida/s
		Track->Estado = establecerEstado( Track, Track->Flysig );
		// Generar la nueva medida en función del estado
		generarMedida( Track, Track->Estado );
		// Generar ruido asociado a la nueva medida
		generarRuido( Track, Track->Estado );
		// Actualizar datos con la nueva medida
		updateTrack( Track, Track->Estado );
		// corregir kalman
		if( Track->Estado != SLEEPING ){
			cvKalmanCorrect( Track->kalman, Track->z_k);
			// actualizamos parámetros
			Track->FlyActual->dir_filtered =Track->x_k_Pos->data.fl[4] ;
		}
	}

	// ANYADIR NUEVOS TRACKS, si es necesario.
	// si hay mosca/s sin asignar a un track/s (id = -1), se crea un nuevo track para esa mosca
	for(int i = 0; i< Flies->numeroDeElementos ;i++ ){
		Fly = (STFly*)obtener(i, Flies );
		if( Fly->etiqueta == -1 && Fly->Tracks->numeroDeElementos == 0){
			Track = initTrack( Fly, lsIds , 1 );
			anyadirAlFinal( Track , lsTracks );
		}
	}

	////////////////////// PREDICCION ////////////////////////
	//Recorremos cada track y hacemos la predicción para el siguiente frame
	for(int i = 0;i < lsTracks->numeroDeElementos ; i++){
		Track = (STTrack*)obtener(i, lsTracks);
		if(Track->Estado != SLEEPING ){
			if( Track->x_k_Pre != NULL ){
				// guardamos los datos de la predicción anterior
				cvCopy(Track->x_k_Pre,Track->x_k_Pre_);
				cvCopy(Track->P_k_Pre,Track->P_k_Pre_);
			}
			// nueva predicción
			Track->x_k_Pre = cvKalmanPredict( Track->kalman, 0);
		}
	}

	///////////////////// VISUALIZAR RESULTADOS ////////////////
	visualizarKalman( frameData, lsTracks );

	return;
}// Fin de Kalman


// etiquetar fly e iniciar nuevo track

STTrack* initTrack( STFly* Fly ,tlcde* ids, float fps ){

	STTrack* Track = NULL;

	Track = (STTrack*)malloc(sizeof(STTrack));
	if(!Track) {error(4); exit(1);}

	asignarNuevaId( Fly , ids );

	Track->FlyActual = Fly;
	Track->Flysig = NULL;
	Track->id = Fly->etiqueta;
	Track->Color = Fly->Color;

	Track->Estado = CAM_CONTROL;
	Track->FrameCount = 1;
	Track->EstadoBlob = 0;

	// iniciar kalman
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

	cvZero(Track->Medida);
	cvZero(Track->z_k );

	return Track;
	//		direccionR = (flyData->direccion*CV_PI)/180;
	// Calcular e iniciar estado

}

CvKalman* initKalman( STFly* Fly, float dt ){

	float Vx=0;// componente X de la velocidad.
	float Vy=0;// componente Y de la velociad.

	// Crear el flitro de Kalman

	CvKalman* Kalman = cvCreateKalman(5,5,0);

	// Inicializar las matrices parámetros para el filtro de Kalman
	// en la primera iteración iniciamos la dirección con la orientación

	Vx= VELOCIDAD*cos(Fly->orientacion*CV_PI/180);
	Vy=-VELOCIDAD*sin(Fly->orientacion*CV_PI/180);

	const float F[] = {1,0,dt,0,0, 0,1,0,dt,0, 0,0,1,0,0, 0,0,0,1,0, 0,0,0,0,1}; // Matriz de transición F
	// Matriz R inicial. Errores en medidas. ( Rx, Ry,RVx, RVy, Rphi ). Al inicio el error en el angulo es de +-180º
	const float R_inicial[] = {50,0,0,0,0, 0,50,0,0,0, 0,0,50,0,0, 0,0,0,50,0, 0,0,0,0,360};//{50,50,50,50,180};
	const float X_k[] = { Fly->posicion.x, Fly->posicion.y, Vx, Vy, Fly->orientacion };// Matiz de estado inicial
	float initialState[] = {Fly->posicion.x, Fly->posicion.y, Vx, Vy, Fly->orientacion};
//
	CvMat x_k =cvMat(1, 5, CV_32FC1, initialState);

	// establecer parámetros
	memcpy( Kalman->transition_matrix->data.fl, F, sizeof(F)); // F

	cvSetIdentity( Kalman->measurement_matrix,cvRealScalar(1) ); // H Matriz de medida
	cvSetIdentity( Kalman->process_noise_cov,cvRealScalar(1 ) ); // error asociado al modelo del sistema Q = f(F,B y H).
	cvSetIdentity( Kalman->error_cov_pre, cvScalar(1) ); // Incertidumbre en predicción. (P_k' = F P_k-1 Ft) + Q
	cvSetIdentity( Kalman->error_cov_post,cvRealScalar(1)); // Incertidumbre tras añadir medida. P_k = ( I - K_k H) P_k'

	memcpy( Kalman->measurement_noise_cov->data.fl, R_inicial, sizeof(R_inicial)); // Incertidumbre en la medida Vk; Vk->N(0,R)
//	cvSetIdentity( Kalman->error_cov_post,cvRealScalar(1)); // Incertidumbre tras añadir medida. P_k = ( I - K_k H) P_k'
	//copyMat(&x_k, Kalman->state_post);
	memcpy( Kalman->state_post->data.fl, X_k, sizeof(X_k)); // Estado inicial

	return Kalman;
}


int establecerEstado( STTrack* Track, STFly* flySig ){

	//Establecer el estado del Track sugún haya o no medida/s.
	if ( !flySig ){
		//dormir track. Si mas adelante hay que generar un nuevo track para una fly sin asignar, usar los que estan en suspensión;
		return SLEEPING;
	}
	else if( flySig->Tracks->numeroDeElementos == 1 ){
		return CAM_CONTROL;
	}

	else if(flySig->Tracks->numeroDeElementos > 1){
		return KALMAN_CONTROL;
	}
	else return -1;

}


void generarMedida( STTrack* Track, int EstadoTrack ){

	STFly* flyActual = Track->FlyActual;
	STFly* flySig = Track->Flysig;

	//Caso 0: No se ha encontrado ninguna posible asignación. No hay medida.
	if ( EstadoTrack == SLEEPING )	return ;

	x_Zk = flySig->posicion.x;
	y_Zk = flySig->posicion.y;


	Ax = x_Zk - Track->z_k->data.fl[0];
	Ay = y_Zk - Track->z_k->data.fl[1];



	vx_Zk = Ax / 1;
	vy_Zk = Ay / 1;

	//Establecemos la dirección phi y el modulo del vector de desplazamiento
	EUDistance( vx_Zk, vy_Zk, &phiZk, &distancia );
	//Caso 1: Se ha encontrado la siguiente posición. la medida se genera a partir de los datos obtenidos de los sensores
	if( EstadoTrack == CAM_CONTROL ) {
		// si no hay movimiento la dirección es la orientación.( o la dirección anterior? o la filtrada??? )
		if( distancia == 0 ) phiZk = flyActual->dir_filtered;
		else{
			// CORREGIR DIRECCIÓN.
			//Sumar ó restar n vueltas para que la dif sea siempre <=180
			phiXk = Track->x_k_Pre->data.fl[4]; // valor filtrado de la direccion

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

	float V[] = {0,0,0,0,0}; // media del ruido
	memcpy( Track->Measurement_noise->data.fl, V, sizeof(V));

	cvGEMM(H, Track->Medida,1, Track->Measurement_noise, 1, Track->z_k,0 ); // Zk = H Medida + V

	return ;
}

void generarRuido( STTrack* Track, int EstadoTrack ){

	STFly* flySig = Track->Flysig;

	//0) No hay nueva medida
	if (EstadoTrack == SLEEPING ) return ;


	// GENERAR RUIDO.
	// 1) Caso normal. Un blob corresponde a un fly
	if (EstadoTrack == CAM_CONTROL ) {
		//1) Posición
		R_x = 1;
		R_y = R_x;


		//2) Velocidad
		R_Vx = 2*R_x;
		R_Vy = 2*R_y;

		// 3) Dirección
		R_phiZk = generarR_PhiZk( );

	}
	//1) Varias flies en un mismo blob
	else if( EstadoTrack == KALMAN_CONTROL ){


		R_x = Track->Flysig->Roi.width/2;
		R_y = Track->Flysig->Roi.height/2;
		R_Vx = 2*R_x;
		R_Vy = 2*R_y;
		R_phiZk = 1000;
	}
	// INSERTAR RUIDO en kalman
	// cargamos el error en la medida en el filtro para calcular la ganancia de kalman
	float R[] =  { R_x,0,0,0,0, 0,R_y,0,0,0, 0,0,R_Vx,0,0, 0,0,0,R_Vy,0, 0,0,0,0,R_phiZk};//covarianza del ruido
	memcpy( Track->kalman->measurement_noise_cov->data.fl, R, sizeof(R)); // V->N(0,R);

	 return ;
}

void EUDistance( int a, int b, float* direccion, float* distancia){

	if( a == 0){
		if( b == 0 ) *direccion = -1; // No hay movimiento.
		if( b > 0 ) *direccion = 270; // Hacia abajo.
		if( b < 0 ) *direccion = 90; // Hacia arriba.
	}
	else if ( b == 0) {
		if (a < 0) *direccion = 180; // hacia la izquierda.
		else *direccion = 0;			// hacia la derecha.
	}
	else	*direccion = atan2( (float)-b , (float)a )* 180 / CV_PI;
	// calcular modulo del desplazamiento.
	*distancia = sqrt( pow( a ,2 )  + pow( b ,2 ) ); // sqrt( a² + b² )

}


float generarR_PhiZk( ){
	// Error debido a la velocidad. Esta directamente relaccionada con la resolución

	float phiDif; // error debido a la diferencia entre el nuevo valor y el filtrado
	float errorV; // invertidumbre en la dirección debido a la velocidad
	float errIz; // error debido a la velocidad por la izqd del ángulo
	float errDr; // error debido a la velocidad por la drcha del ángulo


	errIz = atan2( abs(vy_Zk), abs( vx_Zk) - 0.5 )  - atan2( abs(vy_Zk) , abs( vx_Zk) );
	errDr = atan2( abs(vy_Zk) , abs( vx_Zk) ) - atan2((abs(vy_Zk) - 0.5) , abs( vx_Zk) );

	if (errIz >= errDr ) errorV = errIz*180/CV_PI;
	else errorV = errDr*180/CV_PI;

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

void updateTrack( STTrack* Track, int EstadoTrack ){

	STFly* flyActual;

	// ACTUALIZAR TRACK
	if ( EstadoTrack == SLEEPING){
		Track->FlyActual = NULL;
		Track->Flysig = NULL;
		return;
	}
	Track->FlyActual = Track->Flysig;
	Track->Flysig = NULL;
	Track->dstTotal = Track->dstTotal + distancia;

	Track->FrameCount ++;
	if( distancia >0 ) Track->EstadoBlob = IN_FG;
	else  Track->EstadoBlob = IN_BG;

	//	updateCounts( Track );
	//	SetStateBlob( Track );
	flyActual = Track->FlyActual;
	// ETIQUETAR Y ACTUALIZAR FLY ACTUAL
	if( EstadoTrack == CAM_CONTROL ){

		flyActual->etiqueta = Track->id;
		flyActual->Color = Track->Color;

		flyActual->direccion = phiZk;
		flyActual->Ax = Ax;
		flyActual->Ay = Ay;
		flyActual->Vx = vx_Zk;
		flyActual->Vy = vy_Zk;

		flyActual->dstTotal = Track->dstTotal;
		flyActual->Estado = Track->EstadoBlob;
	}
	if( EstadoTrack ==  KALMAN_CONTROL){
		// actualizar fly
		flyActual->etiqueta = 0;
		flyActual->Color = cvScalar( 255, 255, 255);

		flyActual->direccion = flyActual->orientacion;
		flyActual->Ax = 0;
		flyActual->Ay = 0;
		flyActual->Vx = 0;
		flyActual->Vy = 0;

		flyActual->dstTotal = 0;

	}


}

void visualizarKalman( STFrame* frameData, tlcde* lsTracks) {

	STTrack* Track;

	if( SHOW_KALMAN_DATA || (SHOW_VISUALIZATION && SHOW_KALMAN) ){
		for(int i = 0; i < lsTracks->numeroDeElementos ; i++){
			Track = (STTrack*)obtener(i, lsTracks);
			// EN CONSOLA
			if(SHOW_KALMAN_DATA ){
				printf("\n*********************** FRAME %d; BLOB NUMERO %d ************************",frameData->num_frame,Track->id);
				showKalmanData( Track );
			}
			if(SHOW_VISUALIZATION && SHOW_KALMAN){
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
		cvAdd(ImKalman,frameData->ImKalman,frameData->ImKalman );


	}
}

void showKalmanData( STTrack *Track){

	printf("\n\nReal State: ");
	printf("\n\tCoordenadas: ( %f , %f )",Track->Medida->data.fl[0],Track->Medida->data.fl[1]);
	printf("\n\tVelocidad: ( %f , %f )",Track->Medida->data.fl[2],Track->Medida->data.fl[3]);
	printf("\n\tDirección:  %f ",Track->Medida->data.fl[4]);

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

	printf("\n Error Process noise:\n\n\t%0.3f\t0\t0\t0\t0\n\t0\t%0.3f\t0\t0\t0\n\t0\t0\t%0.3f\t0\t0\n\t0\t0\t0\t%0.3f\t0\n\t0\t0\t0\t0\t%0.3f",
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

int deadTrack( tlcde* Tracks, int id ){

	STTrack* Track = NULL;
	Track = (STTrack*)borrarEl( id , Tracks);
	if(Track) {
		cvReleaseKalman(&Track->kalman);

		cvReleaseMat( &Track->x_k_Pre_ );

		cvReleaseMat( &Track->P_k_Pre_ );

		cvReleaseMat( &Track->Medida );
		cvReleaseMat(&Track->z_k );
		cvReleaseMat(&Track->Measurement_noise );
		free(Track);
		Track = NULL;
		return 1;
	}
	else return 0;
}

void liberarTracks( tlcde* lista){
	// Borrar todos los elementos de la lista
	STTrack *Track = NULL;
	// Comprobar si hay elementos
	if(lista == NULL || lista->numeroDeElementos<1)  return;
	// borrar: borra siempre el elemento actual

	irAlPrincipio(lista);
	Track = (STTrack *)borrar(lista);
	while (Track)
	{
		cvReleaseKalman(&Track->kalman);

		cvReleaseMat( &Track->x_k_Pre_ );

		cvReleaseMat( &Track->P_k_Pre_ );

		cvReleaseMat( &Track->Medida );
		cvReleaseMat(&Track->z_k );
		cvReleaseMat(&Track->Measurement_noise );


		free(Track); // borrar el área de datos del elemento eliminado
		Track = NULL;
		Track = (STTrack *)borrar(lista);
	}
}

/// Limpia de la memoria las imagenes usadas durante la ejecución
void DeallocateKalman( tlcde* lista ){

	cvReleaseImage( &ImKalman);

	cvReleaseMat( &CoordReal);

	liberarTracks( lista);

}

void copyMat (CvMat* source, CvMat* dest){

	int i,j;

	for (i=0; i<source->rows; i++)
		for (j=0; j<source->cols;j++)
			dest->data.fl[i*source->cols+j]=source->data.fl[i*source->cols+j];

}
