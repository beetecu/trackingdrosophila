/*
 * AsignarIdentidades.cpp
 *
 *  -Resuelve la ambiguedad en la orientación mediante el cálculo
 *   del gradiente global del movimiento de la escena tras ser segmentado
 *   en movimientos locales.
 *  -Hace una primera asignación de identidad mediante una plantilla de movimiento.
 *  Cada nuevo objeto del mhi se etiqueta como un nuevo elemento.
 *  en caso contrario, se etiqueta igual que el blob correspondiente
 *  del frame anterior. Los casos de fisión y fusión de blobs se etiquetan
 *  como 0.
 *  MotionTemplate trabaja desde el penultimo elemento del buffer
 *  hacia atrás, dependiendo el número de frames de MHI_DURATION .
 *  Created on: 02/09/2011
 *      Author: chao
 */

#include "AsignarIdentidades.hpp"


using namespace cv;
// various tracking parameters (in seconds)


double MHI_DURATION = 0.2;//0.3
double MAX_TIME_DELTA = 0.5;	//0.5
double MIN_TIME_DELTA = 0.05;	//0.05
double dstUmbral = 1; // umbral en pixels para establecer si la mosca está quieta
int FIJAR_ORIENTACION = 3 ;// numero de frames que transcurren
// number of cyclic frame buffer used for motion detection
// (should, probably, depend on FPS)
const int N = 3;

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



	float div = 100;
	int i;

	CvMemStorage* storage = 0; // temporary storage
	CvSeq* seq;
	CvRect comp_rect;
	double count;
	double angle;
	CvPoint center;
	double magnitude;
	CvScalar color;

	const int Idx1 = framesBuf->numeroDeElementos-1; // Zona de trabajo de motion template. ultimo frame
	static int Idx2 ; //  penultimo frame
	static int Idx3;  //  antepenultimo frame

	STFrame* frameIdx2 = NULL;
	STFrame* frameIdx1 = NULL;
	STFrame* frameIdx3 = NULL;

	tlcde* flies;
	STFly* fly;

	// Trackbars para ajustar los parametros de la motion template
	if(CREATE_TRACKBARS){
		static	int TBposMHI = 20;
		static	int TBposDMin = 5;
		static	int TBposDMax = 50;
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
	}

	// obtenemos los frames de trabajo
	irAl(Idx1,framesBuf);
	frameIdx1 = ( STFrame* )obtener(Idx1, framesBuf );
	if( framesBuf->numeroDeElementos > 1){
		Idx2 = framesBuf->numeroDeElementos-2;
		frameIdx2 = ( STFrame* )obtener(Idx2, framesBuf );
	}
	if( framesBuf->numeroDeElementos > 2){
		Idx3 = framesBuf->numeroDeElementos-3;
		frameIdx3 = ( STFrame* )obtener(Idx3, framesBuf );
	}

	allocateMotionTemplate(frameIdx1->FG);
//	if(frameIdx1->num_frame == 140){
//		printf("hola");
//	}

	flies = frameIdx1->Flies;

	if( SHOW_MT_DATA ){
		printf( "\nMostrando datos de Motion Template:\n");
		if( framesBuf->numeroDeElementos > 1) {
			printf(" \nFlies FRAME t inicial\n");
			mostrarListaFlies(Idx1,framesBuf);
			printf(" \nFlies FRAME t-1 inicial\n");
			mostrarListaFlies(Idx2,framesBuf);
		}
	}
	// Dibujamos las siluetas del frame del punto de trabajo para obtener el fg.
	dibujarFG( flies, silh, CLEAR );

//	cvShowImage("Foreground",silh);
//	cvWaitKey(0);
	double timestamp = (double)clock()/CLOCKS_PER_SEC; // get current time in seconds
	cvUpdateMotionHistory( silh, mhi, timestamp, MHI_DURATION ); // update MHI

	// convert MHI to blue 8u image
	cvCvtScale( mhi, mask, 255./MHI_DURATION,(MHI_DURATION - timestamp)*255./MHI_DURATION );
	cvZero( frameIdx1->ImMotion );
	cvMerge( mask, 0, 0, 0, frameIdx1->ImMotion );
   // calculate motion gradient orientation and valid orientation mask
	cvCalcMotionGradient( mhi, mask, orient, MAX_TIME_DELTA, MIN_TIME_DELTA, 3 ); //0.01, 0.5
//	cvShowImage( "Foreground", orient);
//	cvWaitKey(0);

	// añade a la lista del frame actual los blobs que previamente estaban en el oldfg y
	// establecemos el estado de los blobs que pasan al oldFG

//	establecerEstado(frameIdx1,frameIdx2,frameIdx3, orient);

