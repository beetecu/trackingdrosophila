/*!
 * BGModel.cpp
 *
 * Modelado de fondo mediante distribución Gaussiana
 *
 *
 *
 *
 *  Created on: 27/06/2011
 *      Author: chao
 */

#include "BGModel.h"

void initBGGModel( CvCapture* t_capture, IplImage* BG,IplImage *DE, IplImage* ImMask,CvRect ROI){

	int num_frames = 0;

	/// Acumulamos el fondo para obtener la mediana y la varianza de cada pixel en 20 frames  ////

	while( num_frames < FRAMES_TRAINING ){
		IplImage* frame = cvQueryFrame( t_capture );
		if ( !frame ) {
			error(2);
			break;
		}
		if ( (cvWaitKey(10) & 255) == 27 ) break;

//		int max_buffer;
//		IplImage* rawImage;

		PreProcesado( frame, ImGray, ImMask, false, ROI);

		accumulateBackground( ImGray, BG ,DE, ROI, 0);

		num_frames += 1;
	}
	return;
}

void accumulateBackground( IplImage* ImGray, IplImage* BGMod,IplImage *Ides,CvRect ROI , IplImage* mask = NULL ) {
	// si la máscara es null, se crea una inicializada a cero, lo que implicará
	// la actualización de todos los pixeles del background
	int flag = 0;
	if (mask == NULL){
		mask = cvCreateImage(cvGetSize( ImGray ), 8, 1 );
		cvZero( mask );
		flag = 1;
	}
	// Se inicializa el fondo con la primera imagen
	static int first = 1; // solo para el modelo inicial
	if ( first == 1 ){
			cvCopy( ImGray, BGMod );
	}

	cvSetImageROI( ImGray, ROI );
	cvSetImageROI( Idiff, ROI );
	cvSetImageROI( mask, ROI );
	cvSetImageROI( BGMod, ROI );
	cvSetImageROI( Ides, ROI );

	// Se estima la mediana

//	cvConvertScale(BGMod ,ImedianF,1,0); // A float
//	cvConvertScale(ImGray ,ImGrayF,1,0); // A float


//	cvConvertScale( ImedianF,BGMod,1,0); // A int
//	cvShowImage( "Foreground",mask);
//	cvWaitKey(0);
	// Se actualiza el fondo usando la máscara.

	for (int y = ImGray->roi->yOffset; y< ImGray->roi->yOffset + ImGray->roi->height; y++){
		uchar* ptr = (uchar*) ( ImGray->imageData + y*ImGray->widthStep + 1*ImGray->roi->xOffset);
		uchar* ptr1 = (uchar*) ( mask->imageData + y*mask->widthStep + 1*mask->roi->xOffset);
		uchar* ptr2 = (uchar*) ( BGMod->imageData + y*BGMod->widthStep + 1*BGMod->roi->xOffset);
		for (int x = 0; x < ImGray->roi->width; x++){
			// Incrementar o decrementar fondo en una unidad
			if ( ptr1[x] == 0 ){ // si el pixel de la mascara es 0 actualizamos
				if ( ptr[x] < ptr2[x] ) ptr2[x] = ptr2[x]-1;
				if ( ptr[x] > ptr2[x] ) ptr2[x] = ptr2[x]+1;
			}
		}
	}

	//Estimamos la desviación típica
	// La primera vez iniciamos la varianza al valor Idiff, que será un valor
	//muy pequeño apropiado para fondos con poco movimiento (unimodales).

	cvAbsDiff( ImGray, BGMod, Idiff);
	if ( first == 1 ){
		cvCopy( Idiff, Ides);
		cvAbsDiff( ImGray, BGMod, Idiff);
		first = 0;
	}
	for (int y = Idiff->roi->yOffset; y< Idiff->roi->yOffset + Idiff->roi->height; y++){
		uchar* ptr1 = (uchar*) ( mask->imageData + y*mask->widthStep + 1*mask->roi->xOffset);
		uchar* ptr3 = (uchar*) ( Idiff->imageData + y*Idiff->widthStep + 1*Idiff->roi->xOffset);
		uchar* ptr4 = (uchar*) ( Ides->imageData + y*Ides->widthStep + 1*Ides->roi->xOffset);
		for (int x= 0; x<Idiff->roi->width; x++){
			if ( ptr1[x] == 0 ){
			// Incrementar o decrementar desviación típica en una unidad
				if ( ptr3[x] < ptr4[x] ) ptr4[x] = ptr4[x]-1;
				if ( ptr3[x] > ptr4[x] ) ptr4[x] = ptr4[x]+1;
			}
		}
	}

	// Corregimos la estimación de la desviación mediante la función de error
	// Así se asegura que la fracción correcta de datos esté dentro de una desvición estándar
	// ( escalado horizontal de la normal.
	cvConvertScale(Ides,Ides,0.7,0);

	cvResetImageROI( ImGray );
	cvResetImageROI( Idiff );
	cvResetImageROI( mask );
	cvResetImageROI( BGMod );
	cvResetImageROI( Ides );

	if (flag == 1) cvReleaseImage( &mask );

}

