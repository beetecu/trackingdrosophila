/*
 * Tracking.cpp
 *
 * Cuerpo principal del algoritmo de rastreo.
 *
 *  Created on: 27/06/2011
 *      Author: chao
 */

#include "Tracking.h"
#include "BGModel.h"

using namespace cv;
using namespace std;




int main() {

/////////// CAPTURA DE IMAGENES E INICIALIZACIÓN ////////////

	printf( "Iniciando captura..." );
	InitialTime = (double)cvGetTickCount();
	GlobalTime = (double)cvGetTickCount() - InitialTime;
	printf( " %.1f Seg\n", GlobalTime/(cvGetTickFrequency()*1000000.) );
	g_capture = NULL;
	g_capture = cvCaptureFromAVI( "Drosophila.avi" );
	if ( !g_capture ) {
		error( 1 );
		return -1;
	}
	frame = cvQueryFrame( g_capture );

	// Creación de ventanas de visualizacion
	CreateWindows( );
//	cvSetCaptureProperty( g_capture, CV_CAP_PROP_POS_AVI_RATIO,0 );
	//	Añadimos un slider a la ventana del video Drosophila.avi


	int TotalFrames = (int) cvGetCaptureProperty( g_capture, CV_CAP_PROP_FRAME_COUNT );
//	int	TotalFrames = getAVIFrames("Drosophila.avi"); // en algun linux no funciona lo anterior
//	if ( TotalFrames != 0 ){
//	cvCreateTrackbar( "Posicion",
//				"Drosophila.avi",
//				&g_slider_position,
//				TotalFrames,
//				onTrackbarSlider );
//	}


	//		int TotalFrames = getAVIFrames("Drosophila.avi"); // en algun linux no funciona lo anterior



	// Iniciar estructura para almacenar las Capas

			 //Capa es un puntero a una estructura de tipo STCapas
			// Asignamos espacio a la estructura
	Capa = ( STCapas *) malloc( sizeof( STCapas));

	// Iniciar estructura para almacerar datos de blobs

	STMoscas *mosca=NULL;
	Lista llse; // Apuntará al primer elemento de la lista lineal
	iniciarLista(&llse);


	// Iniciar modelo de fondo

	CvBGStatModel *bg_model = NULL;
	CvGaussBGStatModelParams paramMoG;

//	CvPixelBackgroundGMM* pGMM=0;

	// creación de imagenes a utilizar
	AllocateImages( frame );
			Capa->BGModel = cvCreateImage(cvSize(frame->width,frame->height),8,1);
			Capa->FG = cvCreateImage(cvSize(frame->width,frame->height),8,1);
			Capa->ImFMask = cvCreateImage(cvSize(frame->width,frame->height),8,1);
			Capa->ImRois = cvCreateImage(cvSize(frame->width,frame->height),8,1);
			Capa->OldFG = cvCreateImage(cvSize(frame->width,frame->height),8,1);

	cvSetCaptureProperty( g_capture, CV_CAP_PROP_POS_AVI_RATIO,0 );
	int NumFrame = 0;
////////////////////////////////////////////////
///////// BUCLE PRINCIPAL DEL ALGORITMO ////////

	while ( g_capture ) {

		frame = cvQueryFrame( g_capture );
		if ( !frame ) {
			error(2);
			break;
		}
		if ( (cvWaitKey(10) & 255) == 27 ) break;

//		int TotalFrames = (int) cvGetCaptureProperty( g_capture, CV_CAP_PROP_FRAME_COUNT );
//		TotalFrames += 1 ;
//		cvSetTrackbarPos( "Posicion","Drosophila.avi",TotalFrames);
//		onTrackbarSlider(TotalFrames);
////////// PREPROCESADO ///////////////


		// Obtencion de mascara del plato

		if( !hecho ){
			MascaraPlato( g_capture, Capa->ImFMask, &PCentroX, &PCentroY, &PRadio );
			GlobalTime = (double)cvGetTickCount() - InitialTime;
		    printf( " %.1f Seg\n", GlobalTime/(cvGetTickFrequency()*1000000.) );

//			cvSetCaptureProperty( g_capture, CV_CAP_PROP_POS_AVI_RATIO,0 );
//			NumFrame = 0;
//			TotalFrames -=1;
			if ( PRadio == 0  ) {
				error(3);
				break;
			}
//			hecho = 1;
		}

		// Crear Modelo de fondo estático .Solo en la primera ejecución
//		if (!hecho) {
//				hecho = BGGModel( g_capture , Capa->BGModel, ImHiThr, ImLowThr, Capa->ImFMask);
//				cvSetCaptureProperty( g_capture, CV_CAP_PROP_POS_AVI_RATIO,0 );
//				continue;
//				}
//		// Modelo de fondo para detección de movimiento
		bool update_bg_model = true;
		fr +=1;
		if(!hecho){
			//create BG model
			initBackgroundModel(&bg_model, frame, &paramMoG);
			printf( "Creando modelo de fondo..." );


		//	bg_model = cvCreateGaussianBGModel( frame );
		//    bg_model = cvCreateFGDStatModel( frame );
			GlobalTime = (double)cvGetTickCount() - InitialTime;
			printf( " %.1f Seg\n", GlobalTime/(cvGetTickFrequency()*1000000.) );
			printf( "Identificando objetos ...\n" );
			hecho = 1;
			cvSetCaptureProperty( g_capture, CV_CAP_PROP_POS_AVI_RATIO,0 );
			NumFrame = 0;
			TotalFrames -=1;
			continue;
		 }

		PreProcesado( frame, Imagen, Capa->ImFMask, 1);

//		cvWaitKey(0);
//		invertirBW( Capa->ImFMask);


// Establecer ROI
		ImROI = cvCloneImage( Imagen );
		cvSetImageROI( ImROI, cvRect(PCentroX-PRadio, PCentroY-PRadio, 2* PRadio, 2*PRadio ) );
		//cvAddS( ImROI, cvScalar(0), ImROI);


////////////// PROCESADO ///////////////

		// Segmentacion basada en COLOR


//		 cvPyrMeanShiftFiltering( ImROI, ImROI, spatialRad, colorRad, maxPyrLevel );

// Actualizar fondo
		 double t = (double)cvGetTickCount();
		 Capa-> FG =  updateBackground(bg_model, frame);
		 // Performs FG post-processing using segmentation
		 // (all pixels of a region will be classified as foreground if majority of pixels of the region are FG).
		 // parameters:
		 //      segments - pointer to result of segmentation (for example MeanShiftSegmentation)
		 //      bg_model - pointer to CvBGStatModel structure
//		cvRefineForegroundMaskBySegm( CvSeq* segments, bg_model );
//		 cvUpdateBGStatModel( frame, bg_model, update_bg_model ? -1 : 0 );
		 t = (double)cvGetTickCount() - t;
		 printf( "%d. %.1f ms\n", fr, t/(cvGetTickFrequency()*1000.) );
//		 char k = cvWaitKey(5);
//		 if( k == 27 ) break;
//		 if( k == ' ' )
//		 {
//			 update_bg_model = !update_bg_model;
//			 if(update_bg_model)
//				printf("Background update is on\n");
//			 else
//				printf("Background update is off\n");
//		 }
		 t = (double)cvGetTickCount();
		t = (double)cvGetTickCount() - t; //tiempo transcurrido
	    printf( "Tiempo transcurrido %.1f\n",  t/(cvGetTickFrequency()*1000.) );

/////// SEGMENTACION

// Creacion de capa de blobs
//		 int ok = CreateBlobs( ImROI, ImBlobs, &mosca ,llse );
//		 if (!ok) break;
//		 mosca = (STMoscas *)obtenerPrimero(&llse);
//		 if ( mosca )
//			 printf("Primero: etiqueta: %d area %f ", mosca->etiqueta, mosca->area);
//		 mostrarLista(llse);


// Creación de estructura de datos
//		 printf( "Creando estructura de datos ..." );
//		 GlobalTime = (double)cvGetTickCount() - GlobalTime;
//		 printf( " %.1f\n", GlobalTime/(cvGetTickFrequency()*1000.) );

// Creación de mascara de ROIS para cada objeto
	//	 CreateRois( Imagen, Capa->ImRois);


/////// TRACKING
//		 printf( "Iniciando rastreo ..." ); // añadir tanto por ciento del video analizado
//		 GlobalTime = (double)cvGetTickCount() - GlobalTime;
//		 	printf( " %.1f\n", GlobalTime/(cvGetTickFrequency()*1000.) );
//
//		 printf( "Rastreo finalizado con éxito ..." );
//		 GlobalTime = (double)cvGetTickCount() - GlobalTime;
//		 	printf( " %.1f\n", GlobalTime/(cvGetTickFrequency()*1000.) );



/*		 						*/
/////// VISUALIZACION ////////////
/*								*/


//Obtenemos la Imagen donde se visualizarán los resultados
	    ImVisual = cvCloneImage(frame);

//Dibujamos el plato en la imagen de visualizacion
		cvCircle( ImVisual, cvPoint( PCentroX,PCentroY ), 3, CV_RGB(0,0,0), -1, 8, 0 );
		cvCircle( ImVisual, cvPoint(PCentroX,PCentroY ),PRadio, CV_RGB(0,0,0),2 );
// Dibujamos la ROI
		cvRectangle( ImVisual,cvPoint(PCentroX-PRadio, PCentroY-PRadio),
		cvPoint(PCentroX + PRadio,PCentroY + PRadio), CV_RGB(255,0,0),2);

 //Dibujamos los blobs

//		for( int i = 0; i < blobs.GetNumBlobs(); i++){
//			CurrentBlob = blobs.GetBlob( i );
//			CurrentBlob -> FillBlob( ImBlobs, CVX_RED );
//			CvBox2D elipse = CurrentBlob->GetEllipse();
//		    cvBoxPoints( elipse,pt );
//
//				   cvEllipse(Imagen,cvPoint(cvRound(elipse.center.x),cvRound(elipse.center.y)),
//						   (cvSize(elipse.size.width,elipse.size.height)),
//						   elipse.angle,0,360,CVX_RED,-1, 8, 0);
//		}

// Mostramos imagenes

		cvShowImage( "Drosophila.avi", frame );

//		cvShowImage( "Visualización", ImVisual);
//		cvShowImage( "ROI", ImROI );

//		cvShowImage("BG", bg_model->background);
		cvShowImage("FG", bg_model->foreground);
//		cvShowImage( "Blobs",ImBlobs);
//		cvShowImage("Bina",ImThres);




	}
/////////// OBTENCIÓN DE DATOS ESTADÍSTICOS ////////////
	printf( "Comenzando análisis estadístico ...\n" );
	printf( "Análisis finalizado ...\n" );

	// LIMPIAR MEMORIA

	DeallocateImages( );
	cvReleaseImage( &Capa->BGModel );
	cvReleaseImage( &Capa->FG );
	cvReleaseImage( &Capa->ImFMask );
	cvReleaseImage( &Capa->OldFG );
	cvReleaseImage( &Capa->ImRois);

	free(Capa);
	cvReleaseCapture(&g_capture);
	cvReleaseBGStatModel( &bg_model );

	DestroyWindows( );
}







