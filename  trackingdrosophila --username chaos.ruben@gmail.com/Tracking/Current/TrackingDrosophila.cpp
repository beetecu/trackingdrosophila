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

extern double NumFrame ; /// contador de frames absolutos ( incluyendo preprocesado )


double NumFrame ; /// contador de frames absolutos ( incluyendo preprocesado )



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
STFrame* FrameDataStats = NULL;
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
	float TiempoFrame;
	float TiempoGlobal;
	double TotalFrames ;
	double FPS;

	char nombreFichero[30];
	char nombreVideo[30];
//	char* Ficheros[4];

	if( argc<1) {help(); return -1;};


	///////////  INICIALIZACIÓN ////////////
	printf("\nInicializando parámetros...");
	if(SHOW_WINDOW) DraWPresent(  );

	TiempoGlobal = 0;
	NumFrame = 0;
	TiempoFrame = 0;
	if (!Inicializacion( argc, argv,nombreFichero,nombreVideo ) ) return -1;

	//////////  PREPROCESADO   ////////////

	if (!PreProcesado( argv[1], &BGModel, &Shape) )  Finalizar(NULL, NULL);

	/////////	PROCESADO   ///////////////

	//  CAPTURA  //

	CvCapture* g_capture = NULL;/// puntero a una estructura de tipo CvCapture
	CvVideoWriter* VWriter;
	IplImage* frame;

	printf( "Iniciando captura...\n" );
	gettimeofday(&tinicio, NULL);//para obtener el tiempo transcurrido desde el inicio del programa
	g_capture = cvCaptureFromAVI( argv[1] );
	if ( !g_capture ) {
		error( 1 );
		help();
		return -1;
	}
//	VWriter = iniciarAvi( g_capture, nombreVideo);
	// Creación de ventanas de visualizacion
	AllocDefaultVisParams(&visParams, BGModel->Imed);

	TotalFrames = cvGetCaptureProperty( g_capture, CV_CAP_PROP_FRAME_COUNT);
	if(!TotalFrames) TotalFrames = getAVIFrames("Drosophila.avi"); // en algun linux no funciona lo anterior
	FPS = cvGetCaptureProperty( g_capture, CV_CAP_PROP_FPS);

	printf("\n\nIniciando procesado...\n");
	if(SHOW_WINDOW) Transicion("Iniciando Tracking...", 1,1000, 50 );

	/*********** BUCLE PRINCIPAL DEL ALGORITMO ***********/

    for( NumFrame = 1;; NumFrame++ ){

    	frame = cvQueryFrame(g_capture);
    	/*Posteriormente  Escribir en un fichero log el error. Actualizar el contador
    	  de frames absolutos. */
    	gettimeofday(&tif, NULL);
    	gettimeofday(&ti, NULL);
    	if( !frame ){
    		NumFrame = NumFrame + RetryCap( g_capture, frame );
    	}
    	if( !frame ) Finalizar(&g_capture, &VWriter);
		if ( (cvWaitKey(10) & 255) == 27 ) break;

		printf("\n//////////////////////////////////////////////////\n");
		printf("\t\t\tFRAME %0.f ",NumFrame);

		//////////  PROCESAR  ////////////
		FrameDataIn = Procesado2(frame, BGModel, Shape, valParams, BGPrParams );

		//////////  RASTREAR  ////////////
		FrameDataOut = Tracking(  &FramesBuf, FrameDataIn );

		// SI BUFFER LLENO
		if( FrameDataOut ){

			////////// ESTADISTICAS //////////
			// para su cálculo usamos el frameDataOut anterior.
			CalcStatsFrame( FrameDataStats,FrameDataOut );

			//////////  VISUALIZAR     ////////////
//			VisualizarFr( FrameDataOut , BGModel, VWriter );
			if(SHOW_WINDOW) DraWWindow( FrameDataOut->Frame, BGModel,VWriter,visParams, FrameDataOut->Flies,FrameDataOut->Stats, TRAKING );

			//////////  ALMACENAR ////////////
			if(!GuardarSTFrame( FrameDataOut, nombreFichero ) ){error(6);Finalizar(&g_capture, &VWriter);}

//			////////// LIBERAR MEMORIA  ////////////
			liberarSTFrame( FrameDataStats );
		}
		else VerEstadoBuffer( frame, FramesBuf->numeroDeElementos, visParams, IMAGE_BUFFER_LENGTH);

		FrameDataStats = FrameDataOut;
		FrameDataIn->Stats = InitStatsFrame( NumFrame, tif, tinicio, TotalFrames, FPS );

		if(!SHOW_WINDOW)	VisualizarEl(FramesBuf->numeroDeElementos-1, FramesBuf , BGModel, g_capture, VWriter, visParams );

		printf("\n//////////////////////////////////////////////////\n");
		printf("\nTiempo de procesado del  Frame %.0f : %5.4g ms\n",NumFrame-1, FrameDataIn->Stats->TiempoFrame);
		printf("Segundos de video procesados: %0.f seg \n", FrameDataIn->Stats->TiempoGlobal);
		printf("Porcentaje completado: %.2f %% \n",((NumFrame-1)/TotalFrames)*100 );
		printf("\n//////////////////////////////////////////////////\n");
//		if(!SHOW_WINDOW)	VisualizarFr( FrameDataIn, BGModel, VWriter, visParams );
//		liberarSTFrame( FrameDataIn );
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
	ReleaseDataTrack( FramesBuf );

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



