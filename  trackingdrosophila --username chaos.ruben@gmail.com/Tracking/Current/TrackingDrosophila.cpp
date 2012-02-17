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

extern float TiempoFrame;
extern float TiempoGlobal;
extern double NumFrame ; /// contador de frames absolutos ( incluyendo preprocesado )
extern double TotalFrames ;
float TiempoFrame;
float TiempoGlobal;
double NumFrame ; /// contador de frames absolutos ( incluyendo preprocesado )
double TotalFrames ;


/// PREPROCESADO
//Modelado de fondo
StaticBGModel* BGModel = NULL;
// Modelado de forma
SHModel* Shape;

// PROCESADO
/// Estructura fly
STFly* Fly = NULL;
/// Lista flies
tlcde *Flies = NULL;
/// Estructura frame
STFrame* FrameDataIn = NULL;
STFrame* FrameDataOut = NULL;
///Parámetros fondo para procesado
BGModelParams *BGPrParams = NULL;
///Parámetros Validación para procesado
ValParams* valParams = NULL;

///TRACKING
/// Buffer frames
tlcde *FramesBuf = NULL;

/// Estructura estadísticas
STStatFrame* Stats;

///HightGui
int g_slider_pos = 0;
// parametros de visualización
VisParams* visParams = NULL;

int main(int argc, char* argv[]) {

	struct timeval ti,  tif, tinicio; // iniciamos la estructura de medida de tiempos

	char nombreFichero[30];
	char nombreVideo[30];
//	char* Ficheros[4];

	if( argc<1) {help(); return -1;};

	///////////  CAPTURA  ////////////

	CvCapture* g_capture = NULL;/// puntero a una estructura de tipo CvCapture
	CvVideoWriter* VWriter;
	IplImage* frame;

	///////////  INICIALIZACIÓN ////////////
	printf("\nInicializando parámetros...");
	if(SHOW_WINDOW) DraWPresent(  );

	TiempoGlobal = 0;
	NumFrame = 0;
	TiempoFrame = 0;
	if (!Inicializacion( argc, argv,nombreFichero,nombreVideo ) ) return -1;

	//////////  PREPROCESADO   ////////////

	if (!PreProcesado( argv[1], &BGModel, &Shape) )  Finalizar(&g_capture, &VWriter);

	printf( "Iniciando captura...\n" );
	gettimeofday(&tinicio, NULL);//para obtener el tiempo transcurrido desde el inicio del programa
	g_capture = cvCaptureFromAVI( argv[1] );
	if ( !g_capture ) {
		error( 1 );
		help();
		return -1;
	}
	VWriter = iniciarAvi( g_capture, nombreVideo);
	// Creación de ventanas de visualizacion
	AllocDefaultVisParams(&visParams, BGModel->Imed);

	TotalFrames = cvGetCaptureProperty( g_capture, CV_CAP_PROP_FRAME_COUNT);
	if(!TotalFrames) TotalFrames = getAVIFrames("Drosophila.avi"); // en algun linux no funciona lo anterior
	printf("\n\nIniciando procesado...\n");
	if(SHOW_WINDOW) Transicion("Iniciando Tracking...", 1,1000, 50 );
	/*********** BUCLE PRINCIPAL DEL ALGORITMO ***********/

    for(int Fr = 1;; Fr++ ){

    	frame = cvQueryFrame(g_capture);
    	/*Posteriormente  Escribir en un fichero log el error. Actualizar el contador
    	  de frames absolutos. */
    	gettimeofday(&tif, NULL);
    	if( !frame ) RetryCap( g_capture );
    	if( !frame ) Finalizar(&g_capture, &VWriter);

		if ( (cvWaitKey(10) & 255) == 27 ) break;
		NumFrame = cvGetCaptureProperty( g_capture, 1);
 		if(FramesBuf && FramesBuf->numeroDeElementos>0){
 			irAl(FramesBuf->numeroDeElementos-1, FramesBuf );
 			FrameDataIn = (STFrame*)obtenerActual(FramesBuf);
 			printf("\n//////////////////////////////////////////////////\n");
 			printf("\nTiempo de procesado del  Frame (incluida visualización) %.0f : %5.4g ms\n",NumFrame-1, FrameDataIn->Stats->TiempoFrame);
			printf("Segundos de video procesados: %0.f seg \n", FrameDataIn->Stats->TiempoGlobal);
			printf("Porcentaje completado: %.2f %% \n",((NumFrame-1)/TotalFrames)*100 );
			printf("\n//////////////////////////////////////////////////\n");
 		}
		gettimeofday(&ti, NULL);
		printf("\n//////////////////////////////////////////////////\n");
		printf("\t\t\tFRAME %0.f ",NumFrame);

		//////////  PROCESAR     ////////////
		FrameDataIn = Procesado2(frame, BGModel, Shape, valParams, BGPrParams );

		//////////  RASTREAR       ////////////
		Tracking( FrameDataIn, &FramesBuf );

		// SI BUFFER LLENO
		if( FramesBuf->numeroDeElementos == MAX_BUFFER){

			////////// ESTADISTICAS //////////
			// para su cálculo usamos el frameDataOut anterior
			if( FrameDataOut ) CalcStatsFrame( FrameDataOut,FramesBuf );

			////////// LIBERAR MEMORIA  ////////////
			if(FrameDataOut) liberarSTFrame( FrameDataOut );
			FrameDataOut = (STFrame*)liberarPrimero( FramesBuf ) ;
			if(!FrameDataOut){error(7); Finalizar(&g_capture, &VWriter);}

			//////////  VISUALIZAR     ////////////
//			VisualizarFr( FrameDataOut , BGModel, VWriter );
			if(SHOW_WINDOW) DraWWindow( FrameDataOut->Frame, BGModel,VWriter,visParams, FrameDataOut->Flies,FrameDataOut->Stats, TRAKING );
			//////////  ALMACENAR ////////////
			if(!GuardarSTFrame( FrameDataOut, nombreFichero ) ){error(6);Finalizar(&g_capture, &VWriter);}
		}
		else VerEstadoBuffer( frame, FramesBuf->numeroDeElementos, visParams, IMAGE_BUFFER_LENGTH);

		//visualizar primer frame del buffer
// 		VisualizarEl( 0, FramesBuf , BGModel,g_capture, VWriter, visParams );
		// visualizar ultimo frame del buffer
		FrameDataIn->Stats->TiempoFrame = FrameDataIn->Stats->TiempoFrame + obtenerTiempo( tif, 0 );
		FrameDataIn->Stats->TiempoGlobal = obtenerTiempo( tinicio, 1);
		FrameDataIn->Stats->totalFrames = TotalFrames;

if(!SHOW_WINDOW)	VisualizarEl(FramesBuf->numeroDeElementos-1, FramesBuf , BGModel, g_capture, VWriter, visParams );
   		///// MEDIR TIEMPOS Del frame de entrada
//   		VisualizarFr( FrameDataIn , BGModel, VWriter );
//   		liberarSTFrame( FrameDataIn );

	}
	///////// LIBERAR MEMORIA Y TERMINAR////////
    Finalizar(&g_capture, &VWriter);

}

