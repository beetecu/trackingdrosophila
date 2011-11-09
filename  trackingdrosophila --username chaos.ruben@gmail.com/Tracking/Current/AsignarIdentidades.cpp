/*
 * AsignarIdentidades.cpp
 *
 *  Created on: 02/09/2011
 *      Author: chao
 */

#include "AsignarIdentidades.hpp"

using namespace cv;
// various tracking parameters (in seconds)
#define NUMBER_OF_IDENTITIES 300

double MHI_DURATION = 0.3;//0.3
double MAX_TIME_DELTA = 0.5;	//0.5
double MIN_TIME_DELTA = 0.05;	//0.05
double dstUmbral = 1; // umbral en pixels para establecer si la mosca está quieta
// number of cyclic frame buffer used for motion detection
// (should, probably, depend on FPS)
const int N = 4;

// ring image buffer
//IplImage **buf = 0;
//int last = 0;

// temporary images
IplImage* img;	// Imagen donde dibujaremos la elipse para comparar con el mhi en matching
IplImage* imgf; // en float
IplImage* comp; // Imagen para realizar una comparacion
IplImage* silh; // Imagen con las siluetas para el mhi
IplImage *mhi = 0; // MHI
IplImage *orient = 0; // orientation
IplImage *mask = 0; // valid orientation mask
IplImage *segmask = 0; // motion segmentation map

