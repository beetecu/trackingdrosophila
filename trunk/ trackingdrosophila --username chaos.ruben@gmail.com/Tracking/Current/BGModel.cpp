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

		int max_buffer;
		IplImage* rawImage;

		PreProcesado( frame, ImGray, ImMask, false);

		accumulateBackground( ImGray, BG );

		num_frames += 1;
	}
	return 1;
}

void accumulateBackground( IplImage* ImGray, IplImage* BGMod ) {

	static int first = 1;
	// Estimamos la mediana

	cvConvertScale(BGMod ,ImedianF,1,0); // A float
	cvConvertScale(ImGray ,ImGrayF,1,0); // A float

	// Inicializamos el fondo con la primera imagen
	if ( first == 1 ){
			cvCopy( ImGray, BGMod );
			first = 0;
		}
//	cvConvertScale( ImedianF,BGMod,1,0); // A int
//	cvShowImage( "Foreground",BGMod);
//	cvWaitKey(0);
	for (int y = 0; y< ImGray->height; y++){
		uchar* ptr = (uchar*) ( ImGray->imageData + y*ImGray->widthStep);
		uchar* ptr2 = (uchar*) ( BGMod->imageData + y*BGMod->widthStep);
		for (int x= 0; x<ImGray->width; x++){
			// Incrementar o decrementar fondo en una unidad
			if ( ptr[x] < ptr2[x] ) ptr2[x] = ptr2[x]-1;
			if ( ptr[x] > ptr2[x] ) ptr2[x] = ptr2[x]+1;
			if ( x == 300 && y == 200 ) printf( "Pixel Mediana: %d\n", ptr2[x]);
		}
	}
	cvAbsDiff( ImGray, BGMod, Idiff);
	//Estimamos la desviación típica
	// La primera vez iniciamos la varianza al valor Idiff, que será un valor
	//muy pequeño apropiado para fondos con poco movimiento (unimodales).
	if ( first == 1 ){
		cvCopy( Idiff, Ides);
		first = 0;
	}
	else {
		for (int y = 0; y< Idiff->height; y++){
			uchar* ptr3 = (uchar*) ( Idiff->imageData + y*Idiff->widthStep);
			uchar* ptr4 = (uchar*) ( Ides->imageData + y*Ides->widthStep);
			for (int x= 0; x<Idiff->width; x++){
				// Incrementar o decrementar desviación típica en una unidad
				if ( ptr3[x] < ptr4[x] ) ptr4[x] = ptr4[x]-1;
				if ( ptr3[x] > ptr4[x] ) ptr4[x] = ptr4[x]+1;
				if ( x == 300 && y == 200 ) printf( "Pixel varianza: %d\n", ptr4[x]);
			}
		}
	}

//	cvShowImage( "Foreground",fg);
	// Corregimos la estimación de la desviación mediante la función de error
//	cvConvertScale(Ides,Ides,1.4826,0);

//	cvConvertScale( ImedianF,BGMod,1,0); // A int
//	cvConvertScale(ImGrayF ,ImGray,1,0); // A int

}

