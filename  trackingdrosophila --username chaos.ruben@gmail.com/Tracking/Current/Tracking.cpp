/*!
 * Tracking.cpp
 *
 * Cuerpo principal del algoritmo de rastreo.
 *
 *  Created on: 27/06/2011
 *      Author: chao
 */

#include "VideoTracker.hpp"
#include "BGModel.h"
#include "Tracking.h"
#include "segmentacion.h"
#include "ShapeModel.hpp"

using namespace cv;
using namespace std;

int main() {

	/////////// CAPTURA DE IMAGENES E INICIALIZACIÓN ////////////
	gettimeofday(&ti, NULL);  //para obtener el tiempo
	TiempoInicial= ti.tv_sec*1000 + ti.tv_usec/1000.0;

	printf( "Iniciando captura..." );
	gettimeofday(&tf, NULL);
	TiempoParcial= (tf.tv_sec - ti.tv_sec)*1000 + \
			(tf.tv_usec - ti.tv_usec)/1000.0;
	printf(" %5.4g ms\n", TiempoParcial);

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
	TotalFrames = cvGetCaptureProperty( g_capture, CV_CAP_PROP_FRAME_COUNT);

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
		FrameCount += 1;
		printf( "\n\t\t\tFRAME %.0f\n", FrameCount);
		UpdateCount += 1;
		gettimeofday(&tif, NULL);

		////////// PREPROCESADO ///////////////
		static int hecho = 0;
		// Obtencion de mascara del plato
		if( !hecho ){
			printf("Localizando plato... ");
			gettimeofday(&ti, NULL);

			MascaraPlato( g_capture, Capa->ImFMask, &PCentroX, &PCentroY, &PRadio );

			if ( PRadio == 0  ) {
				error(3);
				break;
			}
			DataFROI = cvRect(PCentroX-PRadio, PCentroY-PRadio, 2* PRadio, 2*PRadio ); // Datos para establecer ROI del plato
			gettimeofday(&tf, NULL);
			TiempoParcial= (tf.tv_sec - ti.tv_sec)*1000 + \
														(tf.tv_usec - ti.tv_usec)/1000.0;
			TiempoGlobal= TiempoGlobal + TiempoParcial ;
			printf(" %5.4g segundos\n", TiempoGlobal/1000);
		}

		// Crear Modelo de fondo estático .Solo en la primera ejecución
		if (!hecho) {
			printf("Creando modelo de fondo..... ");
			gettimeofday(&ti, NULL);

			initBGGModel( g_capture , Capa->BGModel,Capa->IDesv, Capa->ImFMask, DataFROI);
			FrameCount = cvGetCaptureProperty( g_capture, 1 );

			gettimeofday(&tf, NULL);
			TiempoParcial= (tf.tv_sec - ti.tv_sec)*1000 + \
											(tf.tv_usec - ti.tv_usec)/1000.0;
			TiempoGlobal= TiempoGlobal + TiempoParcial ;
			printf(" %5.4g segundos\n", TiempoGlobal/1000);
			TiempoGlobal = 0; // inicializamos el tiempo global
			printf("Iniciando rastreo...\n");

		}
		// Modelado de la forma de los objetos a rastrear.
		if( !hecho){
			printf("Creando modelo de forma..... ");
			gettimeofday(&ti, NULL);

			ShapeModel( g_capture, &FlyAreaMed, & FlyAreaDes, Capa->ImFMask, DataFROI );

			FrameCount = cvGetCaptureProperty( g_capture, 1 ); //Actualizamos los frames
			gettimeofday(&tf, NULL);
			TiempoParcial= (tf.tv_sec - ti.tv_sec)*1000 + \
											(tf.tv_usec - ti.tv_usec)/1000.0;
			TiempoGlobal= TiempoGlobal + TiempoParcial ;
			printf(" %5.4g seg\n", TiempoGlobal/1000);
			TiempoGlobal = 0;
			hecho = 1;
		}

		// Modelado de fondo para detección de movimiento
		gettimeofday(&ti, NULL);

		PreProcesado( frame, Imagen, Capa->ImFMask, 0, DataFROI);

		gettimeofday(&tf, NULL);
		TiempoParcial= (tf.tv_sec - ti.tv_sec)*1000 + \
								(tf.tv_usec - ti.tv_usec)/1000.0;
		printf("\nPreprocesado: %5.4g ms\n", TiempoParcial);

		////////////// PROCESADO ///////////////

		//// BACKGROUND UPDATE
		gettimeofday(&ti, NULL);
		cvCopy( Capa->BGModel, BGTemp);
		cvCopy(Capa->IDesv,DETemp);

		// Primera actualización del fondo

		if ( UpdateCount == BGUpdate ){
			UpdateBGModel( Imagen, Capa->BGModel,Capa->IDesv, DataFROI, 0 );
			UpdateCount = 0;
		}
		gettimeofday(&tf, NULL);
		TiempoParcial= (tf.tv_sec - ti.tv_sec)*1000 + \
										(tf.tv_usec - ti.tv_usec)/1000.0;
		printf("Background update: %5.4g ms\n", TiempoParcial);

//		cvShowImage( "Foreground",Capa->BGModel);
#if CREATE_TRACKBARS == 1
	cvCreateTrackbar( "BGUpdate",
					  "Foreground",
					  &BGUpdate,
					  100  );
#endif

		/////// BACKGROUND DIFERENCE. Obtención de la máscara del foreground
		gettimeofday(&ti, NULL);
		BackgroundDifference( Imagen, Capa->BGModel,Capa->IDesv, Capa->FG , DataFROI);

		// Actualizamos el fondo haciendo uso de la máscara del foreground
		if ( UpdateCount == 0 ){
				UpdateBGModel( Imagen, BGTemp,DETemp, DataFROI, Capa->FG );
		}
		cvCopy( BGTemp, Capa->BGModel);
		cvCopy( DETemp, Capa->IDesv);
		BackgroundDifference( Imagen, Capa->BGModel,Capa->IDesv, Capa->FG , DataFROI);
		gettimeofday(&tf, NULL);
		TiempoParcial= (tf.tv_sec - ti.tv_sec)*1000 + \
						(tf.tv_usec - ti.tv_usec)/1000.0;
		printf("Obtención de máscara de Foreground : %5.4g ms\n", TiempoParcial);
		/////// EIMINACIÓN DE SOMBRAS
		// Performs FG post-processing using segmentation
		// (all pixels of a region will be classified as foreground if majority of pixels of the region are FG).
		// parameters:
		//      segments - pointer to result of segmentation (for example MeanShiftSegmentation)
		//      bg_model - pointer to CvBGStatModel structure
		//              cvRefineForegroundMaskBySegm( CvSeq* segments, bg_model );
		// Segmentacion basada en COLOR
//		 cvPyrMeanShiftFiltering( ImROI, ImROI, spatialRad, colorRad, maxPyrLevel );
//		cvPyrSegmentation(Imagen, ImPyr, storage, &comp,
//        level, threshold1+1, threshold2+1);


		/////// SEGMENTACION

		gettimeofday(&ti, NULL);
		printf( " Iniciando segmentación...\n");
		segmentacion(Imagen, Capa ,SegROI);

		gettimeofday(&tf, NULL);
		TiempoParcial= (tf.tv_sec - ti.tv_sec)*1000 + \
								(tf.tv_usec - ti.tv_usec)/1000.0;
		printf("\n\nSegmentación finalizada : %5.4g milisegundos\n", TiempoParcial);

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

		/////// VALIDACIÓN
		gettimeofday(&ti, NULL);
		printf( " Iniciando validación...\n");
//		validacion(Imagen, Capa ,SegROI);
		gettimeofday(&tf, NULL);
		TiempoParcial= (tf.tv_sec - ti.tv_sec)*1000 + \
								(tf.tv_usec - ti.tv_usec)/1000.0;
		printf("\n\nValidación finalizada : %5.4g ms\n", TiempoParcial);

		/////// TRACKING
		gettimeofday(&ti, NULL);
		cvZero( Capa->ImMotion);

//		MotionTemplate( Capa->FG, Capa->ImMotion);

//		OpticalFlowLK( Capa->FGTemp, ImOpFlowX, ImOpFlowY );

		cvCircle( Capa->ImMotion, cvPoint( PCentroX,PCentroY ), 3, CV_RGB(0,255,0), -1, 8, 0 );
		cvCircle( Capa->ImMotion, cvPoint(PCentroX,PCentroY ),PRadio, CV_RGB(0,255,0),2 );
		gettimeofday(&tf, NULL);
		TiempoParcial= (tf.tv_sec - ti.tv_sec)*1000 + \
												(tf.tv_usec - ti.tv_usec)/1000.0;
		printf("\nTracking: %5.4g ms\n", TiempoParcial);

		/*                                                              */
		/////// VISUALIZACION ////////////
		/*                                                              */

#if SHOW_VISUALIZATION == 1
		//Obtenemos la Imagen donde se visualizarán los resultados
		cvCopy(frame, ImVisual);

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
#if SHOW_BG_REMOVAL == 1
		cvShowImage("Background", Capa->BGModel);
//		cvShowImage("FG", bg_model->foreground);
		//              cvShowImage( "Blobs",ImBlobs);
		//              cvShowImage("Bina",ImThres);
//		cvShowImage( "Foreground",Capa->FG);
//		cvWaitKey(0);
#endif
#if SHOW_OPTICAL_FLOW == 1
		cvShowImage( "Flujo Optico X", ImOpFlowX );
		cvShowImage( "Flujo Optico Y", ImOpFlowY);
#endif

		cvShowImage( "Motion",Capa->ImMotion);

		gettimeofday(&tff, NULL);
		TiempoFrame = (tff.tv_sec - tif.tv_sec)*1000 + \
				(tff.tv_usec - tif.tv_usec)/1000.0;
		TiempoGlobal = TiempoGlobal + TiempoFrame;
		printf("\n//////////////////////////////////////////////////\n");
		printf("\nTiempo de procesado del Frame %.0f : %5.4g ms\n",FrameCount, TiempoFrame);
		printf("Segundos de video procesados: %.3f seg \n", TiempoGlobal/1000);
		printf("Porcentaje completado: %.2f % \n",(FrameCount/TotalFrames)*100 );
		printf("\n//////////////////////////////////////////////////\n");
	}

	/*oooooooooooooooooooooooooooooooooooooooooo*/
	 /*o			ANALISIS ESTADÍSTICO      o*/
	  /*oooooooooooooooooooooooooooooooooooooo*/

	printf( "Rastreo finalizado con éxito ..." );
	printf( "Comenzando análisis estadístico de los datos obtenidos ...\n" );
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
	Capa->FGTemp = cvCreateImage(sz,8,1);
	Capa->IDesv = cvCreateImage(sz,8,1);
	Capa->ImFMask = cvCreateImage(sz,8,1);
	Capa->ImRois = cvCreateImage(sz,8,1);
	Capa->OldFG = cvCreateImage(sz,8,1);
	Capa->ImMotion = cvCreateImage( sz, 8, 3 );

	cvZero( Capa->BGModel );
	cvZero( Capa->FG );
	cvZero( Capa->FGTemp );
	cvZero( Capa->IDesv );
	cvZero( Capa->ImFMask );
	cvZero( Capa->ImRois );
	cvZero( Capa->OldFG );
	cvZero( Capa->ImMotion );
	Capa->ImMotion->origin = I->origin;

	BGTemp = cvCreateImage( sz,8,1);
	DETemp = cvCreateImage( sz,8,1);
	FOTemp = cvCreateImage( sz,8,1);
	Imagen = cvCreateImage( sz ,8,1);
	ImPyr = cvCreateImage( sz ,8,1);
	ImOpFlowX = cvCreateImage( sz ,IPL_DEPTH_32F,1 );
	ImOpFlowY = cvCreateImage( sz ,IPL_DEPTH_32F,1 );
	ImBlobs = cvCreateImage( sz,8,1 );
	ImThres = cvCreateImage( sz,8,1 );
	ImVisual = cvCreateImage( sz,8,3);


}

