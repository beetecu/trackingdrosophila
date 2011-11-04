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

char nombreFichero[30];

struct timeval ti, tf, tif, tff; // iniciamos la estructura
float TiempoInicial;
float TiempoParcial;
float TiempoFrame;
extern float TiempoGlobal = 0;
extern double NumFrame = 0; /// contador de frames absolutos ( incluyendo preprocesado )


double TotalFrames = 0;
CvCapture *g_capture ; /// puntero a una estructura de tipo CvCapture

///HightGui
int g_slider_pos = 0;

/// MODELADO DE FONDO
StaticBGModel* BGModel = NULL;
BGModelParams *BGParams = NULL;
CvBGStatModel* Dbg_model = 0;

/// Estructura frame
STFrame* FrameData = NULL;

/// Buffer frames
tlcde *FramesBuf = NULL;

/// Estructura fly
STFly* Fly = NULL;
/// Lista flies
tlcde *Flies = NULL;


// Modelado de forma

SHModel* Shape;

/// Imagenes que se usarán en el programa principal
/// CAPTURA
/// Imagenes RGB 3 canales
IplImage* frame;



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

	//inicializar parametros, buffer y fichero de datos
	if (!Inicializacion(frame, &Shape, &BGParams, argc, argv) ) return -1;

	//inicializar buffer de datos.
	FramesBuf = ( tlcde * )malloc( sizeof(tlcde));
	if( !FramesBuf ) {error(4);FinalizarTracking();}
	iniciarLcde( FramesBuf );

	gettimeofday(&tf, NULL);
	TiempoParcial= (tf.tv_sec - ti.tv_sec)*1000
			     + (tf.tv_usec - ti.tv_usec)/1000.0;
	printf(" %5.4g ms\n", TiempoParcial);

	//////////  PREPROCESADO   ////////////
	printf("\nIniciando preprocesado.");
	if (!PreProcesado( ) ) FinalizarTracking();

	/*********** BUCLE PRINCIPAL DEL ALGORITMO ***********/
    for(int Fr = 1;frame; frame = cvQueryFrame(g_capture), Fr++ ){
    	/*Posteriormente  Escribir en un fichero log el error. Actualizar el contador
    	  de frames absolutos. */
    	if( !frame ) RetryCap();
    	if( !RetryCap ) FinalizarTracking();

		if ( (cvWaitKey(10) & 255) == 27 ) break;
		NumFrame = cvGetCaptureProperty( g_capture, 1);

		gettimeofday(&tif, NULL);

		//////////  PROCESAR      ////////////
		//Procesado(frame, FramesBuf,BGModel, Shape );
		Procesado2(frame, FramesBuf,BGModel, Shape );

		//////////  RASTREAR       ////////////
		Tracking( FramesBuf );

		//////////  ALMACENAR ////////////
		// Se mantienen en memoria las estructuras correspondientes a STRUCT_BUFFER_LENGTH frames
		// ( buffer de datos  ) e IMAGE_BUFFER_LENGHT ( buffer de imagenes ) .
		// Los buffers son colas FIFO

		// si buffer lleno
		if( FramesBuf->numeroDeElementos == STRUCT_BUFFER_LENGTH){
			// obtener primero
			FrameData = (STFrame*) FramesBuf->ultimo->siguiente->dato;
			mostrarListaFlies(FrameData->Flies);// del ultimo elemento
			// calculo de datos estadísticos simples en tiempo de ejecución
//			CalcStatDataFrame();

//			VisualizarEl( PRIMERO , FramesBuf , BGModel );

			// guardar datos del primer frame en fichero
			if(!GuardarPrimero( FramesBuf, nombreFichero ) ){error(6);FinalizarTracking();}
			// Liberar de memoria los datos del frame
			if(!liberarPrimero( FramesBuf ) ){error(7);FinalizarTracking();}
			FrameData = NULL;
		}
		gettimeofday(&tff, NULL);
		TiempoFrame = (tff.tv_sec - tif.tv_sec)*1000 + \
				(tff.tv_usec - tif.tv_usec)/1000.0;
		TiempoGlobal = TiempoGlobal + TiempoFrame;
		// calcular los tiempos del frame
		//			CalcDataFrame( FrameData )

		////////// ESTADISTICAS /////////////
		//


		//////////  VISUALIZAR     ////////////
		//
		// incrustar datos en primer frame del buffer
		FrameData = (STFrame*) FramesBuf->ultimo->dato;
		ShowStatDataFr( FrameData->Frame);
//		ShowStatDataFr( FrameData->FG);//
		//visualizar primer frame del buffer
//		VisualizarEl( 0, FramesBuf , BGModel );
		// visualizar ultimo frame del buffer
		VisualizarEl(FramesBuf->numeroDeElementos-1, FramesBuf , BGModel );
		// FramesBuf->ultimo->siguiente->dato
		printf("\n//////////////////////////////////////////////////\n");
		printf("\nTiempo de procesado del Frame %.0f : %5.4g ms\n",NumFrame, TiempoFrame);
		printf("Segundos de video procesados: %.3f seg \n", TiempoGlobal/1000);
		printf("Porcentaje completado: %.2f %% \n",(NumFrame/TotalFrames)*100 );
		printf("\n//////////////////////////////////////////////////\n");


	}
	///////// LIBERAR MEMORIA Y TERMINAR////////
	FinalizarTracking();
    ///////// POSTPROCESADO //////////
//	AnalisisEstadistico();

}


