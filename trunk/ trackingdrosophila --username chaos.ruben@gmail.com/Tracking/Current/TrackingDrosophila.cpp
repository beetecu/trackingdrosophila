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
char nombreVideo[30];


extern float TiempoFrame;
extern float TiempoGlobal;
extern double NumFrame ; /// contador de frames absolutos ( incluyendo preprocesado )
extern double TotalFrames ;
float TiempoFrame;
float TiempoGlobal;
double NumFrame ; /// contador de frames absolutos ( incluyendo preprocesado )
double TotalFrames ;

///HightGui
int g_slider_pos = 0;

/// MODELADO DE FONDO
StaticBGModel* BGModel = NULL;

// Modelado de forma
SHModel* Shape;

/// Estructura frame
STFrame* FrameData = NULL;
/// Buffer frames
tlcde *FramesBuf = NULL;

/// Estructura fly
STFly* Fly = NULL;
/// Lista flies
tlcde *Flies = NULL;

/// Estructura estadísticas
STStatFrame* Stats;

IplImage* gray1 = NULL;
	IplImage* gray2 = NULL;
	IplImage* gray3 = NULL;
void help(){
	printf("\n Para ejecutar el programa escriba en la consola: "
			"TrackingDrosophila [nombre_video.avi] [Nombre_Fichero]\n  "
			"Donde:\n - [nombre_video.avi] es el nombre del video a analizar. Ha de "
			"estar en formato avi. Se deberán tener instalados los codecs ffmpeg.\n"
			"[Nombre_Fichero] Será el nombre del fichero donde se guardarán los datos."
			"Si no se especifica se le solicitará uno al usuario. Si continúa sin especificarse"
			"se establecerá [Data_n] por defecto hasta un máximo de n = 29. ");
}
int main(int argc, char* argv[]) {

	struct timeval ti, tf, tif, tff,tinicio; // iniciamos la estructura

	float TiempoParcial;

	CvCapture* g_capture = NULL;/// puntero a una estructura de tipo CvCapture
	CvVideoWriter* VWriter;
	IplImage* frame;

	if( argc<1) {help(); return -1;};

	///////////  CAPTURA  ////////////

	printf( "Iniciando captura...\n" );

	g_capture = cvCaptureFromAVI( argv[1] );
	if ( !g_capture ) {
		error( 1 );
		help();
		return -1;
	}
	frame = cvQueryFrame( g_capture );

	///////////  INICIALIZACIÓN ////////////

	TotalFrames = cvGetCaptureProperty( g_capture, CV_CAP_PROP_FRAME_COUNT);
	if(!TotalFrames) TotalFrames = getAVIFrames("Drosophila.avi"); // en algun linux no funciona lo anterior
	TiempoGlobal = 0;
	NumFrame = 0;
	//inicializar parametros, buffer y fichero de datos
	printf("\nInicializando parámetros...");
	if (!Inicializacion(frame, argc, argv, &FramesBuf, &Stats) ) return -1;
	VWriter = iniciarAvi( g_capture, nombreVideo);
	//////////  PREPROCESADO   ////////////
	gettimeofday(&ti, NULL);//para obtener el tiempo transcurrido desde el inicio del programa
	printf("\nIniciando preprocesado...");

	if (!PreProcesado( g_capture, &BGModel, &Shape) ) Finalizar();

	TiempoParcial = obtenerTiempo( ti , 1);
	printf("\nPreprocesado correcto.Tiempo total: %0.2f s\n", TiempoParcial);
	printf("\n\nIniciando procesado...\n");
	gettimeofday(&tinicio, NULL);//para obtener el tiempo transcurrido desde el inicio del programa
	/*********** BUCLE PRINCIPAL DEL ALGORITMO ***********/
    for(int Fr = 1;frame; frame = cvQueryFrame(g_capture), Fr++ ){///////////////////////////////////
    	/*Posteriormente  Escribir en un fichero log el error. Actualizar el contador
    	  de frames absolutos. */
    	if( !frame ) RetryCap( g_capture );
    	if( !RetryCap ) Finalizar( );
    	gettimeofday(&tif, NULL);
		if ( (cvWaitKey(10) & 255) == 27 ) break;
		NumFrame = cvGetCaptureProperty( g_capture, 1);
 		if(Fr>1){
 			printf("\n//////////////////////////////////////////////////\n");
 			printf("\nTiempo de procesado del  Frame (incluida visualización) %.0f : %5.4g ms\n",NumFrame-1, TiempoFrame);
			printf("Segundos de video procesados: %.3f seg \n", TiempoGlobal/1000);
			printf("Porcentaje completado: %.2f %% \n",((NumFrame-1)/TotalFrames)*100 );
			printf("\n//////////////////////////////////////////////////\n");
 		}
		gettimeofday(&ti, NULL);
		printf("\n//////////////////////////////////////////////////\n");
		printf("\t\t\tFRAME %0.f ",NumFrame);
		//////////  PROCESAR      ////////////
		printf("\n1)Procesado:\n");

	//	FrameData = Procesado(frame, BGModel, Shape );

		FrameData = Procesado2(frame, BGModel, Shape );

//		IplImage* mascara = NULL;
//		if(!mascara) mascara = cvCreateImage( cvGetSize( frame ),8,3);
//		cvZero( mascara);
//		crearMascara( FrameData->Frame, FrameData->FG,mascara);
//		cvShowImage("mascara",mascara);
//		if(GRABAR_VISUALIZACION){
//						 cvWriteFrame( VWriter,mascara);
//		}

	//	FrameData = Procesado4(frame, BGModel,bg_model, Shape );
//		muestrearLinea( FrameData->Frame,cvPoint( 0, 240 ),cvPoint( 640, 240 ), 20);
		TiempoParcial = obtenerTiempo( ti , NULL);
		printf("Procesado correcto.Tiempo total %5.4g ms\n", TiempoParcial);
		anyadirAlFinal( FrameData, FramesBuf);

		//////////  RASTREAR       ////////////
		gettimeofday(&ti, NULL);
		printf("\n2)Tracking:\n");

		Tracking( FramesBuf );

		TiempoParcial = obtenerTiempo( ti , NULL);
		printf("Tracking correcto.Tiempo total %5.4g ms\n", TiempoParcial);

		//////////  ALMACENAR ////////////
		// Se mantienen en memoria las estructuras correspondientes a STRUCT_BUFFER_LENGTH frames
		// ( buffer de datos  ) e IMAGE_BUFFER_LENGHT ( buffer de imagenes ) .
		// Los buffers son colas FIFO
		// si buffer lleno
		if( FramesBuf->numeroDeElementos == STRUCT_BUFFER_LENGTH){
			// obtener primero
			FrameData = (STFrame*) FramesBuf->ultimo->siguiente->dato;
			//mostrarListaFlies(FramesBuf->numeroDeElementos-1,FramesBuf);// del ultimo elemento
			// calculo de datos estadísticos simples en tiempo de ejecución
//

//			VisualizarEl( PRIMERO , FramesBuf , BGModel );

			// guardar datos del primer frame en fichero
			if(!GuardarPrimero( FramesBuf, nombreFichero ) ){error(6);Finalizar();}
			// Liberar de memoria los datos del frame
			if(!liberarPrimero( FramesBuf ) ){error(7);Finalizar();}
			FrameData = NULL;
		}
		//else VisualizarEl( PRIMERO , FramesBuf , BGModel, g_capture );

		// calcular los tiempos del frame
		//			CalcDataFrame( FrameData )

		////////// ESTADISTICAS /////////////	//
		gettimeofday(&ti, NULL);
		printf("\n3)Cálculo de estadísticas en tiempo de ejecución:\n");

//		CalcStatDataFrame();

		TiempoParcial = obtenerTiempo( ti , NULL);
		printf("Cálculos realizados. Tiempo total %5.4g ms\n", TiempoParcial);

		TiempoFrame = obtenerTiempo( tif, 0 );

		//////////  VISUALIZAR     ////////////
		//
		gettimeofday(&ti, NULL);
		printf("\n4)Visualización:\n");
		// incrustar datos en primer frame del buffer
		FrameData = (STFrame*) FramesBuf->ultimo->dato;
		ShowStatDataFr( FrameData->Frame);

//		ShowStatDataFr( FrameData->FG);//
		//visualizar primer frame del buffer
// 		VisualizarEl( 0, FramesBuf , BGModel );
		// visualizar ultimo frame del buffer
		if(FramesBuf->numeroDeElementos >0)
   		VisualizarEl(FramesBuf->numeroDeElementos-1, FramesBuf , BGModel, g_capture );

		TiempoParcial = obtenerTiempo( ti , NULL);
		printf("Visualización finalizada.Tiempo total %5.4g ms\n", TiempoParcial);

		TiempoFrame = TiempoFrame + obtenerTiempo( tif, 0 );
		TiempoGlobal = obtenerTiempo( tinicio, 1);

		// FramesBuf->ultimo->siguiente->dato

	}
	///////// LIBERAR MEMORIA Y TERMINAR////////

    Finalizar();
	cvReleaseCapture(&g_capture);
	cvReleaseVideoWriter( &VWriter);
	DestroyWindows( );
    ///////// POSTPROCESADO //////////
//	AnalisisEstadistico();

}

