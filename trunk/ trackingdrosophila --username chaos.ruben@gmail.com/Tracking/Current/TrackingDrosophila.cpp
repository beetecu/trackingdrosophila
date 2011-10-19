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
	if (!Inicializacion(frame, &Flat, &Shape, &BGParams,&BGModel, argc, argv) ) return -1;
	//inicializar buffer de datos.
	FramesBuf = ( tlcde * )malloc( sizeof(tlcde));
	if( !FramesBuf ) error(4);
	iniciarLcde( FramesBuf );


	gettimeofday(&tf, NULL);
	TiempoParcial= (tf.tv_sec - ti.tv_sec)*1000
			     + (tf.tv_usec - ti.tv_usec)/1000.0;
	printf(" %5.4g ms\n", TiempoParcial);

	/*********** BUCLE PRINCIPAL DEL ALGORITMO ***********/
    for( FrameCountRel = 1;frame; frame = cvQueryFrame(g_capture), FrameCountRel++ ){
    	/*Posteriormente  Escribir en un fichero log el error. Actualizar el contador
    	  de frames absolutos. */
    	if( !frame ) frame = cvQueryFrame(g_capture); // intentar de nuevo
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
				break;
			}
		}
		if ( (cvWaitKey(10) & 255) == 27 ) break;
		FrameCountAbs = cvGetCaptureProperty( g_capture, 1);
//		FrameCountAbs += 1;
		UpdateCount += 1;
		gettimeofday(&tif, NULL);

		//////////  PREPROCESADO   ////////////
		if (!PreProcesado( ) ) return -1;
		//////////  PROCESADO      ////////////
		Procesado( );
		//////////  RASTREAR       ////////////
		Tracking(  );
		//////////  VISUALIZAR     ////////////
		Visualizacion();
		//////////  GUARDAR DATOS ////////////
		// Se mantienen en memoria las estructuras correspondientes a STRUCT_BUFFER_LENGTH frames
		// ( buffer de datos) e IMAGE_BUFFER_LENGHT ( buffer de imagenes ).
		if( FramesBuf->numeroDeElementos == STRUCT_BUFFER_LENGTH){
			FrameData = (STFrame*) FramesBuf->ultimo->dato;
			mostrarListaFlies(FrameData->Flies);
			// Una vez que se llenan los buffer se almacenan los datos del primer frame en fichero

			if(!GuardarPrimero( FramesBuf, nombreFichero ) ){error(6);exit(-1);}
			if(!liberarPrimero( FramesBuf ) ){error(7);exit(-1);};
			FrameData = NULL;
		}
	}
    ///////// POSTPROCESADO //////////
	AnalisisEstadistico();

	///////// LIBERAR MEMORIA ////////

	// completar. tras añadir buffers liberar memoria de todos los elementos (no solo del puntero actual)
	//liberar estructuras
	cvReleaseImage(&BGModel->IDesv);
	cvReleaseImage(&BGModel->ImFMask);
	cvReleaseImage(&BGModel->Imed);
	free(BGModel);
	free(Flat);
	liberarBuffer( FramesBuf );
	//liberar listas
	free( FramesBuf);
	free( Flies );


	DeallocateImages( );
//	DeallocateImagesBGM();

	cvReleaseCapture(&g_capture);

	DestroyWindows( );

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

int existe(char *nombreFichero)
{
  FILE *pf = NULL;
  // Verificar si el fichero existe
  int exis = 0; // no existe
  if ((pf = fopen(nombreFichero, "r")) != NULL)
  {
    exis = 1;   // existe
    fclose(pf);
  }
  return exis;
}

void crearFichero(char *nombreFichero )
{
  FILE *pf = NULL;
  // Abrir el fichero nombreFichero para escribir "w"
  if ((pf = fopen(nombreFichero, "wb")) == NULL)
  {
    printf("El fichero no puede crearse.");
    exit(1);
  }
 fclose(pf);
}

