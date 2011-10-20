/*!
 * TrackingDrosophila.cpp
 *
 * Cuerpo principal del algoritmo de rastreo.
 *
 *  Created on: 27/06/2011
 *      Author: chao
 */

#include "TrackingDrosophila.hpp"

using namespace cv;
using namespace std;

void help(){
	printf("\n Para ejecutar el programa escriba en la consola: "
			"TrackingDrosophila [nombre_video.avi] [Nombre_Fichero]\n  "
			"Donde:\n - [nombre_video.avi] es el nombre del video a analizar. Ha de "
			"estar en formato avi. Se deberán tener instalados los codecs ffmpeg.\n"
			"[Nombre_Fichero] Será el nombre del fichero donde se guardarán los datos."
			"Si no se especifica se le solicitará uno al usuario. Si continúa sin especificarse"
			"se establecerá [Data] por defecto. ");
}
int main(int argc, char* argv[]) {

	if( argc<1) {help(); return -1;};

	///////////  CAPTURA  ////////////
	gettimeofday(&ti, NULL);  //para obtener el tiempo
	TiempoInicial= ti.tv_sec*1000 + ti.tv_usec/1000.0;
	printf( "Iniciando captura..." );

	g_capture = NULL;
	g_capture = cvCaptureFromAVI( argv[1] );
	if ( !g_capture ) {
		error( 1 );
		help();
		return -1;
	}
	frame = cvQueryFrame( g_capture );

	///////////  INICIALIZACIÓN ////////////

	//inicializar estructuras, imagenes y fichero de datos
	if (!Inicializacion(frame, &Flat, &Shape, &BGParams,&BGModel, argc, argv) ) return -1;

	//inicializar buffer de datos.
	FramesBuf = ( tlcde * )malloc( sizeof(tlcde));
	if( !FramesBuf ) {error(4);Finalizar();}
	iniciarLcde( FramesBuf );

	gettimeofday(&tf, NULL);
	TiempoParcial= (tf.tv_sec - ti.tv_sec)*1000
			     + (tf.tv_usec - ti.tv_usec)/1000.0;
	printf(" %5.4g ms\n", TiempoParcial);

	//////////  PREPROCESADO   ////////////
	if (!PreProcesado( ) ) Finalizar();

	/*********** BUCLE PRINCIPAL DEL ALGORITMO ***********/
    for( FrameCountRel = 1;frame; frame = cvQueryFrame(g_capture), FrameCountRel++ ){
    	/*Posteriormente  Escribir en un fichero log el error. Actualizar el contador
    	  de frames absolutos. */
    	if( !frame ) RetryCap();
    	if( !RetryCap ) Finalizar();

		if ( (cvWaitKey(10) & 255) == 27 ) break;
		FrameCountAbs = cvGetCaptureProperty( g_capture, 1);
		UpdateCount += 1;
		gettimeofday(&tif, NULL);

		//////////  PROCESAR      ////////////
		Procesado( );

		//////////  RASTREAR       ////////////
		Tracking(  );

		//////////  VISUALIZAR     ////////////
		Visualizacion();

		//////////  ALMACENAR ////////////
		// Se mantienen en memoria las estructuras correspondientes a STRUCT_BUFFER_LENGTH frames
		// ( buffer de datos) e IMAGE_BUFFER_LENGHT ( buffer de imagenes ).
		if( FramesBuf->numeroDeElementos == STRUCT_BUFFER_LENGTH){
			FrameData = (STFrame*) FramesBuf->ultimo->dato;
			mostrarListaFlies(FrameData->Flies);
			// Una vez que se llenan los buffer se almacenan los datos del primer frame en fichero

			if(!GuardarPrimero( FramesBuf, nombreFichero ) ){error(6);Finalizar();}
			if(!liberarPrimero( FramesBuf ) ){error(7);Finalizar();};
			FrameData = NULL;
		}
	}
    ///////// POSTPROCESADO //////////
	AnalisisEstadistico();

	///////// LIBERAR MEMORIA Y TERMINAR////////
	Finalizar();
}