//	if( SHOW_MT_DATA){
//		if( framesBuf->numeroDeElementos > 1) {
//			printf(" \nFlies FRAME t tras establecer estado\n");
//			mostrarListaFlies(Idx1,framesBuf);
//			printf(" \nFlies FRAME t-1 tras establecer estado\n");
//			mostrarListaFlies(Idx2,framesBuf);
//		}
//	}

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
			angle = 360.0 - angle;  // adjust for images with top-left origin

			count = cvNorm( silh, 0, CV_L1, 0 ); // calculate number of points within silhouette ROI

			cvResetImageROI( mhi );
			cvResetImageROI( orient );
			cvResetImageROI( mask );
			cvResetImageROI( silh );

			// check for the case of little motion
			if( count < comp_rect.width*comp_rect.height * 0.05 )	continue;

			fly = matchingIdentity(frameIdx1, frameIdx2, Etiquetas, comp_rect, angle );

			if(SHOW_MOTION_TEMPLATE){
				// draw a clock with arrow indicating the direction
				center = cvPoint( (comp_rect.x + comp_rect.width/2),
								  (comp_rect.y + comp_rect.height/2) );
				cvCircle( frameIdx1->ImMotion, center, cvRound(magnitude*1.2), color, 3, CV_AA, 0 );
				cvLine( frameIdx1->ImMotion, center, cvPoint( cvRound( center.x + magnitude*cos(angle*CV_PI/180)),
						cvRound( center.y - magnitude*sin(angle*CV_PI/180))), color, 3, CV_AA, 0 );
			}


			}//ELSE
	}// Fin asignaciones

	//añadimos al frame actual las moscas no asignadas del oldfg

//	anyadirEstado0( frameIdx2, frameIdx1);

//	frameIdx2 = ( STFrame* )obtener(framesBuf->numeroDeElementos-3, framesBuf );
//	fliesIdx2 = frameIdx2->Flies;

	if( SHOW_MT_DATA){
		if( framesBuf->numeroDeElementos > 1 ) {
			printf(" \n\nFlies FRAME t tras asignacion\n");
			mostrarListaFlies(Idx1,framesBuf);
			printf(" \nFlies FRAME t-1\n");
			mostrarListaFlies(Idx2,framesBuf);
			printf("\n");
		}
	}
}



STFly* matchingIdentity( STFrame* frameActual , STFrame*frameAnterior, tlcde* ids , CvRect MotionRoi, double angle ){

	// buscamos  el blob que se corresponde con la roi de la plantilla
	//de movimiento y su silueta en la lista en el frame t y en el t-1

	STFrame* frameData;
	STFly* fly;
	STFly* flyActual = NULL;
	STFly* flyAnterior = NULL;
	tlcde* flies;
	int numActual = 0; // numero de coincidencias en el frame actual
	int numAnterior = 0; // numero de coincidencias en el frame anterior
	int posActual[10]; // vectores para almacenar las posiciones de las coincidencias
	int posAnterior[10];


	// buscamos posibles asignaciones en el fg(t)  con el oldFG (t-1).


	// buscamos posibles asignaciones en el fg(t) con el fg (t-1).

	numActual = buscarFlies( frameActual , MotionRoi, posActual );
	if( numActual == 0 ){
		printf("/nNo se han encontrado coincidencias en el frame t");
		return 0;
	}
	numAnterior = buscarFlies( frameAnterior, MotionRoi, posAnterior);

	// Caso de asignación normal
	if( numAnterior == 1 && numActual == 1){
		flyActual = (STFly*)obtener( posActual[0], frameActual->Flies );
		flyAnterior = (STFly*)obtener( posAnterior[0], frameAnterior->Flies);
		enlazarFlies( flyAnterior, flyActual, NULL);
		SetTita( flyAnterior, flyActual, angle, FIJAR_ORIENTACION);
	}
	// Caso de nueva etiqueta ( nuevo blob )
	else if( numAnterior == 0 && numActual == 1){
		flyActual = (STFly*)obtener( posActual[0], frameActual->Flies );
		asignarNuevaId( flyActual, ids);
		flyActual->FrameCount = flyActual->FrameCount +1;
	}
	// caso de fisión de blobs. un blob se separa en 2 o mas mientras su mhi permanece
	// unido resultando en varias coincidencias en actual y una en anterior
	else if( numActual >= numAnterior){
		// etiquetamos todos los nuevos blobs como el anterior
//		for ( int i = 0; i < numActual; i++ ){
//					flyAnterior = (STFly*)obtener( posAnterior[0], frameAnterior->Flies );
//					flyActual = (STFly*)obtener( posActual[i], frameActual->Flies );
//					enlazarFlies( flyAnterior, flyActual, ids);
//		}
	}
	// Caso de fusión de blobs. 2 o mas blobs que se unen en uno y su mhi se funde en uno
	// resultando una coincidencia en actual y varias en anterior
	//etiquetamos el nuevo como el primero de los anteriores
	else if( numActual <= numAnterior){
//		for( int i = 0; i = numActual; i++){
//			for ( int j = 0; i < numAnterior; i++ ){
//				flyActual = (STFly*)obtener( posActual[0], frameActual->Flies );
//
//				flyAnterior = (STFly*)obtener( posAnterior[j], frameAnterior->Flies );
//				enlazarFlies( flyAnterior, flyActual, ids);
//			}
//		}
	}

	//else if( !numActual && numAnterior)
	return flyActual;

}
///! establecemos el estado del blob. Si la máscara del gradiente del blob no tiene
/// pixels distintos de 0 en el area correspondiente a la roi del blob del frame t-1
/// establecemos su estado a 0 (al oldFG ) y el de los 2 frames anteriores pare evitar su asignacion
/// en matching