void UpdateBGModel( IplImage* tmp_frame, STCapas* Cap, CvRect DataROI){

	accumulateBackground( tmp_frame, Cap->BGModel );
//	RunningBGGModel( tmp_frame, bg_model, Ides, DataROI );
//	RunningVariance

}
void RunningBGGModel( IplImage* Image, IplImage* median, IplImage* Idest, CvRect dataroi ){

	IplImage* ImTemp;
	CvSize sz = cvGetSize( Image );
	ImTemp = cvCreateImage( sz, 8, 1);

	cvCopy( Image, ImTemp);

	cvSetImageROI( ImTemp, dataroi );
	cvSetImageROI( median, dataroi );
	cvSetImageROI( Idiff, dataroi );
	cvSetImageROI( Idest, dataroi );

	// Para la mediana
	cvConvertScale(ImTemp, ImTemp, ALPHA,0);
	cvConvertScale( median, median, (1 - ALPHA),0);
	cvAdd(ImTemp , median , median );

	// Para la desviación típica
	cvConvertScale(Idiff, ImTemp, ALPHA,0);
    cvConvertScale( Idest, Idest, (1 - ALPHA),0);
	cvAdd(ImTemp , Idest , Idest );

	cvResetImageROI( median );
	cvResetImageROI( Idiff );
	cvResetImageROI( Idest );

	cvReleaseImage( &ImTemp );

}
void BackgroundDifference( IplImage* ImGray, IplImage* bg_model,IplImage* fg ){

	cvZero(fg); // Iniciamos el nuevo primer plano a cero

	setHighThreshold( bg_model, HIGHT_THRESHOLD );
//	setLowThreshold( bg_model, LOW_THRESHOLD );

//	cvInRange( ImGray, IhiF,IlowF, Imaskt);
	cvAbsDiff( ImGray, bg_model, Idiff);
//	cvOr( fg, Imaskt, fg);

	for (int y = 0; y< Idiff->height; y++){
		uchar* ptr3 = (uchar*) ( Idiff->imageData + y*Idiff->widthStep);
		uchar* ptr4 = (uchar*) ( Ides->imageData + y*Ides->widthStep);
		uchar* ptr5 =  (uchar*) ( fg->imageData + y*fg->widthStep);
		for (int x= 0; x<Idiff->width; x++){
			// Si la desviación tipica del pixel supera en HiF veces la
			// desviación típica del modelo, el pixel se clasifica como
			//foreground ( 255 ), en caso contrario como background
			if ( ptr3[x] > HIGHT_THRESHOLD*ptr4[x] ) ptr5[x] = 255;
			else ptr5[x] = 0;
		}
	}
	FGCleanup( fg );
//	invertirBW( fg );
//cvAnd(Ides, Ides, IvarF);
//
//for (int y = 0; y< Idiff->height; y++){
//
//		uchar* ptr4 = (uchar*) ( Ides->imageData + y*Ides->widthStep);
//		uchar* ptr5 =  (uchar*) ( IvarF->imageData + y*IvarF->widthStep);
//
//		for (int x= 0; x<Idiff->width; x++){
//			// Si la desviación tipica del pixel supera en HiF veces la
//			// desviación típica del modelo, el pixel se clasifica como
//			//foreground ( 255 ), en caso contrario como background
//			if ( x== 300 && y == 200) printf( "Ides = %d, IvarF = %d\n",ptr4[x],ptr5[x]);
//		}
//	}





	cvCreateTrackbar( "HighT",
				  "Foreground",
				  &HIGHT_THRESHOLD,
				  100  );
	cvCreateTrackbar( "ALPHA",
						  "Foreground",
						  &g_slider_position,
						  100,
						  onTrackbarSlide );
//	printf(" Alpha = %f\n",ALPHA);
	cvShowImage( "Foreground",fg);
}
void FGCleanup( IplImage* FG){

	static CvMemStorage* mem_storage = NULL;
	static CvSeq* contours = NULL;

	// Aplicamos morflogia: Erosión y dilatación
	IplConvKernel* KernelMorph = cvCreateStructuringElementEx(3, 3, 0, 0, CV_SHAPE_ELLIPSE, NULL);

	cvMorphologyEx( FG, FG, 0, KernelMorph, CV_MOP_OPEN , CVCLOSE_ITR);
	cvMorphologyEx( FG, FG, 0, KernelMorph, CV_MOP_CLOSE, CVCLOSE_ITR );
	cvReleaseStructuringElement( &KernelMorph );

	// Buscamos los contornos cuya area se encuentre en un rango determinado
	if( mem_storage == NULL ){
		mem_storage = cvCreateMemStorage(0);
	} else {
		cvClearMemStorage( mem_storage);
	}
	CvContourScanner scanner = cvStartFindContours(
			FG, mem_storage, sizeof(CvContour),CV_RETR_EXTERNAL, CV_CHAIN_APPROX_SIMPLE );
	CvSeq* c;

	int numCont = 0;
	while( (c = cvFindNextContour( scanner )) != NULL ) {
		CvSeq* c_new;
		double area = cvContourArea( c );
		area = fabs( area );

		if ( (area < MIN_CONTOUR_AREA) || (area > MAX_CONTOUR_AREA) ) {
			cvSubstituteContour( scanner, NULL ); // eliminamos el contorno
		}else{

			c_new = cvConvexHull2( c, mem_storage, CV_CLOCKWISE, 1);

		cvSubstituteContour( scanner, c_new );
		numCont++;
		}
	}
	contours = cvEndFindContours( & scanner );
	// Dibujamos los contornos
	cvZero ( FG );
	IplImage* maskTemp;
	int i = 0;
	for( i = 0, c = contours; c != NULL; c = c->h_next, i++){
		cvDrawContours( FG, c, CVX_WHITE,CVX_WHITE,-1,CV_FILLED,8 );
	}


}
// Establece el Umbral alto como la mediana mas HiF veces la desviación típica
void setHighThreshold( IplImage* BG, int HiF ){
	cvConvertScale( Ides, IhiF, HiF,0 );
//	cvAdd(BG, IhiF, IhiF);
//	cvShowImage( "Foreground",IhiF);

}
// Establece el Umbral bajo como la mediana menos LowF veces la desviación típica
void setLowThreshold( IplImage* BG, int LowF ){
	cvConvertScale( Ides, IlowF, LowF,0 );
//	cvSub(BG, IlowF, IlowF);
//	cvShowImage( "Foreground",IlowF);
}

void onTrackbarSlide(int pos) {
   ALPHA = pos / 100;
}
void AllocateImagesBGM( IplImage *I ) {  // I is just a sample for allocation purposes

        CvSize sz = cvGetSize( I );
        ImedianF = cvCreateImage( sz, IPL_DEPTH_32F, 1 );
        IvarF = cvCreateImage( sz, 8, 1 );
        Ivar =  cvCreateImage( sz, 8, 1 );
        IdesF = cvCreateImage( sz, IPL_DEPTH_32F, 1 );
        Ides = cvCreateImage( sz, 8, 1 );
        IdiffF = cvCreateImage( sz, IPL_DEPTH_32F, 1 );
        Idiff = cvCreateImage( sz, 8, 1 );
        IhiF = cvCreateImage( sz, 8, 1 );
        IlowF = cvCreateImage( sz, 8, 1 );

        cvZero( ImedianF );
        cvZero( IvarF );
        cvZero( IdesF );
        cvZero( Ides );
        cvZero( IdiffF );
        cvZero( Idiff );
        cvZero( IhiF );
        cvZero( IlowF );

        ImGray = cvCreateImage( sz, 8, 1 );
        ImGrayF = cvCreateImage( sz, IPL_DEPTH_32F, 1 );
        Imaskt = cvCreateImage( sz, 8, 1 );
}

void DeallocateImagesBGM() {

		cvReleaseImage( &ImedianF );

        cvReleaseImage( &IvarF );
        cvReleaseImage( &Ivar );

        cvReleaseImage( &IdesF );
        cvReleaseImage( &Ides );

        cvReleaseImage( &IdiffF );
        cvReleaseImage( &Idiff );

        cvReleaseImage( &IhiF );
        cvReleaseImage( &IlowF );
        cvReleaseImage( &Imaskt );
        cvReleaseImage( &ImGray );
        cvReleaseImage( &ImGrayF );
}