void AllocateImages( IplImage* I){
	CvSize sz = cvGetSize( I );

	Imagen = cvCreateImage( sz ,8,1);

	ImHiThr= cvCreateImage( sz ,IPL_DEPTH_32F,1);
	ImLowThr= cvCreateImage( sz ,IPL_DEPTH_32F,1);

//    ImGris = cvCreateImage( sz ,8,1);
//	ImFilter = cvCreateImage( sz,8,1);

	ImROI = cvCreateImage( sz, 8, 1);
	ImBlobs = cvCreateImage( sz,8,1 );
	ImThres = cvCreateImage( sz,8,1 );
	ImVisual = cvCreateImage( sz,8,1);


}

// Libera memoria de las imagenes creadas

void DeallocateImages( ){

//	cvReleaseImage( &ImGris );
//	cvReleaseImage( &ImFilter );
	cvReleaseImage( &Imagen );
	cvReleaseImage( &ImROI );

	cvReleaseImage( &ImHiThr );
    cvReleaseImage( &ImLowThr );

	cvReleaseImage( &ImBlobs );
	cvReleaseImage( &ImVisual );


}

// Creación de ventanas
void CreateWindows( ){

#if SHOW_BG_REMOVAL == 1
	cvNamedWindow( "BG",CV_WINDOW_AUTOSIZE);
	cvNamedWindow( "FG",CV_WINDOW_AUTOSIZE);
    cvMoveWindow("BG", 350, 500 );
    cvMoveWindow("FG", 10, 500);
#endif
	cvNamedWindow( "Drosophila.avi", CV_WINDOW_AUTOSIZE );
	cvNamedWindow( "Visualización",CV_WINDOW_AUTOSIZE);
	cvNamedWindow( "Imagen", CV_WINDOW_AUTOSIZE);
	cvNamedWindow( "ROI", CV_WINDOW_AUTOSIZE);

	cvNamedWindow("Bina",1);
	cvNamedWindow("Blobs",1);
}


