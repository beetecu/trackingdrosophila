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

	struct timeval ti, tf, tif, tff; // iniciamos la estructura
	float TiempoInicial;
	float TiempoParcial;


	CvCapture* g_capture = NULL;/// puntero a una estructura de tipo CvCapture
	IplImage* frame;

	if( argc<1) {help(); return -1;};

	///////////  CAPTURA  ////////////
	gettimeofday(&ti, NULL);  //para obtener el tiempo
	TiempoInicial= ti.tv_sec*1000 + ti.tv_usec/1000.0;
	printf( "Iniciando captura..." );

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
	if (!Inicializacion(frame, argc, argv, &FramesBuf) ) return -1;

	gettimeofday(&tf, NULL);
	TiempoParcial= (tf.tv_sec - ti.tv_sec)*1000
			     + (tf.tv_usec - ti.tv_usec)/1000.0;
	printf(" %5.4g ms\n", TiempoParcial);

	//////////  PREPROCESADO   ////////////
	printf("\nIniciando preprocesado.");
	if (!PreProcesado( g_capture, &BGModel, &Shape) ) Finalizar();

	/*********** BUCLE PRINCIPAL DEL ALGORITMO ***********/
    for(int Fr = 1;frame; frame = cvQueryFrame(g_capture), Fr++ ){
    	/*Posteriormente  Escribir en un fichero log el error. Actualizar el contador
    	  de frames absolutos. */
    	if( !frame ) RetryCap( g_capture );
    	if( !RetryCap ) Finalizar( );

		if ( (cvWaitKey(10) & 255) == 27 ) break;
		NumFrame = cvGetCaptureProperty( g_capture, 1);

		if(NumFrame == 1443 ){
			printf("Hola");
		}
		gettimeofday(&tif, NULL);

		//////////  PROCESAR      ////////////
		//Procesado(frame, FramesBuf,BGModel, Shape );
		FrameData = Procesado2(frame, BGModel, Shape );
		anyadirAlFinal( FrameData, FramesBuf);
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
			//mostrarListaFlies(FramesBuf->numeroDeElementos-1,FramesBuf);// del ultimo elemento
			// calculo de datos estadísticos simples en tiempo de ejecución
//			CalcStatDataFrame();

//			VisualizarEl( PRIMERO , FramesBuf , BGModel );

			// guardar datos del primer frame en fichero
			if(!GuardarPrimero( FramesBuf, nombreFichero ) ){error(6);Finalizar();}
			// Liberar de memoria los datos del frame
			if(!liberarPrimero( FramesBuf ) ){error(7);Finalizar();}
			FrameData = NULL;
		}
		//else VisualizarEl( PRIMERO , FramesBuf , BGModel );
		gettimeofday(&tff, NULL);
		TiempoFrame = (tff.tv_sec - tif.tv_sec)*1000 + \
				(tff.tv_usec - tif.tv_usec)/1000.0;
		TiempoGlobal = TiempoGlobal + TiempoFrame;
		// calcular los tiempos del frame
		//			CalcDataFrame( FrameData )

		////////// ESTADISTICAS /////////////	//


		//////////  VISUALIZAR     ////////////
		//
		// incrustar datos en primer frame del buffer
		FrameData = (STFrame*) FramesBuf->ultimo->dato;
		ShowStatDataFr( FrameData->Frame);
//		ShowStatDataFr( FrameData->FG);//
		//visualizar primer frame del buffer
		VisualizarEl( 0, FramesBuf , BGModel );
		// visualizar ultimo frame del buffer
		if(FramesBuf->numeroDeElementos >0)
//		VisualizarEl(FramesBuf->numeroDeElementos-1, FramesBuf , BGModel );
		// FramesBuf->ultimo->siguiente->dato
 		printf("\n//////////////////////////////////////////////////\n");
		printf("\nTiempo de procesado del Frame %.0f : %5.4g ms\n",NumFrame, TiempoFrame);
		printf("Segundos de video procesados: %.3f seg \n", TiempoGlobal/1000);
		printf("Porcentaje completado: %.2f %% \n",(NumFrame/TotalFrames)*100 );
		printf("\n//////////////////////////////////////////////////\n");

	}
	///////// LIBERAR MEMORIA Y TERMINAR////////

    Finalizar();
	cvReleaseCapture(&g_capture);
	cvDestroyAllWindows( );
    ///////// POSTPROCESADO //////////
//	AnalisisEstadistico();

}

int Inicializacion( IplImage* frame,
					int argc,
					char* argv[],
					tlcde** FramesBuf){

	// Creación de ventanas de visualizacion
	CreateWindows( );

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
	//inicializar buffer de datos.
	tlcde* framesBuf = ( tlcde * )malloc( sizeof(tlcde));
	if( !framesBuf ) {error(4);Finalizar();}
	iniciarLcde( framesBuf );
	*FramesBuf = framesBuf;

	return 1;
}

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
	// liberar imagenes y datos de preprocesado
	releaseDataPreProcess();
	// liberar imagenes y datos de segmentacion
	ReleaseDataSegm( );
	// liberar imagenes y datos de procesado
	releaseDataProcess();
	//liberar listas
	if(Flies) free( Flies );

	// liberar imagenes y datos de tracking
	ReleaseDataTrack();

}