int Inicializacion( IplImage* frame,
					SHModel** Shape,
					BGModelParams** BGParams,
					int argc,
					char* argv[]){
	// Creación de ventanas de visualizacion
	CreateWindows( );
	//      cvSetCaptureProperty( g_capture	cvResetImageROI(Capa->BGModel);, CV_CAP_PROP_POS_AVI_RATIO,0 );
	//      Añadimos un slider a la ventana del video Drosophila.avi
	//              int TotalFrames = getAVIFrames("Drosophila.avi"); // en algun linux no funciona lo anterior

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

	// iniciar estructura para modelo de fondo dinamico
//	if(!bg_model)
//		{
//			//create BG model
//			Dbg_model = cvCreateGaussianBGModel( tmp_frame );
//			//bg_model = cvCreateFGDStatModel( temp );
//			continue;
//		}
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

	int hecho = 0;

// Crear Modelo de fondo estático .Solo en la primera ejecución

	printf("Creando modelo de fondo..... ");
	gettimeofday(&ti, NULL);
	// establecer parametros
	InitialBGModelParams( BGParams);

	BGModel = initBGModel( g_capture , BGParams );
	if(!BGModel) FinalizarTracking();

	gettimeofday(&tf, NULL);
	TiempoParcial= (tf.tv_sec - ti.tv_sec)*1000 + \
									(tf.tv_usec - ti.tv_usec)/1000.0;
	TiempoGlobal = TiempoGlobal + TiempoParcial ;
	printf(" %5.4g segundos\n", TiempoGlobal/1000);
	TiempoGlobal = 0; // inicializamos el tiempo global

	printf("Creando modelo de forma..... ");
	gettimeofday(&ti, NULL);

	ShapeModel( g_capture, Shape , BGModel->ImFMask, BGModel->DataFROI );

	gettimeofday(&tf, NULL);
	TiempoParcial= (tf.tv_sec - ti.tv_sec)*1000 + \
									(tf.tv_usec - ti.tv_usec)/1000.0;
	TiempoGlobal= TiempoGlobal + TiempoParcial ;
	printf(" %5.4g seg\n", TiempoGlobal/1000);
	printf("Fin preprocesado. Iniciando procesado...\n");
	TiempoGlobal = 0;

	NumFrame = cvGetCaptureProperty( g_capture, 1 ); //Actualizamos los frames

	return hecho = 1;
}
void ShowStatDataFr( IplImage* Im  ){

	CvFont fuente1;
	CvFont fuente2;

	char NFrame[100];
	CvPoint NFrameO;
	char TProcesF[100];
	CvPoint TProcesFO;
	char TProces[100];
	CvPoint TProcesO;
	char PComplet[100];
	CvPoint PCompletO;
	char FPS[100];
	CvPoint FPSO;

	sprintf(NFrame,"Frame %.0f ",NumFrame);
	sprintf(TProcesF,"Tiempo de procesado del Frame : %5.4g ms", TiempoFrame);
	sprintf(TProces,"Segundos de video procesados: %.3f seg ", TiempoGlobal/1000);
	sprintf(PComplet,"Porcentaje completado: %.2f %% ",(NumFrame/TotalFrames)*100 );
	sprintf(FPS,"FPS: %.2f ",(1000/TiempoFrame));

	cvInitFont( &fuente1, CV_FONT_HERSHEY_PLAIN, 1, 1, 0, 1, 8);
	cvInitFont( &fuente2, CV_FONT_HERSHEY_PLAIN, 0.5, 0.5, 0, 1, 8);

	NFrameO.x = 10;
	NFrameO.y = 20;
	cvPutText( Im, NFrame, NFrameO, &fuente1, CVX_WHITE );

	TProcesFO.x = 10;
	TProcesFO.y = 40;
	cvPutText( Im, TProcesF, TProcesFO, &fuente2, CVX_GREEN );

	TProcesO.x = 10;
	TProcesO.y = 60;
	cvPutText( Im, TProces, TProcesO, &fuente2, CVX_GREEN);

	PCompletO.x = 10;
	PCompletO.y = 80;
	cvPutText( Im, PComplet, PCompletO, &fuente2, CVX_GREEN);

	FPSO.x = 10;
	FPSO.y = 100;
	cvPutText( Im, FPS, FPSO, &fuente1, CVX_WHITE);
}


void AnalisisEstadistico(){
	printf( "Rastreo finalizado con éxito ..." );
	printf( "Comenzando análisis estadístico de los datos obtenidos ...\n" );
	printf( "Análisis finalizado ...\n" );
}

void FinalizarTracking(){
	// completar. tras añadir buffers liberar memoria de todos los elementos (no solo del puntero actual)
	//liberar estructuras
	DeallocateBGM( BGModel );
	ReleaseDataSegm( );
	DeallocateTrackIm();
	if(FramesBuf) {liberarBuffer( FramesBuf );free( FramesBuf);}
	//liberar listas
	if(Flies) free( Flies );
	if(Fly) free(Fly);
	if( Shape) free(Shape);


//	DeallocateTempImages();

	cvReleaseCapture(&g_capture);

	DestroyWindows( );
}

void InitialBGModelParams( BGModelParams* Params){
	 static int first = 1;
	 if ( DETECTAR_PLATO ) Params->FLAT_FRAMES_TRAINING = 50;
	 else Params->FLAT_FRAMES_TRAINING = 0;
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

			i++;
		}
		if ( !frame ) {
			error(2);
			return 0;
		}
		else return 1;
	}
	else return 1;
}