int Inicializacion( IplImage* frame,
					STFlat** Flat,
					SHModel** Shape,
					BGModelParams** BGParams,
					StaticBGModel** BGModel,
					int argc,
					char* argv[]){
	// Creación de ventanas de visualizacion
	CreateWindows( );
	//      cvSetCaptureProperty( g_capture	cvResetImageROI(Capa->BGModel);, CV_CAP_PROP_POS_AVI_RATIO,0 );
	//      Añadimos un slider a la ventana del video Drosophila.avi
	//              int TotalFrames = getAVIFrames("Drosophila.avi"); // en algun linux no funciona lo anterior


	// Iniciar estructura para modelo de plato
	STFlat* flat = NULL;
	flat = *Flat;
	flat = ( STFlat *) malloc( sizeof( STFlat));
	if ( !Flat ) {error(4);return 0;}
	flat->PCentroX = 0;
	flat->PCentroY = 0;
	flat->PRadio = 0;
	*Flat = flat;

	// Iniciar estructura para modelo de forma
	SHModel *shape = NULL;
	shape = *Shape;
	shape = ( SHModel *) malloc( sizeof( SHModel));
	if ( !shape ) {error(4);return 0;}
	shape->FlyAreaDes = 0;
	shape->FlyAreaMed = 0;
	shape->FlyAreaMedia=0;
	*Shape = shape;
	// Iniciar estructura para parametros del modelo de fondo en primera actualización
	BGModelParams *bgparams = NULL;
	bgparams = *BGParams;
	bgparams = ( BGModelParams *) malloc( sizeof( BGModelParams));
	if ( !bgparams ) {error(4);return 0;}
	*BGParams = bgparams;

	//Iniciar estructura para el model de fondo estático
	StaticBGModel* bgmodel;
	bgmodel = *BGModel;
	bgmodel = ( StaticBGModel*) malloc( sizeof( StaticBGModel));
	if ( !bgmodel ) {error(4);return 0;}
	*BGModel = bgmodel;

	AllocateImages( frame, bgmodel);
	// Obtener datos del video y regresar puntero CvCapture al inicio del video.
	cvSetCaptureProperty( g_capture, CV_CAP_PROP_POS_AVI_RATIO,0 );
	TotalFrames = cvGetCaptureProperty( g_capture, CV_CAP_PROP_FRAME_COUNT);

	/// Crear fichero de datos.
	if( argc == 2 ){
			// nombre por defecto
//		  do{
			  const char nombre[] = "Data";
//			  int i = 1;
//			  toascii( i );
			  strncpy(nombreFichero, nombre,5);
//			  strncat(nombreFichero, i,10);
//			  i++;
//		  }
//		  while( existe( nombreFichero) );
		  crearFichero( nombreFichero );
	}
	if( argc == 3 ){
		if( existe(argv[2]) ){ // verificar si el nombre del archivo de datos existe.
			char resp = getchar();
			fflush(stdin);
			do
			{
			  printf("\nEl fichero existe ¿desea sobrescribirlo? (s/n): ");

			  if (resp == 's'||'S')  crearFichero(argv[2]);
			  else if (resp == 'n'||'N'){
				  printf("\nEscriba el nuevo nombre o pulse intro para nombre por defecto: ");
				  fgets(nombreFichero, 30, stdin );
				  fflush(stdin);
//				  if (strlen(nombreFichero)<1 ) {// nombre por defecto
//					  do{
//					  int i = 1;
//					  toascii( i );
//					  nombreFichero[30] = "Data";
//					  strncat(nombreFichero, i);
//					  i++;
//					  }
//					  while( existe( nombreFichero) );
//				  }
				  crearFichero( nombreFichero );
			  }
			}
			while (resp != 's'||'S'||'n'||'N');
		}
	}

	return 1;
}