void MotionTemplate( tlcde* framesBuf, tlcde* Etiquetas){

static	int TBposMHI = 30;
static	int TBposDMin = 5;
static	int TBposDMax = 50;

	float div = 100;
	int i, idx2;
//	int  idx1 = last;
	CvMemStorage* storage = 0; // temporary storage
	CvSeq* seq;
	CvRect comp_rect;
	double count;
	double angle;
	CvPoint center;
	double magnitude;
	CvScalar color;

	STFrame* frameData;
	tlcde* flies;
	STFly* fly;

	cvCreateTrackbar( "MHI Duration",
							  "Motion",
							  &TBposMHI,
							  500);
	cvCreateTrackbar( "MAX TIME DELTA",
								  "Motion",
								  &TBposDMax,
								  100);
	cvCreateTrackbar( "MIN_TIME_DELTA",
								  "Motion",
								  &TBposDMin,
								  50);
	MHI_DURATION = TBposMHI / div;
	MIN_TIME_DELTA = TBposDMin/div;
	MAX_TIME_DELTA = TBposDMax/div;

	irAlFinal(framesBuf);
	frameData = ( STFrame* )obtenerActual( framesBuf );
	allocateMotionTemplate(frameData->FG);

	cvZero( silh);
	flies = frameData->Flies;
	if( flies->numeroDeElementos>0 ) {
		irAlPrincipio( flies);
		for( int j = 0; j < flies->numeroDeElementos; j++){
			fly = (STFly*)obtener( j, flies);
			CvSize axes = cvSize( cvRound(fly->a) , cvRound(fly->b) );
			cvEllipse( silh, fly->posicion, axes, fly->orientacion, 0, 360, cvScalar( 255,0,0,0), -1, 8);
		}
		if( SHOW_DATA_ASSIGNMENT){
			if( framesBuf->numeroDeElementos > 1) {
				printf(" \nFlies FRAME t\n");
				mostrarListaFlies(framesBuf->numeroDeElementos-1,framesBuf);
				printf(" \nFlies FRAME t-1\n");
				mostrarListaFlies(framesBuf->numeroDeElementos-2,framesBuf);
			}
		}
	}



	double timestamp = (double)clock()/CLOCKS_PER_SEC; // get current time in seconds
	cvUpdateMotionHistory( silh, mhi, timestamp, MHI_DURATION ); // update MHI

	// convert MHI to blue 8u image
	cvCvtScale( mhi, mask, 255./MHI_DURATION,(MHI_DURATION - timestamp)*255./MHI_DURATION );
	cvZero( frameData->ImMotion );
	cvMerge( mask, 0, 0, 0, frameData->ImMotion );
   // calculate motion gradient orientation and valid orientation mask
	cvCalcMotionGradient( mhi, mask, orient, MAX_TIME_DELTA, MIN_TIME_DELTA, 3 ); //0.01, 0.5

	if( !storage )
		storage = cvCreateMemStorage(0);
	else
		cvClearMemStorage(storage);

	// segment motion: get sequence of motion components
	// segmask is marked motion components map. It is not used further
	seq = cvSegmentMotion( mhi, segmask, storage, timestamp, MAX_TIME_DELTA );
//	cvShowImage( "Motion",segmask);
//			cvWaitKey(0);
	// iterate through the motion components,
	// One more iteration (i == -1) corresponds to the whole image (global motion)
	for( i = -1; i < seq->total; i++ ) {

		if( i < 0 ) { // case of the whole image
			comp_rect = cvRect( 0, 0, cvSize(mhi->width,mhi->height).width, cvSize(mhi->width,mhi->height).height );
			color = CV_RGB(255,255,255);
			magnitude = 100;
		}
		else { // i-th motion component
			comp_rect = ((CvConnectedComp*)cvGetSeqElem( seq, i ))->rect;
			CvSeq* contour =  ((CvConnectedComp*)cvGetSeqElem( seq, i ))->contour;
//			if( comp_rect.width + comp_rect.height < 100 ) // reject very small components
//				continue;
			color = CV_RGB(255,0,0);
			magnitude = 30;
	//						cvShowImage( "Motion",orient);
	//								cvWaitKey(0);
			// select component ROI
			cvSetImageROI( silh, comp_rect );

			cvSetImageROI( mhi, comp_rect );
			cvSetImageROI( orient, comp_rect );
			cvSetImageROI( mask, comp_rect );

			// calculate orientation
			angle = cvCalcGlobalOrientation( orient, mask, mhi, timestamp, MHI_DURATION);
			//angle = 360.0 - angle;  // adjust for images with top-left origin

			count = cvNorm( silh, 0, CV_L1, 0 ); // calculate number of points within silhouette ROI

			cvResetImageROI( mhi );
			cvResetImageROI( orient );
			cvResetImageROI( mask );
			cvResetImageROI( silh );

			// check for the case of little motion
			if( count < comp_rect.width*comp_rect.height * 0.05 )
				continue;

			fly = matchingIdentity( framesBuf , Etiquetas, comp_rect, angle );

			// draw a clock with arrow indicating the direction
			center = cvPoint( (comp_rect.x + comp_rect.width/2),
							  (comp_rect.y + comp_rect.height/2) );
			double op_angle = 360.0 - angle;  // adjust for images with top-left origin
			cvCircle( frameData->ImMotion, center, cvRound(magnitude*1.2), color, 3, CV_AA, 0 );
			cvLine( frameData->ImMotion, center, cvPoint( cvRound( center.x + magnitude*cos(op_angle*CV_PI/180)),
					cvRound( center.y - magnitude*sin(op_angle*CV_PI/180))), color, 3, CV_AA, 0 );
	//		imshow( "Motion",frameData->ImMotion);
	//					cvWaitKey(0);

			}//ELSE
	}// Fin asignaciones

	if( SHOW_DATA_ASSIGNMENT){
		if( framesBuf->numeroDeElementos > 1 ) {

			printf(" \n\nFlies FRAME t tras asignacion\n");
			mostrarListaFlies(framesBuf->numeroDeElementos-1,framesBuf);
			printf(" \nFlies FRAME t-1\n");
			mostrarListaFlies(framesBuf->numeroDeElementos-2,framesBuf);
		}
	}
}



