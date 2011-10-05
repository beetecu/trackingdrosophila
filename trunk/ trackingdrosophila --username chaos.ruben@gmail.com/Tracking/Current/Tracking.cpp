/*!
 * Tracking.cpp
 *
 * Cuerpo principal del algoritmo de rastreo.
 *
 *  Created on: 27/06/2011
 *      Author: chao
 */

#include "Tracking.h"

using namespace cv;
using namespace std;
void help(){
	printf("\n Para ejecutar el programa escriba en la consola: "
			"TrackingDrosophila [nombre_video.avi] \n  ");
}
int main(int argc, char* argv[]) {

	if( argc<2) {help(); return -1;};

	///////////  CAPTURA  ////////////
	gettimeofday(&ti, NULL);  //para obtener el tiempo
	TiempoInicial= ti.tv_sec*1000 + ti.tv_usec/1000.0;
	printf( "Iniciando captura..." );

	g_capture = NULL;
	g_capture = cvCaptureFromAVI( argv[1] );
	if ( !g_capture ) {
		error( 1 );
		return -1;
	}
	frame = cvQueryFrame( g_capture );
	///////////  INICIALIZACIÓN ////////////
	int ok = Inicializacion(frame, &Flat, &Capa, &Shape,&BGParams);
	if (!ok ) return -1;

	gettimeofday(&tf, NULL);
	TiempoParcial= (tf.tv_sec - ti.tv_sec)*1000
			     + (tf.tv_usec - ti.tv_usec)/1000.0;
	printf(" %5.4g ms\n", TiempoParcial);

	/*********** BUCLE PRINCIPAL DEL ALGORITMO ***********/

	while ( g_capture ) {

		frame = cvQueryFrame( g_capture );
		if ( !frame ) {
			error(2);
			break;
		}
		if ( (cvWaitKey(10) & 255) == 27 ) break;
		FrameCount += 1;
		UpdateCount += 1;
		gettimeofday(&tif, NULL);

		////////// PREPROCESADO ///////////////
		int hecho = PreProcesado( ) ;
		if (!hecho) return -1;
		////////// PROCESADO ///////////////
		Procesado( );
		//////////  TRACKING ////////////////////
		Tracking( );
		////////// VISUALIZACION ////////////
		Visualizacion();
	}

	AnalisisEstadistico();

	// LIMPIAR MEMORIA

	free(Capa);
	free(Flat);
	free(Flie);
	DeallocateImages( );
	DeallocateImagesBGM();

	cvReleaseCapture(&g_capture);

	DestroyWindows( );

}

int Inicializacion(IplImage* frame,
					STFlat** Flat,
					STCapas** Capa ,
					SHModel** Shape,
					BGModelParams** BGParams){
	// Creación de ventanas de visualizacion
	CreateWindows( );
	//      cvSetCaptureProperty( g_capture	cvResetImageROI(Capa->BGModel);, CV_CAP_PROP_POS_AVI_RATIO,0 );
	//      Añadimos un slider a la ventana del video Drosophila.avi
	//              int TotalFrames = getAVIFrames("Drosophila.avi"); // en algun linux no funciona lo anterior

	// Iniciar estructura para almacenar las Capas
	STCapas* Cap;
	Cap = ( STCapas *) malloc( sizeof( STCapas));
	if ( !Capa ) {
		error(4);
		return 0;
	}
	// creación de imagenes a utilizar
	AllocateImages( frame, Cap );

	*Capa = Cap;
	// Iniciar estructura para modelo de plato
	STFlat* flat;
	flat = ( STFlat *) malloc( sizeof( STFlat));
	if ( !Flat ) {
			error(4);
			return 0;
	}
	flat->PCentroX = 0;
	flat->PCentroY = 0;
	flat->PRadio = 0;
	*Flat = flat;

	// Iniciar estructura para modelo de forma
	SHModel *shape;
	shape = ( SHModel *) malloc( sizeof( SHModel));
	if ( !shape ) {
			error(4);
			return 0;
		}
	shape->FlyAreaDes = 0;
	shape->FlyAreaMed = 0;
	shape->FlyAreaMedia=0;
	*Shape = shape;
	// Iniciar estructura para parametros del modelo de fondo en primera actualización
	BGModelParams *bgparams;
	bgparams = ( BGModelParams *) malloc( sizeof( BGModelParams));
	if ( !bgparams ) {
				error(4);
				return 0;
	}
	*BGParams = bgparams;
	// Iniciar estructura para parámetros del modelo de fondo para validación
//	BGForVal = ( BGModelParams *) malloc( sizeof( BGModelParams));
	// Iniciar estructura para almacerar datos de blobs

	//STFlies *Flies=NULL;
	Lista llse; // Apuntará al primer elemento de la lista lineal
	iniciarLista(&llse);


	cvSetCaptureProperty( g_capture, CV_CAP_PROP_POS_AVI_RATIO,0 );
	TotalFrames = cvGetCaptureProperty( g_capture, CV_CAP_PROP_FRAME_COUNT);
	return 1;
}

