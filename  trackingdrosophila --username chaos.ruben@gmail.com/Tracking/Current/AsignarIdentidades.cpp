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
			//if(!fly) exit(1);
			if(SHOW_VISUALIZATION&&SHOW_MOTION_TEMPLATE){
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

int asignarIdentidades(  tlcde* lsTraks , tlcde *Flies){


	CvMat* CoorReal=cvCreateMat(1,2,CV_32FC1);
	CvMat* Matrix_Asignation= NULL;

	STTrack* Track=NULL;
	STTrack* TrackActual=NULL;

	STFly* FlyNext=NULL;

}

int asignarIdentidades(  tlcde* lsTraks , tlcde *Flies){


	CvMat* CoorReal=cvCreateMat(1,2,CV_32FC1);
	CvMat* Matrix_Asignation= NULL;

	STTrack* Track=NULL;
	STTrack* TrackActual=NULL;

	STFly* FlyNext=NULL;
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

int asignarIdentidades(  tlcde* lsTraks , tlcde *Flies){


	CvMat* CoorReal=cvCreateMat(1,2,CV_32FC1);
	CvMat* Matrix_Asignation= NULL;

	STTrack* Track=NULL;
	STTrack* TrackActual=NULL;

	STFly* FlyNext=NULL;
	STFly* FlySiguiente=NULL;

	double Hungarian_Matrix [lsTraks->numeroDeElementos][Flies->numeroDeElementos];
	int v=0;
	int g=0;
	int indCandidato; // posicion de la Fly con mayor porbabilidad.


	if( lsTraks->numeroDeElementos == 0  ) return 0;

	else if(Flies->numeroDeElementos >0){

		CvMat* Matrix_Hungarian = cvCreateMat(lsTraks->numeroDeElementos,Flies->numeroDeElementos,CV_32FC1); // Matriz de Pesos
		cvZero(Matrix_Hungarian);

				int p=0;

				for(int d=0;d < lsTraks->numeroDeElementos;d++){

					int id=0;

					Track=(STTrack*)obtener(d,lsTraks);

					for(int f=0;f < Flies->numeroDeElementos;f++){

						FlyNext=(STFly*)obtener(f,Flies);

							CoorReal->data.fl[0]=FlyNext->posicion.x;
							CoorReal->data.fl[1]=FlyNext->posicion.y;

							double Peso = PesosKalman(Track->Measurement_noise_cov,Track->x_k_Pre,CoorReal);
							Matrix_Hungarian->data.fl[p] = Peso;
							Hungarian_Matrix[d][f]=Peso;
							if(Matrix_Hungarian->data.fl[p]!=0) id=1;
							p++;

					}

				}


	Matrix_Asignation=Hungaro(Matrix_Hungarian);


	for(int i=0; i < Matrix_Asignation->rows;i++){
			for(int j=0;j < Matrix_Asignation->cols;j++){

					if(Matrix_Asignation->data.fl[v]==1){
					indCandidato=j;

					TrackActual=(STTrack*)obtener(i,lsTraks);
					FlySiguiente=(STFly*)obtener(indCandidato,Flies);
//					TrackActual->Flysig=(STFly*)obtener(indCandidato,Flies);
//					if(TrackSiguiente && TrackActual) enlazarFlies( TrackActual->FlyActual,FlySiguiente);
					if(FlySiguiente && TrackActual){

						TrackActual->Flysig=FlySiguiente;
						anyadirAlFinal(FlySiguiente,TrackActual->Flysig->Tracks);

						enlazarFlies(TrackActual->FlyActual,FlySiguiente);

					}


					}

				v++;
			}



			}

		}

	return 0;

	// enlazar flies

	//	enlazarFlies( Track->FlyActual, STFly* flyActual,float dt, tlcde* ids )

	// rellenar para cada track puntero a fly con la que enlaza Trak->Fliesig ; NULL si no hay

	// rellenar para cada blob vector fly->tracks; Tracks que apuntan a ese blob.


}
// si el ultimo parámetro no es null indica
//int enlazarFlies( STFly* flyAnterior, STFly* flyActual,float dt, tlcde* ids ){
int enlazarFlies( STFly* flyAnterior, STFly* flyActual){
	// si la actual ya habia sido etiquetada dejamos su etiqueta

	flyAnterior->siguiente = (STFly*)flyActual;
	flyActual->etiqueta = flyAnterior->etiqueta;
	flyActual->Color = flyAnterior->Color;

	return 1;
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

double PesosKalman(const CvMat* Matrix,const CvMat* Predict,CvMat* CordReal){

	float Matrix_error_cov[] = {Matrix->data.fl[0], Matrix->data.fl[6]};

	float X = CordReal->data.fl[0];
	float Y = CordReal->data.fl[1];
	float EX = Predict->data.fl[0];
	float EY = Predict->data.fl[1];
	float VarX = sqrt(Matrix_error_cov[0]);
	float VarY = sqrt(Matrix_error_cov[1]);

//	float VarX = sqrt(50);
//	float VarY = sqrt(50);

	double ValorX,ValorY;
	double DIVX,DIVY;
	double ProbKalman;

	ValorX=X-EX;
	ValorY=Y-EY;
	DIVX=-((ValorX/VarX)*(ValorX/VarX))/2;
	DIVY=-((ValorY/VarY)*(ValorY/VarY))/2;

	ProbKalman =exp (-abs(DIVX + DIVY));

	ProbKalman = 100*ProbKalman;

	if (ProbKalman < 1) ProbKalman = 0;

	return ProbKalman;

}

int asignarIdentidades2(  tlcde* lsTraks , tlcde *Flies){

	CvMat* CoorReal;
	STFly* FlyNext=NULL;
	STTrack* Track=NULL;
	CvMat* Matrix_Asignation= NULL;
	STTrack* TrackActual=NULL;
	STFly* FlySiguiente=NULL;
	double Hungarian_Matrix [lsTraks->numeroDeElementos][Flies->numeroDeElementos];
	int v=0;
	int indCandidato; // posicion de la Fly con mayor porbabilidad.


if(lsTraks->numeroDeElementos>0 && Flies->numeroDeElementos >0){

		CvMat* Matrix_Hungarian = cvCreateMat(lsTraks->numeroDeElementos,Flies->numeroDeElementos,CV_32FC1); // Matriz de Pesos
		cvZero(Matrix_Hungarian);

				int p=0;

				for(int d=0;d < lsTraks->numeroDeElementos;d++){

					Track=(STTrack*)obtener(d,lsTraks);

					for(int f=0;f < Flies->numeroDeElementos;f++){

						FlyNext=(STFly*)obtener(f,Flies);

							CoorReal->data.fl[0]=FlyNext->posicion.x;
							CoorReal->data.fl[1]=FlyNext->posicion.y;

							double Peso = PesosKalman(Track->Measurement_noise_cov,Track->x_k_Pre,CoorReal);
							Matrix_Hungarian->data.fl[p] = Peso;
							Hungarian_Matrix[d][f]=Peso;
							p++;
						}
				}

	Matrix_Asignation=Hungaro(Matrix_Hungarian);

	for(int i=0; i < Matrix_Hungarian->rows;i++){
			for(int j=0;j < Matrix_Hungarian->cols;j++){
				if(Matrix_Hungarian->data.fl[v]>70){
					indCandidato=j;

					TrackActual=(STTrack*)obtener(i,lsTraks);
					FlySiguiente=(STFly*)obtener(indCandidato,Flies);
					if(FlySiguiente && TrackActual)	anyadirAlFinal(FlySiguiente,TrackActual->Flysig->Tracks);
//					if(TrackSiguiente && TrackActual) enlazarFlies( TrackActual->FlyActual,FlySiguiente);
//					if(FlySiguiente && TrackActual) FlySiguiente->etiqueta=TrackActual->id;

					}

				v++;
			}
		}

	}

	return 0;

	// enlazar flies

	//	enlazarFlies( Track->FlyActual, STFly* flyActual,float dt, tlcde* ids )

	// rellenar para cada track puntero a fly con la que enlaza Trak->Fliesig ; NULL si no hay

	// rellenar para cada blob vector fly->tracks; Tracks que apuntan a ese blob.


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