STFly* matchingIdentity( tlcde* framesBuf , tlcde* ids , CvRect MotionRoi, double angle ){

	// buscamos  el blob que se corresponde con la roi de la plantilla
	//de movimiento y su silueta en la lista en el frame t y en el t-1

	STFrame* frameData;
	STFly* fly;
	STFly* flyActual = NULL;
	STFly* flyAnterior = NULL;
	tlcde* flies;
	Identity* id;


	int i = 0, imgNoZ = 0, compNoZ = 0;
	int j = 0;
	irAlFinal( framesBuf);


	// si primer frame
	cvSetImageROI( img, MotionRoi);
	cvSetImageROI( imgf, MotionRoi);
	cvSetImageROI( comp, MotionRoi);
	cvSetImageROI( mhi, MotionRoi );


	// recorremos el último frame y el anterior si procede buscando la coincidencia de siluetas de la lista con el mhi
	// De la lista solo buscaremos aquellos que estén en el foreground (estado  = 1)
	for( j = 0; j < 2 ; j++ ){
		frameData = ( STFrame* )obtenerActual( framesBuf );
		flies = frameData->Flies;
		if(flies->numeroDeElementos== 0) return 0;
		// si en el frame anterior no hay blobs en el fg

		irAlPrincipio( flies );
		i = 0;
		for( i = 0; i< flies->numeroDeElementos; i++)
		{
			// igual se puede añadir una heuristica(x ej que no comprueba moscas con el centro alejado de la motionroi
			// obtenemos la mosca
			fly = (STFly*)obtener(i, flies);
			//si en el frame enterior no hay blobs en el fg
			if(( j == 1 )&&( flies->numeroDeElementos== 0)) break;
			if( fly->Estado == 0) continue; // si no está en el fg continuamos
			// si su centro no está dentro de la motionRoi continuamos
			if( ( fly->posicion.x < MotionRoi.x) ||
					(fly->posicion.x > (MotionRoi.x + MotionRoi.width) ) ) continue;
			if( ( fly->posicion.y < MotionRoi.y) ||
								(fly->posicion.y > (MotionRoi.y + MotionRoi.height) ) ) continue;
			// dibujamos su silueta
			cvResetImageROI( img );
			cvZero(img);
			//cvZero(imgf);
			CvSize axes = cvSize( cvRound(fly->a) , cvRound(fly->b) );
			cvEllipse( img, fly->posicion, axes, fly->orientacion, 0, 360, cvScalar( 255,0,0,0), -1, 8);
			// comparamos con el mhi

			cvResetImageROI( mhi );
			if( (frameData->num_frame == 122)&&(j == 1) ){
			cvShowImage("Foreground",img);
//			cvWaitKey(0);
			cvShowImage("Background",mhi);
//			cvWaitKey(0);
			}
			cvSetImageROI( img, MotionRoi);
			cvSetImageROI( mhi, MotionRoi);

			imgNoZ = cvCountNonZero(img);
			cvConvertScale( img, imgf,1,0);
			cvAnd(imgf,mhi,comp);
			compNoZ = cvCountNonZero(comp);
			// si el numero de elementos distintos de 0 coinciden

			//if( ( abs(compNoZ - imgNoZ )< 20) &&( compNoZ != 0) &&( imgNoZ != 0) ){
				// es el ultimo frame
				if( ( framesBuf->numeroDeElementos -1) == (framesBuf->posicion) ){
					flyActual = fly;
					printf("\n%d con ",flies->posicion);
					break;
				}
				else {
					flyAnterior = fly;
					printf("%d",flies->posicion);
					break;
				}

				// ¿ podria haber mas de una coincidencia en el mismo frame??? si
			//}

		}//FOR
		if ( framesBuf->numeroDeElementos < 2) break; // es el primer frame( no hay anterior)
		irAlAnterior(framesBuf);

	}//FOR
	//si se ha encontrado la correspondencia hacemos la asignación etiquetándolos igual
	if( flyActual && flyAnterior ){
		enlazarFlies( flyAnterior, flyActual, NULL);
		SetTita( flyAnterior, flyActual, angle );
		printf(" Angle = %0.1f",angle);
//		if( ( flyActual->dstTotal - flyAnterior->dstTotal) < dstUmbral) flyActual->CountState+=1;
//		else flyActual->CountState = 0;
//		if( flyActual->CountState == 5) flyActual->Estado = 0;//Mosca quieta. al oldfg
	}
	// si se ha encontrado correspondencia en el actual y no en el anterior, es un nuevo elemento
	else if ( flyActual && !flyAnterior ){
		printf("nadie. No se han encontrado coincidencias en el frame t-1");
		// nueva id
		asignarNuevaId( flyActual, ids);
		//mostrarIds(ids);
	}
	else if(!flyActual) {
		printf("/nNo se han encontrado coincidencias en el frame t");
	}

	cvResetImageROI( mhi );
	cvResetImageROI( comp );
	cvResetImageROI( img );
	cvResetImageROI( imgf );
	return flyActual;

}