int Inicializacion( IplImage* frame,
					int argc,
					char* argv[],
					tlcde** FramesBuf,
					STStatFrame** Stats){

	// Creación de ventanas de visualizacion
	CreateWindows( );

	/// Crear fichero de datos.
	/// si no se especifica nombre en la ejecución establecerlo por defecto
	if( argc == 2 ){
		for( int i = 0; i < 1000; i++ ){
			sprintf(nombreFichero,"Data_%d ",i);
			if( !existe( nombreFichero) ) {
				crearFichero( nombreFichero );
				break;
		    }
		}
		for( int i = 0; i < 1000; i++ ){
			sprintf(nombreVideo,"Visual_%d.avi",i);
			if( !existe( nombreVideo) ) {
				break;
			}
		}
	}
	// si se especifica nombre de fichero de datos
	if( argc == 3 ){
		if( existe(argv[2]) ){ // verificar si el nombre del archivo de datos existe.
			printf("\nEl fichero existe ¿desea sobrescribirlo? (s/n): ");
			char resp[1];
			do
			{
			  fgets(resp,1,stdin);
			  fflush(stdin);
			  QuitarCR( resp );
			  if (resp[0] == 's'||'S')  crearFichero(argv[2]);
			  else if (resp[0] == 'n'||'N'){
				  printf("\nEscriba el nuevo nombre o pulse intro para Data_n como nombre por defecto: ");
				  fgets(nombreFichero, 30, stdin );
				  fflush(stdin);
				  QuitarCR( nombreFichero );
				  if (strlen(nombreFichero)<1) {// nombre por defecto
					for( int i = 0; i < 1000; i++ ){
						sprintf(nombreFichero,"Data_%d ",i);
						if( !existe( nombreFichero) ) {
							crearFichero( nombreFichero );
							break;
						}
					}
				 } //fin nombre por defecto
			  }
			  else printf("\nResponda s/n.El fichero existe ¿desea sobrescribirlo?:");

			}
			while (resp[0] != 's'||'S'||'n'||'N');
		}
		else{
			crearFichero( argv[2] );
		}
	} // si se especifica nombre de fichero de video
	if( argc == 4 ){
		if( existe(argv[4]) ){ // verificar si el nombre del fichero existe.
			printf("\nEl fichero existe ¿desea sobrescribirlo? (s/n): ");
			char resp[1];
			do
			{
			  fgets(resp,1,stdin);
			  fflush(stdin);
			  QuitarCR( resp );
			  if (resp[0] == 's'||'S') continue; // sobreescribir
			  else if (resp[0] == 'n'||'N'){
				  printf("\nEscriba el nuevo nombre o pulse intro para Visual_n como nombre por defecto: ");

				  fgets(nombreVideo, 30, stdin );
				  fflush(stdin);
				  QuitarCR( nombreVideo );
				  if (strlen(nombreVideo)<1) {// nombre por defecto
					  for( int i = 0; i < 1000; i++ ){
							sprintf(nombreVideo,"Visual_%d.avi",i);
							if( !existe( nombreVideo) ) {
								// eliminamos la extensión
								//char *p = strstr(nombreVideo,".avi");
								//p[0]= '\0';
								break;
							}
						}
				 } //fin nombre por defecto
			  }
			  else printf("\nResponda s/n.El fichero existe ¿desea sobrescribirlo?:");

			}
			while (resp[0] != 's'||'S'||'n'||'N');
		} // si no existe verificamos que el nombre tenga la extensión correcta
//		else {
//			char *nombre;
//			if(nombre = strstr( nombreVideo, ".avi"));
//
//		}
	}
	//inicializar buffer de datos.
	tlcde* framesBuf = ( tlcde * )malloc( sizeof(tlcde));
	if( !framesBuf ) {error(4);Finalizar();}
	iniciarLcde( framesBuf );
	*FramesBuf = framesBuf;

	// Inicializar estructura de analisis estadístico

	STStatFrame* stats = ( STStatFrame * )malloc( sizeof(STStatFrame));
		if( !stats ) {error(4);Finalizar();}
	*Stats = stats;
//	if( argc == 4 ) setMouseCallback( "CamShift Demo", onMouse, 0 );

	return 1;
}
//void onMouse( int event, int x, int y, int, void* )
//{
//    if( selectObject )
//    {
//        selection.x = MIN(x, origin.x);
//        selection.y = MIN(y, origin.y);
//        selection.width = std::abs(x - origin.x);
//        selection.height = std::abs(y - origin.y);
//
//        selection &= Rect(0, 0, image.cols, image.rows);
//    }
//
//    switch( event )
//    {
//    case CV_EVENT_LBUTTONDOWN:
//        origin = Point(x,y);
//        selection = Rect(x,y,0,0);
//        selectObject = true;
//        break;
//    case CV_EVENT_LBUTTONUP:
//        selectObject = false;
//        if( selection.width > 0 && selection.height > 0 )
//            trackObject = -1;
//        break;
//    }
//}
void AnalisisEstadistico(){
	printf( "Rastreo finalizado con éxito ..." );
	printf( "Comenzando análisis estadístico de los datos obtenidos ...\n" );
	printf( "Análisis finalizado ...\n" );
}

