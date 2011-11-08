/*
 * AsignarIdentidades.cpp
 *
 *  Created on: 02/09/2011
 *      Author: chao
 */

#include "AsignarIdentidades.hpp"

// various tracking parameters (in seconds)
#define NUMBER_OF_IDENTITIES 300

double MHI_DURATION = 0.5;//0.3
double MAX_TIME_DELTA = 0.5;	//0.5
double MIN_TIME_DELTA = 0.05;	//0.05
double dstUmbral = 3; // umbral en pixels para establecer si la mosca está quieta
// number of cyclic frame buffer used for motion detection
// (should, probably, depend on FPS)
const int N = 4;

// ring image buffer
//IplImage **buf = 0;
//int last = 0;

// temporary images
IplImage* img;
IplImage* comp; // Imagen para realizar una comparacion
IplImage* silh; // Imagen con las siluetas
IplImage *mhi = 0; // MHI
IplImage *orient = 0; // orientation
IplImage *mask = 0; // valid orientation mask
IplImage *segmask = 0; // motion segmentation map

void MotionTemplate( tlcde* framesBuf, tlcde* Etiquetas){

static	int TBposMHI = 500;
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

	STFly* fly;

	frameData = ( STFrame* )obtenerActual( framesBuf );
	CvSize size = cvSize(frameData->FG->width,frameData->FG->height); // get current frame size
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
		    cvReleaseImage( &comp );
	        cvReleaseImage( &mhi );
	        cvReleaseImage( &orient );
	        cvReleaseImage( &segmask );
	        cvReleaseImage( &mask );
	        cvReleaseImage( &silh);

	        img =  cvCreateImage( size, IPL_DEPTH_8U, 1 );
	        comp =  cvCreateImage( size, IPL_DEPTH_8U, 1 );
	        mhi = cvCreateImage( size, IPL_DEPTH_32F, 1 );
	        cvZero( mhi ); // clear MHI at the beginning
	        silh = cvCreateImage( size, IPL_DEPTH_8U, 1 );
	        orient = cvCreateImage( size, IPL_DEPTH_32F, 1 );
	        segmask = cvCreateImage( size, IPL_DEPTH_32F, 1 );
	        mask = cvCreateImage( size, IPL_DEPTH_8U, 1 );
	    }
	cvCopy(frameData->FG,img);

//	cvZero(img);
	i = 0;

	// obtener máscara del fg con rectángulos en ved de elipses,

//	while( i <  frameData->Flies->numeroDeElementos ){
//		fly = (STFly*)obtener(i, frameData->Flies);
//		cvSetImageROI( img, fly->Roi);
//		invertirBW( img ); // añadir para invertir con roi
//		cvResetImageROI( img);
//		i++;
//	}
//	cvShowImage( "Motion",img);
//	cvWaitKey(0);

//	cvCopy( img, buf[last]);
//	idx2 = (last + 1) % N; // index of (last - (N-1))th frame
//	last = idx2;
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

	cvCopy( img, silh);
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
			comp_rect = cvRect( 0, 0, size.width, size.height );
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
		if( count < comp_rect.width*comp_rect.height * 0.05 )
			continue;

		//etiquetamos y establecemos la direccion

		fly = matchingIdentity( framesBuf , Etiquetas, comp_rect );

	//
		// Resolvemos ambiguedad en orientación y
//		if (framesBuf->numeroDeElementos == 1) etiquetar();
//		for( int y=comp_rect.y;  y< comp_rect.y+comp_rect.height; y++){
//			uchar* ptr = (uchar*) ( silh->imageData + y*silh->widthStep + 1*comp_rect.x);
//			for (int x = 0; x<comp_rect.width; x++) {
//				if (ptr[x] == 255){
//					i = 0;
//					while( i <  frameData->Flies->numeroDeElementos ){
//							fly = (STFly*)obtener(i, frameData->Flies);
//							if ( (fly->posicion.x == x)&&(fly->posicion.y == y) ) break;
//							i++;
//						}
//					if ( (fly->posicion.x == x)&&(fly->posicion.y == y) ) break;
//				}
//			}
//		}


		// draw a clock with arrow indicating the direction
		center = cvPoint( (comp_rect.x + comp_rect.width/2),
						  (comp_rect.y + comp_rect.height/2) );

		cvCircle( frameData->ImMotion, center, cvRound(magnitude*1.2), color, 3, CV_AA, 0 );
		cvLine( frameData->ImMotion, center, cvPoint( cvRound( center.x + magnitude*cos(angle*CV_PI/180)),
				cvRound( center.y - magnitude*sin(angle*CV_PI/180))), color, 3, CV_AA, 0 );
//		imshow( "Motion",frameData->ImMotion);
//					cvWaitKey(0);

		}
	}
}