void establecerEstado( STFrame* frame0, STFrame* frame1, STFrame* frame2,IplImage* orient){


	tlcde* flies0 = NULL;
	tlcde* flies1 = NULL;
	STFly* fly = NULL;

	if( !frame1 ) return;
	flies0 = frame0->Flies;
	flies1 = frame1->Flies;

	irAlPrincipio( flies1 );
	for( int i = 0; i < flies1->numeroDeElementos; i++){
		fly = (STFly*)obtener( i, flies1 );
		if( fly->Estado == 1){
			cvSetImageROI( orient, fly->Roi );
			// si todos los pixel de la mascara de orientacion son 0 al bg
			if( !cvCountNonZero( orient) ){
				fly->Estado = 0;
				corregirEstado(  frame0, frame1, frame2, flies1->posicion );
			}
			cvResetImageROI( orient);
		}else{ // si el estado es 0, los añadimos a la lista como blobs del oldfg
			anyadirAlFinal( fly, flies0 );
			fly->num_frame = frame0->num_frame;
		}
	}
}

void corregirEstado( STFrame* frame0, STFrame* frame1, STFrame* frame2, int pos ){

	STFly* fly;
	STFly* fly0 = NULL;
	STFly* fly1 = NULL;
	STFly* fly2 = NULL;

	// nos situamos en frame desde el que empezaremos la correccion del elemento teniendo en cuenta
	// el llenado del buffer
	// corregimos el frame actual los n frames anteriores anterior

	fly1 = (STFly*)obtener(pos,frame1->Flies);

	// buscamos el elemento a corregir en el t-2
	if( frame2!=NULL){

		for( int i  = 0; i < frame2->Flies->numeroDeElementos; i++){
			// obtenemos el primer elemento cuyo estado estableceremos a 0 y usamos
			// sus datos para corregir el resto de frames
			irAl(i,frame2->Flies);
			fly= ( STFly* )obtenerActual(frame2->Flies );
			if( fly->etiqueta == fly1->etiqueta ){
				fly->Estado = 0;
				fly2 = fly ;
				break;
			}
		}
	}
	if(fly2){
		// corregimos el frame t-1
		irAl(pos,frame1->Flies);
		modificar( fly2, frame1->Flies );
		// corregimos el numero de frame
		fly1->num_frame = frame1->num_frame;
		//lo añadimos al final al t
		anyadirAlFinal(fly1, frame0->Flies );
		fly0 = (STFly*)obtener(frame0->Flies->numeroDeElementos-1,frame0->Flies);
		// corregimos el numero de frame
		fly0->num_frame = frame0->num_frame;
	}
	else{
		// lo añadimos al frame t
		anyadirAlFinal(fly1, frame0->Flies );
		fly0 = (STFly*)obtener(frame0->Flies->numeroDeElementos-1,frame0->Flies);
		fly0->num_frame = frame0->num_frame;
	}
}

/// recibe el frame y busca en la lista flies los blobs que se corresponden cn la silueta de la
/// roi del mhi
/// devuelve el número de coincidencias y un vector con las posiciones donde se encontraron coincidencias
int buscarFlies( STFrame* frameData ,CvRect MotionRoi, int *p ){

	int i = 0, imgNoZ = 0, compNoZ = 0;
	int j = 0;
	STFly* fly;
	tlcde* flies;

	// recorremos el penúltimo frame y el anterior si procede buscando coincidencias de siluetas de la lista
	// en la ROI del mhi
	// De la lista solo buscaremos aquellos que estén en el foreground (estado  = 1)
	if (!frameData) return 0;
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

		if( fly->Estado == 0) continue; // si no está en el fg continuamos
		// si su centro no está dentro de la motionRoi continuamos
		if( ( fly->posicion.x < MotionRoi.x) ||
				(fly->posicion.x > (MotionRoi.x + MotionRoi.width) ) ) continue;
		if( ( fly->posicion.y < MotionRoi.y) ||
							(fly->posicion.y > (MotionRoi.y + MotionRoi.height) ) ) continue;

		p[j] = i;
		j++;
		//}
	}//FOR

	// ponemos el ultimo elemento a null
	p[j] = NULL;
	return j;
}

void anyadirEstadoO( STFrame* frameAnterior, STFrame* frameActual){


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
