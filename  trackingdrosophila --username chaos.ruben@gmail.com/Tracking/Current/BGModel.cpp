/*!
 * BGModel.cpp
 *
 * Modelado de fondo mediante distribución Gaussiana
 *
 * Recibe un puntero tipo cv_Capture a la secuencia a modelar
 * Devuelve un puntero IplImage al modelo de fondo
 *
 *  Created on: 27/06/2011
 *      Author: chao
 */

#include "BGModel.h"

int initBGGModel( CvCapture* t_capture, IplImage* BG, IplImage* ImMask){

	int num_frames = 0;

	/// Acumulamos el fondo para obtener la mediana y la varianza de cada pixel en 20 frames  ////

	printf(" Creando modelo de fondo...\n");
	while( num_frames < FRAMES_TRAINING ){
		IplImage* frame = cvQueryFrame( t_capture );
		if ( !frame ) {
			error(2);
			break;
		}
		if ( (cvWaitKey(10) & 255) == 27 ) break;

		PreProcesado( frame, ImGray, ImMask, false);

		accumulateBackground( ImGray, BG );

		num_frames += 1;
	}
	return 1;
}

void accumulateBackground( IplImage* ImGray, IplImage* BGMod ) {

	static int first = 1;
	// Estimamos la mediana
	// Inicializamos el fondo con la primera imagen
	if ( first == 1 ){
			cvCopy( ImGray, BGMod );
			first = 0;
		}
	for (int y = 0; y< ImGray->height; y++){
		uchar* ptr = (uchar*) ( ImGray->imageData + y*ImGray->widthStep);
		uchar* ptr2 = (uchar*) ( BGMod->imageData + y*BGMod->widthStep);
		for (int x= 0; x<ImGray->width; x++){
			// Incrementar o decrementar fondo en una unidad
			if ( ptr[x] < ptr2[x] ) ptr2[x] = ptr2[x]-1;
			if ( ptr[x] > ptr2[x] ) ptr2[x] = ptr2[x]+1;
		}
	}
	cvAbsDiff( ImGray, BGMod, IdiffF);
	//Estimamos la desviación típica
	// La primera vez iniciamos la varianza al valor IdiffF, que será un valor
	//muy pequeño apropiado para fondos con poco movimiento (unimodales).
	if ( first == 1 ){
		cvCopy( IdiffF, IvarF);
		first = 0;
	}
	else {
		for (int y = 0; y< IdiffF->height; y++){
			uchar* ptr3 = (uchar*) ( IdiffF->imageData + y*IdiffF->widthStep);
			uchar* ptr4 = (uchar*) ( IvarF->imageData + y*IvarF->widthStep);
			for (int x= 0; x<IdiffF->width; x++){
				// Incrementar o decrementar desviación típica en una unidad
				if ( ptr3[x] < ptr4[x] ) ptr4[x] = ptr4[x]-1;
				if ( ptr3[x] > ptr4[x] ) ptr4[x] = ptr4[x]+1;
			}
		}
	}
	// Corregimos la estimación de la desviación mediante la función de error
//	cvConvertScale(IvarF,IvarF,1.4826,0);
}

void UpdateBackground( IplImage* tmp_frame, IplImage* bg_model, CvRect DataROI){

	accumulateBackground( tmp_frame, bg_model );
	RunningBGGModel( tmp_frame, bg_model, IvarF, ALPHA, DataROI );
//	RunningVariance

}
void RunningBGGModel( IplImage* Image, IplImage* median, IplImage* Idesvt, double alpha, CvRect dataroi ){

	IplImage* ImTemp;
	CvSize sz = cvGetSize( Image );
	ImTemp = cvCreateImage( sz, 8, 1);

	cvCopy( Image, ImTemp);

	cvSetImageROI( ImTemp, dataroi );
	cvSetImageROI( median, dataroi );
	cvSetImageROI( IdiffF, dataroi );
	cvSetImageROI( Idesvt, dataroi );b

	// Para la mediana
	cvConvertScale(ImTemp, ImTemp, alpha,0);
	cvConvertScale( median, median, (1 - alpha),0);
	cvAdd(ImTemp , median , median );

	// Para la desviación típica
	cvConvertScale(IdiffF, ImTemp, alpha,0);
    cvConvertScale( Idesvt, Idesvt, (1 - alpha),0);
	cvAdd(ImTemp , Idesvt , Idesvt );

	cvResetImageROI( median );
	cvResetImageROI( IdiffF );
	cvResetImageROI( Idesvt );

	cvReleaseImage( &ImTemp );
}
void BackgroundDifference( IplImage* ImGray, IplImage* bg_model,
		IplImage* fg,  int HiF, int LowF ){

	cvZero(fg); // Iniciamos el nuevo primer plano a cero

	setHighThreshold( bg_model, HiF );
	setLowThreshold( bg_model, LowF );

//	cvInRange( ImGray, IhiF,IlowF, Imaskt);

//	cvOr( fg, Imaskt, fg);

	for (int y = 0; y< IdiffF->height; y++){
		uchar* ptr3 = (uchar*) ( IdiffF->imageData + y*IdiffF->widthStep);
		uchar* ptr4 = (uchar*) ( IvarF->imageData + y*IvarF->widthStep);
		uchar* ptr5 =  (uchar*) ( fg->imageData + y*fg->widthStep);
		for (int x= 0; x<IdiffF->width; x++){
			// Si la desviación tipica del pixel supera en HiF veces la
			// desviación típica del modelo, el pixel se clasifica como
			//foreground ( 255 ), en caso contrario como background
			if ( ptr3[x] >= HiF*ptr4[x] ) ptr5[x] = 255;
			else ptr5[x] = 0;
		}
	}
	invertirBW( fg );
		cvShowImage( "Foreground",fg);
}
// Establece el Umbral alto como la mediana mas HiF veces la desviación típica
void setHighThreshold( IplImage* BG, int HiF ){
	cvConvertScale( IvarF, IhiF, HiF,0 );
//	cvAdd(BG, IhiF, IhiF);
//	cvShowImage( "Foreground",IhiF);

}
// Establece el Umbral bajo como la mediana menos LowF veces la desviación típica
void setLowThreshold( IplImage* BG, int LowF ){
	cvConvertScale( IvarF, IlowF, LowF,0 );
//	cvSub(BG, IlowF, IlowF);
//	cvShowImage( "Foreground",IlowF);
}
void AllocateImagesBGM( IplImage *I ) {  // I is just a sample for allocation purposes

        CvSize sz = cvGetSize( I );

        IvarF = cvCreateImage( sz, 8, 1 );
        IdiffF = cvCreateImage( sz, 8, 1 );
        IhiF = cvCreateImage( sz, 8, 1 );
        IlowF = cvCreateImage( sz, 8, 1 );

        cvZero( IvarF );
        cvZero( IdiffF );
        cvZero( IhiF );
        cvZero( IlowF );

        ImGray = cvCreateImage( sz, 8, 1 );
        Imaskt = cvCreateImage( sz, 8, 1 );
}

void DeallocateImagesBGM() {

        cvReleaseImage( &IvarF );
        cvReleaseImage( &IdiffF );
        cvReleaseImage( &IhiF );
        cvReleaseImage( &IlowF );
        cvReleaseImage( &Imaskt );
        cvReleaseImage( &ImGray );
}