int PreProcesado( ){

	static int hecho = 0;
	// Obtencion de mascara del plato
	if( !hecho ){
		printf("\nIniciando preprocesado.");
		printf("Localizando plato... ");
		gettimeofday(&ti, NULL);

		MascaraPlato( g_capture, Capa->ImFMask, Flat );

		if ( Flat->PRadio == 0  ) {
			error(3);
			return 0;
		}

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
		// establecer parametros
		InitialBGModelParams( BGParams);
		initBGGModel( g_capture , Capa->BGModel,Capa->IDesv, Capa->ImFMask, BGParams, Flat->DataFROI);
		FrameCount = cvGetCaptureProperty( g_capture, 1 );

		gettimeofday(&tf, NULL);
		TiempoParcial= (tf.tv_sec - ti.tv_sec)*1000 + \
										(tf.tv_usec - ti.tv_usec)/1000.0;
		TiempoGlobal= TiempoGlobal + TiempoParcial ;
		printf(" %5.4g segundos\n", TiempoGlobal/1000);
		TiempoGlobal = 0; // inicializamos el tiempo global
	}
	// Modelado de la forma de los objetos a rastrear.
	if( !hecho){
		printf("Creando modelo de forma..... ");
		gettimeofday(&ti, NULL);

		ShapeModel( g_capture, Shape , Capa->ImFMask, Flat->DataFROI );

		FrameCount = cvGetCaptureProperty( g_capture, 1 ); //Actualizamos los frames
		gettimeofday(&tf, NULL);
		TiempoParcial= (tf.tv_sec - ti.tv_sec)*1000 + \
										(tf.tv_usec - ti.tv_usec)/1000.0;
		TiempoGlobal= TiempoGlobal + TiempoParcial ;
		printf(" %5.4g seg\n", TiempoGlobal/1000);
		printf("Fin preprocesado. Iniciando procesado...\n");
		TiempoGlobal = 0;
	}
	return hecho = 1;
}