// Libera memoria de las imagenes creadas

void DeallocateImages( ){

	cvReleaseImage( &Imagen );
	cvReleaseImage( &ImPyr);
	cvReleaseImage( &ImOpFlowX);
	cvReleaseImage( &ImOpFlowY);
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

	 cvNamedWindow( "Motion", 1 );

#if SHOW_OPTICAL_FLOW == 1
	cvNamedWindow( "Flujo Optico X",CV_WINDOW_AUTOSIZE);
	cvNamedWindow( "Flujo Optico X",CV_WINDOW_AUTOSIZE);
	cvMoveWindow("Flujo Optico X", 0, 0 );
	cvMoveWindow("Flujo Optico Y", 640, 0);
#endif
#if SHOW_VISUALIZATION == 1
	cvNamedWindow( "Visualización",CV_WINDOW_AUTOSIZE);
#endif
	//        cvNamedWindow( "Imagen", CV_WINDOW_AUTOSIZE);
    //	cvNamedWindow( "Region_Of_Interest", CV_WINDOW_AUTOSIZE);

}
// Destruccion de ventanas
void DestroyWindows( ){
	cvDestroyWindow( "Drosophila.avi" );


#if SHOW_BG_REMOVAL == 1
	cvDestroyWindow( "Background");
	cvDestroyWindow( "Foreground");
#endif
#if SHOW_OPTICAL_FLOW == 1
	cvDestroyWindow( "Flujo Optico X");
	cvDestroyWindow( "Flujo Optico Y");
#endif
#if SHOW_VISUALIZATION == 1
	cvDestroyWindow( "Visualización" );
#endif
	cvDestroyWindow( "Motion" );
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