int PreProcesado(  ){

	static int hecho = 0;
	// Obtencion de mascara del plato
	if( !hecho ){
		printf("\nIniciando preprocesado.");
		printf("Localizando plato... ");
		gettimeofday(&ti, NULL);

		MascaraPlato( g_capture, BGModel->ImFMask, Flat );

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
		initBGGModel( g_capture ,BGModel->Imed,BGModel->IDesv, BGModel->ImFMask, BGParams, Flat->DataFROI);

		gettimeofday(&tf, NULL);
		TiempoParcial= (tf.tv_sec - ti.tv_sec)*1000 + \
										(tf.tv_usec - ti.tv_usec)/1000.0;
		TiempoGlobal = TiempoGlobal + TiempoParcial ;
		printf(" %5.4g segundos\n", TiempoGlobal/1000);
		TiempoGlobal = 0; // inicializamos el tiempo global
	}
	// Modelado de la forma de los objetos a rastrear.
	if( !hecho){
		printf("Creando modelo de forma..... ");
		gettimeofday(&ti, NULL);

//		ShapeModel( g_capture, Shape , FrameData->ImFMask, Flat->DataFROI );

		gettimeofday(&tf, NULL);
		TiempoParcial= (tf.tv_sec - ti.tv_sec)*1000 + \
										(tf.tv_usec - ti.tv_usec)/1000.0;
		TiempoGlobal= TiempoGlobal + TiempoParcial ;
		printf(" %5.4g seg\n", TiempoGlobal/1000);
		printf("Fin preprocesado. Iniciando procesado...\n");
		TiempoGlobal = 0;
	}
	FrameCountAbs = cvGetCaptureProperty( g_capture, 1 ); //Actualizamos los frames

	return hecho = 1;
}

/*  Esta función realiza las siguientes acciones:
 *
 * - Limpieza del foreground en tres etapas :
 *   1_Actualización de fondo y resta de fondo obteniendo el foreground
 *   2_Nueva actualización de fondo usando la máscacara de foreground obtenida.
 *   Resta de fondo
 *   Redefinición de la máscara de foreground mediante ajuste por elipses y
 *   obtención de los parámetros de los blobs en segmentación.
 *   3_Repetir de nuevo el ciclo de actualizacion-resta-segmentación con la nueva máscara
 *   obteniendo así el foreground y el background definitivo
 *
 * - Rellena la lista lineal doblemente enlazada ( Flies )con los datos de cada uno de los blobs
 * - Rellena la estructura FrameData con las nuevas imagenes y la lista Flies.
 * - Finalmente añade la lista Flies a la estructura FrameData
 *   y la estructura FrameData al buffer FramesBuf   */