void Procesado(){

	gettimeofday(&ti, NULL);

	ImPreProcess( frame, Imagen, Capa->ImFMask, 0, Flat->DataFROI);

	gettimeofday(&tf, NULL);
	TiempoParcial= (tf.tv_sec - ti.tv_sec)*1000 + \
							(tf.tv_usec - ti.tv_usec)/1000.0;
	printf( "\n\t\t\tFRAME %.0f\n", FrameCount);
	printf("\nPreprocesado de imagen: %5.4g ms\n", TiempoParcial);

	cvCopy( Capa->BGModel, BGTemp); // guardamos una copia del modelo original
	cvCopy(Capa->IDesv,DETemp);
	cvZero( Capa->FG);
	for ( int i = 0; i < 3; i++){
		gettimeofday(&ti, NULL);
		if ( i == 0 ) printf("\nDefiniendo foreground :\n\n");
		if ( i > 0 ) printf("\nRedefiniendo foreground %d de 2:\n\n", i);
		//// BACKGROUND UPDATE
		// Primera actualización del fondo
		// establecer parametros
		InitialBGModelParams( BGParams);

		UpdateBGModel( Imagen, Capa->BGModel,Capa->IDesv, BGParams, Flat->DataFROI, Capa->FG );

		gettimeofday(&tf, NULL);
		TiempoParcial= (tf.tv_sec - ti.tv_sec)*1000 + \
										(tf.tv_usec - ti.tv_usec)/1000.0;
		printf("Background update: %5.4g ms\n", TiempoParcial);

		/////// BACKGROUND DIFERENCE. Obtención de la máscara del foreground
		gettimeofday(&ti, NULL);

		BackgroundDifference( Imagen, Capa->BGModel,Capa->IDesv, Capa->FG ,BGParams, Flat->DataFROI);

		gettimeofday(&tf, NULL);
		TiempoParcial= (tf.tv_sec - ti.tv_sec)*1000 + \
									(tf.tv_usec - ti.tv_usec)/1000.0;
		printf("Obtención de máscara de Foreground : %5.4g ms\n", TiempoParcial);
		/////// SEGMENTACION
		if( i > 0 ){
			gettimeofday(&ti, NULL);
			printf( "Segmentando Foreground...");

			segmentacion(Imagen, Capa, Flat->DataFROI,Flie);
			cvCopy( Capa->FGTemp, Capa->FG);
			gettimeofday(&tf, NULL);
			TiempoParcial= (tf.tv_sec - ti.tv_sec)*1000 + \
									(tf.tv_usec - ti.tv_usec)/1000.0;
			printf(" %5.4g ms\n", TiempoParcial);
		}
		// en la ultima iteracion nos kedamos con ultimo BGModel obtenido
		if (i < 2 ){
			cvCopy( BGTemp, Capa->BGModel );
			cvCopy( DETemp, Capa->IDesv );
		}
		/////// VALIDACIÓN
		// solo en la última iteracion
//			if (i > 1){
//				gettimeofday(&ti, NULL);
//				printf( "\nValidando contornos...");
//
//		//		Validacion(Imagen, Capa , Shape, Flat->DataFROI, Flie, NULL, NULL);
//
//				gettimeofday(&tf, NULL);
//				TiempoParcial= (tf.tv_sec - ti.tv_sec)*1000 +
//										(tf.tv_usec - ti.tv_usec)/1000.0;
//				printf(" %5.4g ms\n", TiempoParcial);
//			}
	}

}