int PreProcesado( ){

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
	// Iniciamos la estructura capas para el procesado

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


///! El algoritmo mantiene un buffer de 50 frames ( 2 seg aprox ) para los datos y para las
///! imagenes.
///! Una vez que se han llenado los buffer, se almacena en fichero los datos de la lista
///! Flies correspondientes al primer frame. La pisición actual no se modifica.
///! Si la lista está vacía mostrará un error. Si se ha guardado con éxito devuelve un uno
int GuardarPrimero( tlcde* framesBuf , char *nombreFichero){


	tlcde* Flies = NULL;
	FILE * pf;
	STFly* fly = NULL;
	STFrame* frameData = NULL;
	int posicion = framesBuf->posicion;

	if (framesBuf->numeroDeElementos == 0) {printf("\nLista vacia\n");return 0;}

	// obtenemos la direccion del primer elemento
	irAlPrincipio( framesBuf );
	frameData = (STFrame*)obtenerActual( framesBuf );
	//obtenemos la lista
	Flies = frameData->Flies;
	int i = 0, tam = Flies->numeroDeElementos;
	// Abrir el fichero nombreFichero para escribir "w".
	if ((pf = fopen(nombreFichero, "a")) == NULL)
	{
	printf("El fichero no puede abrirse.");
	exit(1);
	}
	while( i < tam ){
		fly = (STFly*)obtener(i, Flies);
		fwrite(&fly, sizeof(STFly), 1, pf);
		if (ferror(pf))
		{
		  perror("Error durante la escritura");
		  exit(2);
		}
		i++;
	}
	fclose(pf);
	irAl( posicion, framesBuf);
	return 1;
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

void mostrarListaFlies(tlcde *lista)
{
		// Mostrar todos los elementos de la lista
	int i = 0, tam = lista->numeroDeElementos;
	STFly* flydata = NULL;
	while( i < tam ){
		flydata = (STFly*)obtener(i, lista);
		printf( "etiqueta %d\nColor\nposicion\n a %0.2f b %0.2f\norientacion %0.1f\nperimetro\nStatic %d\n num_frame %d\n;",
				flydata->etiqueta,
				flydata->a,
				flydata->b,
				flydata->orientacion,
				flydata->Static,
				flydata->num_frame);
		i++;
	}
	if (tam = 0 ) printf(" Lista vacía\n");
}

void liberarListaFlies(tlcde *lista)
{
  // Borrar todos los elementos de la lista
  STFly *flydata = NULL;

  // borrar: borra siempre el elemento actual
  irAlPrincipio(lista);
  flydata = (STFly *)borrar(lista);
  while (flydata)
  {
    free(flydata); // borrar el área de datos del elemento eliminado
    flydata = (STFly *)borrar(lista);
  }
}

///! Borra y libera el espacio del primer elemento del buffer ( el frame mas antiguo )
///! El puntero actual seguirá apuntando al mismo elemento que apuntaba antes de llamar
///! a la función.
int liberarPrimero(tlcde *FramesBuf ){

	STFrame *frameData = NULL;
	STFrame *frDataTemp = NULL;

//Guardamos la posición actual.
	int i = FramesBuf->posicion;
//
	if(!irAl(0, FramesBuf) ) {printf("\nBuffer vacio");return 0;}
	frameData = (STFrame*)obtenerActual( FramesBuf );
	 // por cada nuevo frame se libera el espacio del primer frame
	liberarListaFlies( frameData->Flies );
	free( frameData->Flies);
	//borra el primer elemento
	cvReleaseImage(&frameData->BGModel);
	cvReleaseImage(&frameData->FG);
	cvReleaseImage(&frameData->IDesv);
	cvReleaseImage(&frameData->ImMotion);
	cvReleaseImage(&frameData->OldFG);
	frameData = (STFrame *)borrar( FramesBuf );
	if( !frameData ) {
		printf( "Se ha borrado el último elemento.Buffer vacio" );
		free( frameData );
		return 0;
	}
	else{
		free( frameData );
		irAl(i - 1, FramesBuf);
		return 1;
	}
}

void liberarBuffer(tlcde *FramesBuf)
{
  // Borrar todos los elementos del buffer
  STFrame *frameData = NULL;

  // borrar: borra siempre el elemento actual
  irAlPrincipio(FramesBuf);
  frameData = (STFrame *)borrar( FramesBuf );
  while (frameData)
  {
	liberarListaFlies( frameData->Flies);
	cvReleaseImage(&frameData->BGModel);
	cvReleaseImage(&frameData->FG);
	cvReleaseImage(&frameData->IDesv);
	cvReleaseImage(&frameData->ImMotion);
	cvReleaseImage(&frameData->OldFG);
    free(frameData); // borrar el área de datos del elemento eliminado
    frameData = (STFrame *)borrar( FramesBuf );
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