// Destruccion de ventanas
void DestroyWindows( ){
	cvDestroyWindow( "Drosophila.avi" );
	cvDestroyWindow( "Visualización" );
	cvDestroyWindow( "Imagen" );
	cvDestroyWindow( "ROI" );
#if SHOW_BG_REMOVAL == 1
	cvDestroyWindow( "BG");
	cvDestroyWindow( "FG");
#endif
	cvDestroyWindow( "BG" );
	cvDestroyWindow( "FG" );
	cvDestroyWindow( "Bina" );
	cvDestroyWindow( "Blobs" );
}

void onTrackbarSlider(  int pos ){

	cvSetCaptureProperty( g_capture, CV_CAP_PROP_POS_FRAMES, pos );
}

int getAVIFrames(char * fname) {
    char tempSize[4];
    // Trying to open the video file
    ifstream  videoFile( fname , ios::in | ios::binary );
    // Checking the availablity of the file
    if ( !videoFile ) {
                cout << "Couldn’t open the input file " << fname << endl;
                exit( 1 );
    }
    // get the number of frames
    videoFile.seekg( 0x30 , ios::beg );
    videoFile.read( tempSize , 4 );
    int frames = (unsigned char ) tempSize[0] + 0x100*(unsigned char ) tempSize[1] + 0x10000*(unsigned char ) tempSize[2] +    0x1000000*(unsigned char ) tempSize[3];
    videoFile.close(  );
    return frames;
}

void mostrarLista(Lista *lista)
{
  // Mostrar todos los elementos de la lista

	int i = 0;
	STMoscas *Mosca = NULL;

	while (i < lista->numeroDeElementos)
	{
		Mosca = (STMoscas *)obtener(i, lista);
		printf("\n Vertical : %f, Horizontal: %f, Punto : %f",Mosca->VV,Mosca->VH,Mosca->punto1.x );
		i++;
	}
}