void Visualizacion(){
	if (SHOW_VISUALIZATION == 1){
	//Obtenemos la Imagen donde se visualizarán los resultados
	cvCopy(frame, ImVisual);

	//Dibujamos el plato en la imagen de visualizacion
	cvCircle( ImVisual, cvPoint( Flat->PCentroX,Flat->PCentroY ), 3, CV_RGB(0,0,0), -1, 8, 0 );
	cvCircle( ImVisual, cvPoint(Flat->PCentroX,Flat->PCentroY ),Flat->PRadio, CV_RGB(0,0,0),2 );
	// Dibujamos la ROI
	cvRectangle( ImVisual,
			cvPoint(Flat->PCentroX-Flat->PRadio, Flat->PCentroY-Flat->PRadio),
			cvPoint(Flat->PCentroX + Flat->PRadio,Flat->PCentroY + Flat->PRadio),
			CV_RGB(255,0,0),2);

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

	}
	// Mostramos imagenes
	cvShowImage( "Drosophila.avi", frame );
	//
	if (SHOW_BG_REMOVAL == 1){
			cvShowImage("Background", Capa->BGModel);
//				cvShowImage( "Foreground",Capa->FG);

	//		cvWaitKey(0);
	}
	if (SHOW_OPTICAL_FLOW == 1){
	cvShowImage( "Flujo Optico X", ImOpFlowX );
	cvShowImage( "Flujo Optico Y", ImOpFlowY);
	}
	if ( SHOW_MOTION_TEMPLATE == 1){
		cvShowImage( "Motion",Capa->ImMotion);
		}
	;

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

void Tracking(){
	gettimeofday(&ti, NULL);
	cvZero( Capa->ImMotion);
	if ( SHOW_MOTION_TEMPLATE == 1){
		MotionTemplate( Capa->FG, Capa->ImMotion);
	}


//		OpticalFlowLK( Capa->FGTemp, ImOpFlowX, ImOpFlowY );

	cvCircle( Capa->ImMotion, cvPoint( Flat->PCentroX,Flat->PCentroY ), 3, CV_RGB(0,255,0), -1, 8, 0 );
	cvCircle( Capa->ImMotion, cvPoint(Flat->PCentroX,Flat->PCentroY ),Flat->PRadio, CV_RGB(0,255,0),2 );
	gettimeofday(&tf, NULL);
	TiempoParcial= (tf.tv_sec - ti.tv_sec)*1000 + \
											(tf.tv_usec - ti.tv_usec)/1000.0;
	printf("Tracking: %5.4g ms\n", TiempoParcial);
}

void AnalisisEstadistico(){
	printf( "Rastreo finalizado con éxito ..." );
	printf( "Comenzando análisis estadístico de los datos obtenidos ...\n" );
	printf( "Análisis finalizado ...\n" );
}
void AllocateImages( IplImage* I ,STCapas* Capa){

	// Crear imagenes y redimensionarlas en caso de que cambien su tamaño

	CvSize size = cvGetSize( I );

//	if( !Capa->BGModel ||
//		 Capa->BGModel->width != size.width ||
//		 Capa->BGModel->height != size.height ) {
//
//		cvReleaseImage( &Capa->BGModel );
//		cvReleaseImage( &Capa->FG );
//		cvReleaseImage( &Capa->IDesv );
//		cvReleaseImage( &Capa->ImFMask );
//		cvReleaseImage( &Capa->ImRois );
//		cvReleaseImage( &Capa->OldFG );
//		cvReleaseImage( &Capa->ImMotion );

		Capa->BGModel = cvCreateImage(size,8,1);
		Capa->FG = cvCreateImage(size,8,1);
		Capa->FGTemp = cvCreateImage(size,8,1);
		Capa->IDesv = cvCreateImage(size,8,1);
		Capa->ImFMask = cvCreateImage(size,8,1);
		Capa->ImRois = cvCreateImage(size,8,1);
		Capa->OldFG = cvCreateImage(size,8,1);
		Capa->ImMotion = cvCreateImage( size, 8, 3 );

		cvZero( Capa->BGModel );
		cvZero( Capa->FG );
		cvZero( Capa->FGTemp );
		cvZero( Capa->IDesv );
		cvZero( Capa->ImFMask );
		cvZero( Capa->ImRois );
		cvZero( Capa->OldFG );
		cvZero( Capa->ImMotion );
		Capa->ImMotion->origin = I->origin;

		cvReleaseImage( &BGTemp );
		cvReleaseImage( &DETemp );
		cvReleaseImage( &Imagen );
		cvReleaseImage( &ImOpFlowX );
		cvReleaseImage( &ImOpFlowY );
		cvReleaseImage( &ImVisual );

		BGTemp = cvCreateImage( size,8,1);
		DETemp = cvCreateImage( size,8,1);
		Imagen = cvCreateImage( size ,8,1);
		ImOpFlowX = cvCreateImage( size ,IPL_DEPTH_32F,1 );
		ImOpFlowY = cvCreateImage( size ,IPL_DEPTH_32F,1 );
		ImVisual = cvCreateImage( size,8,3);

		cvZero( BGTemp );
		cvZero( DETemp );
		cvZero( Imagen );
		cvZero( ImOpFlowX );
		cvZero( ImOpFlowY );
		cvZero( ImVisual );

	//}
}

// Libera memoria de las imagenes creadas

void DeallocateImages( ){

	cvReleaseImage( &Imagen );
	cvReleaseImage( &ImOpFlowX);
	cvReleaseImage( &ImOpFlowY);
	cvReleaseImage( &ImVisual );
	cvReleaseImage( &BGTemp);
	cvReleaseImage( &DETemp);
	cvReleaseImage( &BGTemp1);
	cvReleaseImage( &DETemp1);

}

// Creación de ventanas
void CreateWindows( ){

	cvNamedWindow( "Drosophila.avi", CV_WINDOW_AUTOSIZE );
	if (SHOW_BG_REMOVAL == 1){
		cvNamedWindow( "Background",CV_WINDOW_AUTOSIZE);
		cvNamedWindow( "Foreground",CV_WINDOW_AUTOSIZE);

		cvMoveWindow("Background", 0, 0 );
		cvMoveWindow("Foreground", 640, 0);
	}
	if ( SHOW_MOTION_TEMPLATE == 1){
		cvNamedWindow( "Motion", 1 );
	}


	if (SHOW_OPTICAL_FLOW == 1){
		cvNamedWindow( "Flujo Optico X",CV_WINDOW_AUTOSIZE);
		cvNamedWindow( "Flujo Optico X",CV_WINDOW_AUTOSIZE);
		cvMoveWindow("Flujo Optico X", 0, 0 );
		cvMoveWindow("Flujo Optico Y", 640, 0);
	}
	if (SHOW_VISUALIZATION == 1){
			cvNamedWindow( "Visualización",CV_WINDOW_AUTOSIZE);
	}
	//        cvNamedWindow( "Imagen", CV_WINDOW_AUTOSIZE);
    //	cvNamedWindow( "Region_Of_Interest", CV_WINDOW_AUTOSIZE);

}
// Destruccion de ventanas
void DestroyWindows( ){
	cvDestroyWindow( "Drosophila.avi" );


if (SHOW_BG_REMOVAL == 1){
	cvDestroyWindow( "Background");
	cvDestroyWindow( "Foreground");
}
if (SHOW_OPTICAL_FLOW == 1){
	cvDestroyWindow( "Flujo Optico X");
	cvDestroyWindow( "Flujo Optico Y");
}
if ( SHOW_MOTION_TEMPLATE == 1){
	cvDestroyWindow( "Motion");
}
if (SHOW_VISUALIZATION == 1) {
	cvDestroyWindow( "Visualización" );
}
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

	STFlies *Flies = NULL;


	while (i < lista->numeroDeElementos)
	{

		Flies = (STFlies *)obtener(i, lista);
		//printf("\n Vertical : %f, Horizontal: %f, Punto : %f",Flies->,Flies->VH,Flies->punto1.x );

		i++;
	}
}
void InitialBGModelParams( BGModelParams* Params){
	 static int first = 1;
	 Params->FRAMES_TRAINING = 20;
	 Params->ALPHA = 0 ;
	 Params->MORFOLOGIA = 0;
	 Params->CVCLOSE_ITR = 1;
	 Params->MAX_CONTOUR_AREA = 200 ;
	 Params->MIN_CONTOUR_AREA = 5;
	 if (CREATE_TRACKBARS == 1){
				 // La primera vez inicializamos los valores.
				 if (first == 1){
					 Params->HIGHT_THRESHOLD = 20;
					 Params->LOW_THRESHOLD = 10;
					 first = 0;
				 }
	 			cvCreateTrackbar( "HighT",
	 							  "Foreground",
	 							  &Params->HIGHT_THRESHOLD,
	 							  100  );
	 			cvCreateTrackbar( "LowT",
	 							  "Foreground",
	 							  &Params->LOW_THRESHOLD,
	 							  100  );
	 }else{
		 Params->HIGHT_THRESHOLD = 20;
		 Params->LOW_THRESHOLD = 10;
	 }
}