void Procesado(){

	static int first = 1;
	gettimeofday(&ti, NULL);

	ImPreProcess( frame, Imagen, BGModel->ImFMask, 0, Flat->DataFROI);
/* Nota. */
	// Iniciar estructura para datos del nuevo frame
	if( first ) { //en la primera iteración
		FrameData = ( STFrame *) malloc( sizeof(STFrame));
		InitNewFrameData( Imagen, FrameData );
		cvCopy(  BGModel->Imed,FrameData->BGModel);
		cvCopy(BGModel->IDesv,FrameData->IDesv);
		first = 0;
	}
	else{
		irAlFinal( FramesBuf );
		FrameData = ( STFrame*)obtenerActual( FramesBuf );
		cvCopy( FrameData->BGModel, BGTemp);
		cvCopy( FrameData->IDesv, DETemp);
		FrameData = NULL;
		FrameData = ( STFrame *) malloc( sizeof(STFrame));
		InitNewFrameData( Imagen, FrameData );
		// copiamos los últimos parámetros del fondo.
		cvCopy( BGTemp, FrameData->BGModel);
		cvCopy( DETemp, FrameData->IDesv);
	}
	cvCopy(  FrameData->BGModel,BGTemp );
	cvCopy( FrameData->IDesv,DETemp );

	gettimeofday(&tf, NULL);
	TiempoParcial= (tf.tv_sec - ti.tv_sec)*1000 + \
							(tf.tv_usec - ti.tv_usec)/1000.0;
	printf( "\n\t\t\tFRAME %.0f\n", FrameCountAbs);
	printf("\nPreprocesado de imagen: %5.4g ms\n", TiempoParcial);

	for ( int i = 0; i < 3; i++){
		gettimeofday(&ti, NULL);
		if ( i == 0 ) printf("\nDefiniendo foreground :\n\n");
		if ( i > 0 ) printf("\nRedefiniendo foreground %d de 2:\n\n", i);
		//// BACKGROUND UPDATE
		// Actualización del fondo original
		// establecer parametros
		InitialBGModelParams( BGParams);

		UpdateBGModel( Imagen, FrameData->BGModel,FrameData->IDesv, BGParams, Flat->DataFROI, FrameData->FG );

		gettimeofday(&tf, NULL);
		TiempoParcial= (tf.tv_sec - ti.tv_sec)*1000 + \
										(tf.tv_usec - ti.tv_usec)/1000.0;
		printf("Background update: %5.4g ms\n", TiempoParcial);

		/////// BACKGROUND DIFERENCE. Obtención de la máscara del foreground
		gettimeofday(&ti, NULL);

		BackgroundDifference( Imagen, FrameData->BGModel,FrameData->IDesv, FrameData->FG ,BGParams, Flat->DataFROI);

		gettimeofday(&tf, NULL);
		TiempoParcial= (tf.tv_sec - ti.tv_sec)*1000 + \
									(tf.tv_usec - ti.tv_usec)/1000.0;
		printf("Obtención de máscara de Foreground : %5.4g ms\n", TiempoParcial);
		/////// SEGMENTACION
		if( i > 0 ){
			if(FrameData->Flies != NULL) {
				liberarListaFlies( FrameData->Flies );//nos quedamos con la última
				free(FrameData->Flies);
				FrameData->Flies = NULL;
			}
			gettimeofday(&ti, NULL);
			printf( "Segmentando Foreground...");

			FrameData->Flies = segmentacion(Imagen, FrameData, Flat->DataFROI,BGModel->ImFMask);

			gettimeofday(&tf, NULL);
			TiempoParcial= (tf.tv_sec - ti.tv_sec)*1000 + \
									(tf.tv_usec - ti.tv_usec)/1000.0;
			printf(" %5.4g ms\n", TiempoParcial);
		}
		// Volvemos a cargar el original,en la ultima iteracion nos kedamos con ultimo BGModel obtenido
		if (i < 2 ){
			cvCopy( BGTemp, FrameData->BGModel );
			cvCopy( DETemp, FrameData->IDesv );
		}
		/////// VALIDACIÓN
		// solo en la última iteracion
//			if (i > 1){
//				gettimeofday(&ti, NULL);
//				printf( "\nValidando contornos...");
//
//		//		Validacion(Imagen, FrameData , Shape, Flat->DataFROI, Flie, NULL, NULL);
//
//				gettimeofday(&tf, NULL);
//				TiempoParcial= (tf.tv_sec - ti.tv_sec)*1000 +
//										(tf.tv_usec - ti.tv_usec)/1000.0;
//				printf(" %5.4g ms\n", TiempoParcial);
//			}

	}
	// Una vez validada añadimos ( al final ) las estructuras a las listas (buffers).
	anyadirAlFinal( FrameData, FramesBuf );
	FrameData = NULL;
}
void Tracking(){
	irAlFinal( FramesBuf);
	FrameData = ( STFrame* )obtenerActual( FramesBuf );
	gettimeofday(&ti, NULL);
	cvZero( FrameData->ImMotion);
	if ( SHOW_MOTION_TEMPLATE == 1){
		MotionTemplate( FrameData->FG, FrameData->ImMotion);
	}


//		OpticalFlowLK( FrameData->FG, ImOpFlowX, ImOpFlowY );

	cvCircle( FrameData->ImMotion, cvPoint( Flat->PCentroX,Flat->PCentroY ), 3, CV_RGB(0,255,0), -1, 8, 0 );
	cvCircle( FrameData->ImMotion, cvPoint(Flat->PCentroX,Flat->PCentroY ),Flat->PRadio, CV_RGB(0,255,0),2 );
	gettimeofday(&tf, NULL);
	TiempoParcial= (tf.tv_sec - ti.tv_sec)*1000 + \
											(tf.tv_usec - ti.tv_usec)/1000.0;
	printf("Tracking: %5.4g ms\n", TiempoParcial);
	FrameData = NULL;
}

void Visualizacion(){

	irAlFinal( FramesBuf);
	FrameData = ( STFrame* )obtenerActual( FramesBuf );

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
			cvShowImage("Background", FrameData->BGModel);
//				cvShowImage( "Foreground",FrameData->FG);

	//		cvWaitKey(0);
	}
	if (SHOW_OPTICAL_FLOW == 1){
	cvShowImage( "Flujo Optico X", ImOpFlowX );
	cvShowImage( "Flujo Optico Y", ImOpFlowY);
	}
	if ( SHOW_MOTION_TEMPLATE == 1){
		cvShowImage( "Motion",FrameData->ImMotion);
		}
	;

	gettimeofday(&tff, NULL);
	TiempoFrame = (tff.tv_sec - tif.tv_sec)*1000 + \
			(tff.tv_usec - tif.tv_usec)/1000.0;
	TiempoGlobal = TiempoGlobal + TiempoFrame;
	printf("\n//////////////////////////////////////////////////\n");
	printf("\nTiempo de procesado del Frame %.0f : %5.4g ms\n",FrameCountAbs, TiempoFrame);
	printf("Segundos de video procesados: %.3f seg \n", TiempoGlobal/1000);
	printf("Porcentaje completado: %.2f %% \n",(FrameCountAbs/TotalFrames)*100 );
	printf("\n//////////////////////////////////////////////////\n");

	FrameData = NULL;
}



