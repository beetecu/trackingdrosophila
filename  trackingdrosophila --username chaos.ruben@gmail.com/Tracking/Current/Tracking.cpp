/*!
 * Tracking.cpp
 *
 * Cuerpo principal del algoritmo de rastreo.
 *
 *  Created on: 27/06/2011
 *      Author: chao
 */

#include "Tracking.h"
#include "segmentacion.h"

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
	//      cvSetCaptureProperty( g_capture	cvResetImageROI(Capa->BGModel);, CV_CAP_PROP_POS_AVI_RATIO,0 );
	//      Añadimos un slider a la ventana del video Drosophila.avi
	//              int TotalFrames = getAVIFrames("Drosophila.avi"); // en algun linux no funciona lo anterior

	// Iniciar estructura para almacenar las Capas
	Capa = ( STCapas *) malloc( sizeof( STCapas));

	// Iniciar estructura para almacerar datos de blobs
	STMoscas *mosca=NULL;
	Lista llse; // Apuntará al primer elemento de la lista lineal
	iniciarLista(&llse);

	// creación de imagenes a utilizar
	AllocateImages( frame );
	AllocateImagesBGM( frame );

	cvSetCaptureProperty( g_capture, CV_CAP_PROP_POS_AVI_RATIO,0 );

	int FrameCount = 0; // contador de frames para la actualización del fondo

	///////////////////////\\\\\\\\\\\\\\\\\\\\\\\
	 ////// BUCLE PRINCIPAL DEL ALGORITMO \\\\\\\
	  /////////////////////\\\\\\\\\\\\\\\\\\\\

	while ( g_capture ) {

		frame = cvQueryFrame( g_capture );
		if ( !frame ) {
			error(2);
			break;
		}
		if ( (cvWaitKey(10) & 255) == 27 ) break;

		////////// PREPROCESADO ///////////////

		// Obtencion de mascara del plato
		if( !hecho ){
	//		static int num_iter = 0;
			MascaraPlato( g_capture, Capa->ImFMask, &PCentroX, &PCentroY, &PRadio );
			GlobalTime = (double)cvGetTickCount() - InitialTime;
			printf( " %.1f Seg\n", GlobalTime/(cvGetTickFrequency()*1000000.) );
			if ( PRadio == 0  ) {
				error(3);
				break;
			}
//			num_iter +=1;
			DataFROI = cvRect(PCentroX-PRadio, PCentroY-PRadio, 2* PRadio, 2*PRadio ); // Datos para establecer ROI del plato
		}

		// Crear Modelo de fondo estático .Solo en la primera ejecución
		if (!hecho) {
			hecho = initBGGModel( g_capture , Capa->BGModel,Capa->IDesv, Capa->ImFMask);
			//cvSetCaptureProperty( g_capture, CV_CAP_PROP_POS_AVI_RATIO,0 );
			//continue;
		}
		// Modelo de fondo para detección de movimiento

		PreProcesado( frame, Imagen, Capa->ImFMask, 0);

		// Establecer ROI

		cvSetImageROI( Imagen, DataFROI );
//		cvShowImage( "Region_Of_Interest", Imagen );
		cvResetImageROI(Imagen);

		////////////// PROCESADO ///////////////

		cvCopy( Capa->BGModel, BGTemp);
		cvCopy(Capa->IDesv,DETemp);

		double t = (double)cvGetTickCount();
		// Primera actualización del fondo
		FrameCount += 1;

		if ( FrameCount == BGUpdate ){
			UpdateBGModel( Imagen, Capa->BGModel,Capa->IDesv, DataFROI, 0 );
			FrameCount = 0;
		}
//		cvShowImage( "Foreground",Capa->BGModel);
		cvCreateTrackbar( "BGUpdate",
						  "Foreground",
						  &BGUpdate,
						  100  );
		// Resta de fondo. Obtención de la máscara del foreground
		BackgroundDifference( Imagen, Capa->BGModel,Capa->IDesv, Capa->FG , DataFROI);

		t = (double)cvGetTickCount() - t;
		printf( "%d. %.1f ms\r", fr, t/(cvGetTickFrequency()*1000.) );

		// Actualizamos el fondo haciendo uso de la máscara del foreground
		if ( FrameCount == 0 ){
					UpdateBGModel( Imagen, BGTemp,DETemp, DataFROI, Capa->FG );
		}
		cvCopy( BGTemp, Capa->BGModel);
		cvCopy( DETemp, Capa->IDesv);
		BackgroundDifference( Imagen, Capa->BGModel,Capa->IDesv, Capa->FG , DataFROI);
//		IplImage *Imtemp = cvCreateImage( cvGetSize(Imagen),8,1);
//		cvAbsDiff( BGTemp, Capa->BGModel, Imtemp);
//
//			for (int y = 0; y< Imtemp->height; y++){
//				uchar* ptr3 = (uchar*) (Imtemp->imageData + y*Imtemp->widthStep);
//
//				for (int x= 0; x<Imtemp->width; x++){
//					// Si la desviación tipica del pixel supera en HiF veces la
//					// desviación típica del modelo, el pixel se clasifica como
//					//foreground ( 255 ), en caso contrario como background
//					if ( ptr3[x] > 0 ) ptr3[x] = 255;
//					else ptr3[x] = 0;
//				}
//			}
//		cvShowImage("Foreground", Imtemp);
//		cvWaitKey(0);



		// Performs FG post-processing using segmentation
		// (all pixels of a region will be classified as foreground if majority of pixels of the region are FG).
		// parameters:
		//      segments - pointer to result of segmentation (for example MeanShiftSegmentation)
		//      bg_model - pointer to CvBGStatModel structure
		//              cvRefineForegroundMaskBySegm( CvSeq* segments, bg_model );
		// Segmentacion basada en COLOR
//		 cvPyrMeanShiftFiltering( ImROI, ImROI, spatialRad, colorRad, maxPyrLevel );

		/////// SEGMENTACION

//		segmentacion(Imagen,Capa->BGModel,Capa->IDesv);

		// Creacion de capa de blobs
		//               int ok = CreateBlobs( ImROI, ImBlobs, &mosca ,llse );
		//               if (!ok) break;
		//               mosca = (STMoscas *)obtenerPrimero(&llse);
		//               if ( mosca )
		//                       printf("Primero: etiqueta: %d area %f ", mosca->etiqueta, mosca->area);
		//               mostrarLista(llse);


		// Creación de estructura de datos
		//               printf( "Creando estructura de datos ..." );
		//               GlobalTime = (double)cvGetTickCount() - GlobalTime;
		//               printf( " %.1f\n", GlobalTime/(cvGetTickFrequency()*1000.) );

		// Creación de mascara de ROIS para cada objeto
		//       CreateRois( Imagen, Capa->ImRois);


		/////// TRACKING
		//               printf( "Iniciando rastreo ..." ); // añadir tanto por ciento del video analizado
		//               GlobalTime = (double)cvGetTickCount() - GlobalTime;
		//                      printf( " %.1f\n", GlobalTime/(cvGetTickFrequency()*1000.) );
		//
		//               printf( "Rastreo finalizado con éxito ..." );
		//               GlobalTime = (double)cvGetTickCount() - GlobalTime;
		//                      printf( " %.1f\n", GlobalTime/(cvGetTickFrequency()*1000.) );



		/*                                                              */
		/////// VISUALIZACION ////////////
		/*                                                              */

#if SHOW_VISUALIZATION == 1
		//Obtenemos la Imagen donde se visualizarán los resultados
		ImVisual = cvCloneImage(frame);

		//Dibujamos el plato en la imagen de visualizacion
		cvCircle( ImVisual, cvPoint( PCentroX,PCentroY ), 3, CV_RGB(0,0,0), -1, 8, 0 );
		cvCircle( ImVisual, cvPoint(PCentroX,PCentroY ),PRadio, CV_RGB(0,0,0),2 );
		// Dibujamos la ROI
		cvRectangle( ImVisual,cvPoint(PCentroX-PRadio, PCentroY-PRadio),
				cvPoint(PCentroX + PRadio,PCentroY + PRadio), CV_RGB(255,0,0),2);

		//Dibujamos los blobs

		//              for( int i = 0; i < blobs.GetNumBlobs(); i++){
		//                      CurrentBlob = blobs.GetBlob( i );
		//                      CurrentBlob -> FillBlob( ImBlobs, CVX_RED );
		//                      CvBox2D elipse = CurrentBlob->GetEllipse();
		//                  cvBoxPoints( elipse,pt );
		//
		//                                 cvEllipse(Imagen,cvPoint(cvRound(elipse.center.x),cvRound(elipse.center.y)),
		//                                                 (cvSize(elipse.size.width,elipse.size.height)),
		//                                                 elipse.angle,0,360,CVX_RED,-1, 8, 0);
		//              }
		//                cvShowImage( "Visualización", ImVisual);

#endif
		// Mostramos imagenes

		cvShowImage( "Drosophila.avi", frame );
		//

		cvShowImage("Background", Capa->BGModel);
//		cvShowImage("FG", bg_model->foreground);
		//              cvShowImage( "Blobs",ImBlobs);
		//              cvShowImage("Bina",ImThres);
		cvShowImage( "Foreground",Capa->FG);
//		cvWaitKey(0);



	}

	///////////////////////\\\\\\\\\\\\\\\\\\\\\\\
	 ////////// ANALISIS ESTADÍSTICO \\\\\\\\\\\\
	  /////////////////////\\\\\\\\\\\\\\\\\\\\\

	printf( "Comenzando análisis estadístico ...\n" );
	printf( "Análisis finalizado ...\n" );

	// LIMPIAR MEMORIA

	free(Capa);
	DeallocateImages( );
	DeallocateImagesBGM();



	cvReleaseCapture(&g_capture);


	DestroyWindows( );
}