STFly* matchingIdentity( tlcde* framesBuf , tlcde* ids , CvRect MotionRoi ){

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
	cvSetImageROI( comp, MotionRoi);
	cvSetImageROI( mhi, MotionRoi );


	// recorremos el último frame y el anterior si procede buscando la coincidencia de siluetas de la lista con el mhi
	// De la lista solo buscaremos aquellos que estén en el foreground (estado  = 1)
	for( j = 0; j =1 ; j++, irAlAnterior(framesBuf) ){
		frameData = ( STFrame* )obtenerActual( framesBuf );
		flies = frameData->Flies;
		irAlPrincipio( flies );
		i = 0;
		while( i <  flies->numeroDeElementos ){
			cvZero(img);
			// igual se puede añadir una heuristica(x ej que no comprueba moscas con el centro alejado de la motionroi
			// obtenemos la mosca
			fly = (STFly*)obtener(i, frameData->Flies);
			if( fly->Estado == 0) continue; // solo buscamos en el fg
			// dibujamos su silueta
			CvSize axes = cvSize( cvRound(fly->a) , cvRound(fly->b) );
			cvEllipse( img, fly->posicion, axes, fly->orientacion, 0, 360, cvScalar( 255,0,0,0), -1, 8);
			// comparamos con el mhi

			imgNoZ = cvCountNonZero(img);
			cvAnd(img,mhi,comp);
			compNoZ = cvCountNonZero(img);
			// si el numero de elementos distintos de 0 coinciden

			if( compNoZ == imgNoZ ){
				// es el ultimo frame
				if( ( framesBuf->numeroDeElementos -1) == (framesBuf->posicion) ){
					flyActual = fly;
				}
				else flyAnterior = fly;
				break; // ¿ podria haber mas de una coincidencia en el mismo frame???
			}
			i++;
		}//WHILE
		if ( framesBuf->numeroDeElementos < 2) break; // es el primer frame( no hay anterior)
	}//FOR
	//si se ha encontrado la correspondencia hacemos la asignación etiquetándolos igual
	if( flyActual && flyAnterior ){
		enlazarFlies( flyActual, flyAnterior, NULL);
		if( ( flyActual->dstTotal - flyAnterior->dstTotal) < dstUmbral) flyActual->CountState+=1;
		else flyActual->CountState = 0;
		if( flyActual->CountState == 3) flyActual->Estado = 0;//Mosca quieta. al oldfg
	}
	// si se ha encontrado correspondencia en el actual y no en el anterior, es un nuevo elemento
	else if ( flyActual && !flyAnterior ){
		printf("/nNo se han encontrado coincidencias en el frame t-1");
		// nueva id
		asignarNuevaId( flyActual, ids);
	}
	else if(!flyActual) printf("/nNo se han encontrado coincidencias en el frame t");

	cvResetImageROI( mhi );
	cvResetImageROI( comp );
	cvResetImageROI( img );
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
	id = (Identity* )borrarEl( NUMBER_OF_IDENTITIES - 1, identities);
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
	SetTita( flyActual, flyAnterior );
}
/// Haya la distancia ecuclidea entre dos puntos. Establece el modulo y y el argumento en grados.
///
void EUDistance( CvPoint posicion1, CvPoint posicion2, float* direccion, float* distancia){

	*direccion = atan( (posicion2.y - posicion1.y) / ( posicion2.x-posicion1.x) );
	if( (( posicion2.y - posicion1.y) < 0) && (( posicion2.x - posicion1.x) < 0) ){
		//resolvemos ambiguedad debida a los signos en la atan
		*direccion = *direccion + PI;
		*direccion = ( (*direccion) *180)/PI; // a grados
	}
	else *direccion = ( (*direccion) *180)/PI;
	// calcular distancia para comprobar si hay desplazamiento. si es menor que un umbral
	// consideramos que esta quieto.
	*distancia = sqrt( pow( (posicion2.y-posicion1.y) ,2 )  + pow( (posicion2.x - posicion1.x) ,2 ) );

}

/// resuelve la ambiguedad en la orientación estableciendo ésta
/// en función de la dirección del desplazamiento.Siempre devuelve un ángulo
/// entre 0 y 360º
void SetTita( STFly* flyActual,STFly* flyAnterior ){

	float sigma = flyAnterior->direccion;
	// si la dirección y la orientación difieren en más de 90º
	if ( abs( flyAnterior->orientacion - sigma ) > 90) {
		// establecemos la orientación según la dirección. Si sobrepasa los 360 le restamos 180
		// si no los sobrepasa se los sumamos. Así el resultado siempre estará entre 0 y 360.
		if (flyAnterior->orientacion > 180) flyAnterior->orientacion = flyAnterior->orientacion - 180;
		else flyAnterior->orientacion = flyAnterior->orientacion + 180;
	}
	// la orientacion  es la misma
	flyActual->orientacion = flyAnterior->orientacion;
}

static Scalar randomColor(RNG& rng)
{
    int icolor = (unsigned)rng;
    return Scalar(icolor&255, (icolor>>8)&255, (icolor>>16)&255);
}

void releaseMotionTemplate(){

	cvReleaseImage( &img );
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