void CrearIdentidades(tlcde* Identities){

	Identity* Id;
	RNG rng(0xFFFFFFFF); // para generar un color aleatorio
//	char nombre[10];
//	nombre = "Vidal";
//	nombre = "Pepe";
//	nombre = "Tomas";
//	nombre = "Pablo";
	int i = NUMBER_OF_IDENTITIES-1;
	for(i=NUMBER_OF_IDENTITIES-1; i >= 0 ; i--){
		Id = ( Identity* )malloc( sizeof(Identity ));
		Id->etiqueta = i + 1;
		Id->color = randomColor(rng);
		anyadirAlFinal( Id, Identities);
	}
}

void liberarIdentidades(tlcde* lista){
	  // Borrar todos los elementos de la lista
	Identity* id;
	  // Comprobar si hay elementos
	  if (lista->numeroDeElementos == 0 ) return;
	  // borrar: borra siempre el elemento actual
	  irAlPrincipio( lista );
	  id = (Identity *)borrar(lista);
	  int i = 0;
	  while( id ){
		  free (id);
		  id = NULL;
		  id = (Identity *)borrar(lista);
	  }
}

void asignarNuevaId( STFly* fly, tlcde* identities){
	Identity *id;
	id = (Identity* )borrarEl( identities->numeroDeElementos - 1, identities);
	fly->etiqueta = id->etiqueta;
	fly->Color = id->color;
	free(id);
	id = NULL;
}

void dejarId( STFly* fly, tlcde* identities ){
	Identity *Id;
	Id = ( Identity* )malloc( sizeof(Identity ));
	Id->etiqueta = fly->etiqueta;
	Id->color = fly->Color;
	anyadirAlFinal( Id , identities );
}

void mostrarIds( tlcde* Ids){

	Identity* id;
	irAlFinal(Ids);

	for(int i = 0; i <Ids->numeroDeElementos ; i++ ){
		id = (Identity*)obtener(i, Ids);
		printf("Id = %d\n", id->etiqueta);
	}
}

void enlazarFlies( STFly* flyAnterior, STFly* flyActual, tlcde* ids ){
	if( (flyActual->etiqueta == 0)||(ids == NULL) ){
		flyActual->etiqueta = flyAnterior->etiqueta;
		flyActual->Color = flyAnterior->Color;
	}
	else{
		dejarId( flyActual, ids);
		flyActual->etiqueta = flyAnterior->etiqueta;
		flyActual->Color = flyAnterior->Color;
	}
	float distancia;
	//Establecemos la dirección y el modulo del vector de desplazamiento
	EUDistance( flyAnterior->posicion,flyActual->posicion, &flyAnterior->direccion, &distancia );
	flyActual->dstTotal = flyAnterior->dstTotal + distancia;
//	SetTita( flyActual, flyAnterior );
}
/// Haya la distancia ecuclidea entre dos puntos. Establece el modulo y y el argumento en grados.
///
void EUDistance( CvPoint posicion1, CvPoint posicion2, float* direccion, float* distancia){

	float b;
	float a;
	b = posicion2.x - posicion1.x;
	a =  posicion2.y - posicion1.y;

	if( ( b < 0)&&(a < 0) )
	{
		*direccion = atan( b / a );
		//resolvemos ambiguedad debida a los signos en la atan
		*direccion = *direccion + PI;
		*direccion = ( (*direccion) *180)/PI; // a grados
	}
	else if( ( b == 0) && ( a == 0) ){
		*direccion = -1;
	}
	else if( (b == 0)&&( a != 0) ){
		if (a < 0) *direccion = 180;
		else *direccion = 0;
	}
	else if( (b != 0)&&( a == 0) ){
		if( b > 0 ) *direccion = 270;
		else *direccion = 90;
	}
	else {
		*direccion = atan( b / a );
		*direccion = ( (*direccion) *180)/PI;
	}

	// calcular distancia para comprobar si hay desplazamiento. si es menor que un umbral
	// consideramos que esta quieto.
	*distancia = sqrt( pow( (posicion2.y-posicion1.y) ,2 )  + pow( (posicion2.x - posicion1.x) ,2 ) );

}