void Finalizar(CvCapture **g_capture,CvVideoWriter**VWriter){

	CvCapture *capture;
	CvVideoWriter *Writer;

	// liberar imagenes y datos de preprocesado
	releaseDataPreProcess();
	//liberar modelo de fondo
	DeallocateBGM( BGModel );
	//liberar modelo de forma
	if( Shape) free(Shape);

	// liberar imagenes y datos de procesado
	releaseDataProcess(valParams, BGPrParams);
	if( FrameDataIn ) liberarSTFrame( FrameDataIn);
	if( FrameDataOut ) liberarSTFrame( FrameDataOut);
	if(Fly) free(Fly);
	//liberar listas
	if(Flies) free( Flies );

	// liberar imagenes y datos de tracking
	ReleaseDataTrack();
	if(FramesBuf) {
		liberarBuffer( FramesBuf );
		free( FramesBuf);
	}
	// liberar estructura estadísticas
	if( Stats ) free(Stats);
	// liberar imagenes y datos de visualización
	releaseVisParams( visParams);
	DestroyWindows( );

	//liberar resto de estructuras
	capture = *g_capture;
	Writer = *VWriter;
	if (capture)cvReleaseCapture(&capture);
	if (Writer) cvReleaseVideoWriter(&Writer);

	exit (1);
}