void Finalizar(){

	//liberar buffer
	if(FramesBuf) {
		liberarBuffer( FramesBuf );
		free( FramesBuf);
	}
	//liberar estructuras
	DeallocateBGM( BGModel );
	if( FrameData ) liberarSTFrame( FrameData);
	if(Fly) free(Fly);
	if( Shape) free(Shape);
	if( Stats ) free(Stats);
	// liberar imagenes y datos de preprocesado
	releaseDataPreProcess();
	// liberar imagenes y datos de segmentacion

	// liberar imagenes y datos de procesado
	releaseDataProcess();
	//liberar listas
	if(Flies) free( Flies );

	// liberar imagenes y datos de tracking
	ReleaseDataTrack();

}

void crearMascara( IplImage* Frame, IplImage* FG,IplImage* mascara){



	if(!gray1){
		gray1 = cvCreateImage( cvGetSize( Frame ),8,1);
		gray2 = cvCreateImage( cvGetSize( Frame ),8,1);
		gray3 = cvCreateImage( cvGetSize( Frame ),8,1);
	}
	cvSplit(Frame, gray1,gray2, gray3,0);
	invertirBW(FG);
	cvSet(gray1, cvScalar(0), FG );
	cvSet(gray2, cvScalar(0), FG );
	cvSet(gray3, cvScalar(0), FG );
	invertirBW(FG);
	cvMerge(gray1,gray2, gray3, NULL, mascara);
}