void AnalisisEstadistico(){
	printf( "Rastreo finalizado con éxito ..." );
	printf( "Comenzando análisis estadístico de los datos obtenidos ...\n" );
	printf( "Análisis finalizado ...\n" );
}

void Finalizar(){
	// completar. tras añadir buffers liberar memoria de todos los elementos (no solo del puntero actual)
	//liberar estructuras
	if(BGModel){
	cvReleaseImage(&BGModel->IDesv);
	cvReleaseImage(&BGModel->ImFMask);
	cvReleaseImage(&BGModel->Imed);
	free(BGModel);
	}
	if(Flat) free(Flat);
	if(FramesBuf) {liberarBuffer( FramesBuf );free( FramesBuf);}
	//liberar listas
	if(Flies) free( Flies );
	if(Fly) free(Fly);
	if( Shape) free(Shape);

	DeallocateImages( );
//	DeallocateImagesBGM();

	cvReleaseCapture(&g_capture);

	DestroyWindows( );
}

void InitNewFrameData(IplImage* I, STFrame *FrameData ){

	CvSize size = cvGetSize( I );
	FrameData->BGModel = cvCreateImage(size,8,1);
	FrameData->FG = cvCreateImage(size,8,1);
	FrameData->IDesv = cvCreateImage(size,8,1);
	FrameData->OldFG = cvCreateImage(size,8,1);
	FrameData->ImMotion = cvCreateImage( size, 8, 3 );
	FrameData->ImMotion->origin = I->origin;
	cvZero( FrameData->BGModel );
	cvZero( FrameData->FG );
	cvZero( FrameData->IDesv );
	cvZero( FrameData->ImMotion);
	FrameData->Flies = NULL;
	FrameData->num_frame = FrameCountAbs;
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


void AllocateImages( IplImage* I , StaticBGModel* bgmodel){

	// Crear imagenes y redimensionarlas en caso de que cambien su tamaño

	CvSize size = cvGetSize( I );

//	if( !FrameData->BGModel ||
//		 FrameData->BGModel->width != size.width ||
//		 FrameData->BGModel->height != size.height ) {
//
//		cvReleaseImage( &FrameData->BGModel );
//		cvReleaseImage( &FrameData->FG );
//		cvReleaseImage( &FrameData->IDesv );
//		cvReleaseImage( &FrameData->ImFMask );
//		cvReleaseImage( &FrameData->ImRois );
//		cvReleaseImage( &FrameData->OldFG );
//		cvReleaseImage( &FrameData->ImMotion );




		bgmodel->Imed = cvCreateImage(size,8,1);
		bgmodel->ImFMask = cvCreateImage(size,8,1);
		bgmodel->IDesv= cvCreateImage(size,8,1);

		cvZero( bgmodel->Imed);
		cvZero( bgmodel->IDesv);
		cvZero( bgmodel->ImFMask);


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

int RetryCap(){
	frame = cvQueryFrame(g_capture); // intentar de nuevo
	if ( !frame ) { // intentar de nuevo cn los siguientes frame
		double n_frame = cvGetCaptureProperty( g_capture, 1);
		int i = 1;
		printf("\n Fallo en captura del frame %0.1f",n_frame);
		while( !frame && i<4){
			printf("\n Intentándo captura del frame %0.1f",n_frame+i);
			cvSetCaptureProperty( g_capture,
								CV_CAP_PROP_POS_AVI_RATIO,
								n_frame +i );
			frame = cvQueryFrame(g_capture);
			if ( !frame ) printf("\n Fallo en captura");
//				FrameCountAbs++;
			i++;
		}
		if ( !frame ) {
			error(2);
			return 0;
		}
		else return 1;
	}
}