void UpdateBGModel( IplImage* tmp_frame, IplImage* BGModel,IplImage* DESVI, CvRect DataROI, IplImage* Mask){

	if ( Mask == NULL ) accumulateBackground( tmp_frame, BGModel,DESVI, DataROI , 0);
	else accumulateBackground( tmp_frame, BGModel,DESVI, DataROI ,Mask);

//	RunningBGGModel( tmp_frame, BGModel, Ides, DataROI );
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
void BackgroundDifference( IplImage* ImGray, IplImage* bg_model,IplImage* Ides,IplImage* fg, CvRect dataroi){


	cvSetImageROI( ImGray, dataroi );
	cvSetImageROI( bg_model, dataroi );
	cvSetImageROI( fg, dataroi );
	cvSetImageROI( Idiff, dataroi );
    cvSetImageROI( Ides, dataroi );


	cvZero(fg); // Iniciamos el nuevo primer plano a cero

//	setHighThreshold( bg_model, HIGHT_THRESHOLD );
//	setLowThreshold( bg_model, LOW_THRESHOLD );

//	cvInRange( ImGray, IhiF,IlowF, Imaskt);
	cvAbsDiff( ImGray, bg_model, Idiff);

	for (int y = Idiff->roi->yOffset; y < Idiff->roi->yOffset + Idiff->roi->height; y++){
		uchar* ptr3 = (uchar*) ( Idiff->imageData + y*Idiff->widthStep + 1*Idiff->roi->xOffset);
		uchar* ptr4 = (uchar*) ( Ides->imageData + y*Ides->widthStep + 1*Ides->roi->xOffset);
		uchar* ptr5 =  (uchar*) ( fg->imageData + y*fg->widthStep + 1*fg->roi->xOffset);
		for (int x= 0; x<Idiff->roi->width; x++){
			// Si la desviación tipica del pixel supera en HiF veces la
			// desviación típica del modelo, el pixel se clasifica como
			//foreground ( 255 ), en caso contrario como background
			if ( ptr3[x] > LOW_THRESHOLD*ptr4[x] ) ptr5[x] = 255;
			else ptr5[x] = 0;
		}
	}

#if CREATE_TRACKBARS == 1
		cvCreateTrackbar( "HighT",
						  "Foreground",
						  &HIGHT_THRESHOLD,
						  100  );
		cvCreateTrackbar( "LowT",
						  "Foreground",
						  &LOW_THRESHOLD,
						  100  );
		cvCreateTrackbar( "ALPHA",
						  "Foreground",
						  &g_slider_position,
						  100,
						  onTrackbarSlide );
#endif

	   	cvResetImageROI( ImGray );
		cvResetImageROI( bg_model );
		cvResetImageROI( fg );
		cvResetImageROI( Idiff );
		cvResetImageROI( Ides );

	FGCleanup( fg, Ides );



//	printf(" Alpha = %f\n",ALPHA);
//	cvShowImage( "Foreground",fg);
//	cvWaitKey(0);
}
void FGCleanup( IplImage* FG, IplImage* DES){

	static CvMemStorage* mem_storage = NULL;
	static CvSeq* contours = NULL;

	// Aplicamos morfologia: Erosión y dilatación
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
		int flag = 1;
		double area = cvContourArea( c );
		area = fabs( area );

		if ( (area < MIN_CONTOUR_AREA) || (area > MAX_CONTOUR_AREA) ) {
			flag = 1;
		}
		else{
			// Eliminamos los contornos que no sobrevivan al HIGHT_THRESHOLD
			// Primero obtenermos ROI del contorno del LOW_THRESHOLD
//			cvResetImageROI(Idiff);
//			cvResetImageROI(DES);


			CvRect ContROI = cvBoundingRect( c );
//			cvSetImageROI( Idiff, ContROI );
//			cvSetImageROI( DES , ContROI );

			for (int y = ContROI.y; y< ContROI.y + ContROI.height; y++){
				uchar* ptr3 = (uchar*) ( Idiff->imageData + y*Idiff->widthStep + 1*ContROI.x);
				uchar* ptr4 = (uchar*) ( DES->imageData + y*DES->widthStep + 1*ContROI.x);
				for (int x= 0; x<ContROI.width; x++){
					// Si alguno de los pixeles del blob supera en HiF veces la
					// desviación típica del modelo,desactivamos el flag para no
					// eliminar el contorno
					if ( ptr3[x] > HIGHT_THRESHOLD*ptr4[x] ){
						flag = 0;
						break;
					}
				}
				if (flag == 0) break;
			}
//			cvResetImageROI( Idiff);
//			cvResetImageROI( DES );
		}
		if ( flag == 1 ) {
					cvSubstituteContour( scanner, NULL ); // eliminamos el contorno
		}
		else{ //pasamos al siguiente contorno

			c_new = cvConvexHull2( c, mem_storage, CV_CLOCKWISE, 1); //
			cvSubstituteContour( scanner, c_new );
			numCont++;
		}
	}
	contours = cvEndFindContours( & scanner );
	// Dibujamos los contornos
	cvZero ( FG );
//	IplImage* maskTemp;
	int i = 0;
	for( i = 0, c = contours; c != NULL; c = c->h_next, i++){
		cvDrawContours( FG, c, CVX_WHITE,CVX_WHITE,-1,CV_FILLED,8 );
	}
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