void AllocateImages( IplImage* I ){


	CvSize sz = cvGetSize( I );


	Capa->BGModel = cvCreateImage(sz,8,1);
	Capa->FG = cvCreateImage(sz,8,1);
	Capa->IDesv = cvCreateImage(sz,8,1);
	Capa->ImFMask = cvCreateImage(sz,8,1);
	Capa->ImRois = cvCreateImage(sz,8,1);
	Capa->OldFG = cvCreateImage(sz,8,1);

	BGTemp = cvCreateImage( sz,8,1);
	DETemp = cvCreateImage( sz,8,1);
	Imagen = cvCreateImage( sz ,8,1);

	ImBlobs = cvCreateImage( sz,8,1 );
	ImThres = cvCreateImage( sz,8,1 );
	ImVisual = cvCreateImage( sz,8,1);


}

// Libera memoria de las imagenes creadas

void DeallocateImages( ){

	cvReleaseImage( &Imagen );
	cvReleaseImage( &ImBlobs );
	cvReleaseImage( &ImVisual );
	cvReleaseImage( &BGTemp);
	cvReleaseImage( &DETemp);


}

// Creación de ventanas
void CreateWindows( ){

	cvNamedWindow( "Drosophila.avi", CV_WINDOW_AUTOSIZE );
#if SHOW_BG_REMOVAL == 1
	cvNamedWindow( "Background",CV_WINDOW_AUTOSIZE);
	cvNamedWindow( "Foreground",CV_WINDOW_AUTOSIZE);

	cvMoveWindow("Background", 0, 0 );
	cvMoveWindow("Foreground", 640, 0);
#endif

	//        cvNamedWindow( "Visualización",CV_WINDOW_AUTOSIZE);
	//        cvNamedWindow( "Imagen", CV_WINDOW_AUTOSIZE);
//	cvNamedWindow( "Region_Of_Interest", CV_WINDOW_AUTOSIZE);

	//        cvNamedWindow("Bina",1);
	//        cvNamedWindow("Blobs",1);
}


// Destruccion de ventanas
void DestroyWindows( ){
	cvDestroyWindow( "Drosophila.avi" );
	//        cvDestroyWindow( "Visualización" );
	//        cvDestroyWindow( "Imagen" );
//	cvDestroyWindow( "Region_Of_Interest" );
#if SHOW_BG_REMOVAL == 1
	cvDestroyWindow( "Background");
	cvDestroyWindow( "Foreground");
#endif
	//        cvDestroyWindow( "Bina" );
	//        cvDestroyWindow( "Blobs" );
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