/// resuelve la ambiguedad en la orientación estableciendo ésta
/// en función de la dirección del desplazamiento.Siempre devuelve un ángulo
/// entre 0 y 360º
void SetTita( STFly* flyAnterior,STFly* flyActual,double angle ){

//	float sigma = flyAnterior->direccion;
	// si la dirección y la orientación difieren en más de 90º
	flyAnterior->direccion = angle;
	if ( abs( flyActual->orientacion - angle ) > 90) {
		// establecemos la orientación según la dirección. Si sobrepasa los 360 le restamos 180
		// si no los sobrepasa se los sumamos. Así el resultado siempre estará entre 0 y 360.
		if (flyActual->orientacion > 180) flyActual->orientacion = flyActual->orientacion - 180;
		else flyActual->orientacion = flyActual->orientacion + 180;
		//flyActual->orientacion = flyAnterior->orientacion;
	}
	//else	flyActual->orientacion = flyAnterior->orientacion;
}

static Scalar randomColor(RNG& rng)
{
    int icolor = (unsigned)rng;
    return Scalar(icolor&255, (icolor>>8)&255, (icolor>>16)&255);
}

void allocateMotionTemplate( IplImage* im){

	CvSize size = cvSize(im->width,im->height); // get current frame size
		if( !mhi || mhi->width != size.width || mhi->height != size.height ) {
	//	        if( buf == 0 ) {
	//	            buf = (IplImage**)malloc(N*sizeof(buf[0]));
	//	            memset( buf, 0, N*sizeof(buf[0]));
	//	        }
	//	        for( i = 0; i < N; i++ ) {
	//	            cvReleaseImage( &buf[i] );
	//	            buf[i] = cvCreateImage( size, IPL_DEPTH_8U, 1 );
	//	            cvZero( buf[i] );
	//	        }
			    cvReleaseImage( &img );
			    cvReleaseImage( &imgf );
			    cvReleaseImage( &comp );
		        cvReleaseImage( &mhi );
		        cvReleaseImage( &orient );
		        cvReleaseImage( &segmask );
		        cvReleaseImage( &mask );
		        cvReleaseImage( &silh);

		        img =  cvCreateImage( size, IPL_DEPTH_8U, 1 );
		        imgf =  cvCreateImage( size, IPL_DEPTH_32F, 1 );
		        comp =  cvCreateImage( size,  IPL_DEPTH_32F, 1 );
		        mhi = cvCreateImage( size, IPL_DEPTH_32F, 1 );
		        cvZero( mhi ); // clear MHI at the beginning
		        silh = cvCreateImage( size, IPL_DEPTH_8U, 1 );
		        orient = cvCreateImage( size, IPL_DEPTH_32F, 1 );
		        segmask = cvCreateImage( size, IPL_DEPTH_32F, 1 );
		        mask = cvCreateImage( size, IPL_DEPTH_8U, 1 );
		    }
}
void releaseMotionTemplate(){

	cvReleaseImage( &img );
	cvReleaseImage( &imgf );
	cvReleaseImage( &comp );
	cvReleaseImage( &mhi );
	cvReleaseImage( &orient );
	cvReleaseImage( &segmask );
	cvReleaseImage( &mask );
	cvReleaseImage( &silh);

}

//void TrackbarSliderMHI(  int pos ){
//	float div = 100;
//	MHI_DURATION = pos / div;
//}
//
//void TrackbarSliderDMin(int pos){
//	float div = 100;
//	MIN_TIME_DELTA = pos/div;
//}
//
//void TrackbarSliderDMax(int pos){
//	float div=100;
//	MAX_TIME_DELTA = pos/div;
//}
