/*
 * Visualizacion.cpp
 *
 *  Created on: 27/10/2011
 *      Author: chao
 */

#include "Visualizacion.hpp"

	extern float TiempoGlobal ;
	extern float TiempoFrame;
	extern double NumFrame ; /// contador de frames absolutos ( incluyendo preprocesado )
	extern double TotalFrames ;

	VParams* Params = NULL;
#include <opencv2/video/tracking.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <iostream>
#include <ctype.h>
#include <stdio.h>
using namespace cv;
using namespace std;

void VisualizarEl( int pos, tlcde* frameBuf, StaticBGModel* Flat, CvCapture* Cap ){

	DefaultVParams( &Params);

	STFrame* frameData;
	tlcde* flies;
	irAl( pos, frameBuf );
	frameData = (STFrame*)obtenerActual(frameBuf);
	flies = frameData->Flies;

	if (SHOW_VISUALIZATION == 1){
		// OBTENER FRAME


		// si no se ha llenado el buffer y se pide visualizar el primero, esperar a llenar buffer.
		if ( ( frameBuf->numeroDeElementos < IMAGE_BUFFER_LENGTH-1) && pos == 0 ){
			VerEstadoBuffer(frameData->Frame, frameBuf->numeroDeElementos );
			return;
		}

		// DIBUJAR PLATO
		if( DETECTAR_PLATO ){
			cvCircle( frameData->Frame, cvPoint( Flat->PCentroX,Flat->PCentroY ), 3, CV_RGB(0,0,0), -1, 8, 0 );
			cvCircle( frameData->Frame, cvPoint(Flat->PCentroX,Flat->PCentroY ),Flat->PRadio, CV_RGB(0,0,0),2 );
		}

		// DIBUJAR BLOBS Y DIRECCIÓN DE DESPLAZAMIENTO
		dibujarBlobs( frameData->Frame, frameData->Flies );

		// MOSTRAMOS IMAGENES
		printf( "\t-Mostrando ventana de visualización...");
		cvShowImage( "Visualización", frameData->Frame );
		printf("Hecho\n");
		if (SHOW_BG_REMOVAL == 1){
			printf( "\t-Mostrando Background, Foreground, OldBackground... ");
			cvShowImage("Background", frameData->BGModel);

			cvShowImage( "Foreground",frameData->FG);
			//cvShowImage( "OldForeground",frameData->OldFG);

			printf("Hecho\n");
		}
		if ( SHOW_MOTION_TEMPLATE == 1){
			printf("\t-Mostrando Motion...");
			cvShowImage( "Motion",frameData->ImMotion);
			printf("Hecho\n");
		}

		if(SHOW_KALMAN){
			printf("\t-Mostrando predicciones del filtro de Kalman...");
			cvShowImage( "Filtro de Kalman", frameData->ImKalman );
			printf("Hecho\n");
		}

		// OPCIONES
		if(ACTIVAR_OPCIONES_VISUALIZACION){
			// si se pulsa p ó P  => pause = true
			if( (cvWaitKey(5) & 255) == 80 || (cvWaitKey(5) & 255) == 112 ){
				Params->pause = true;
			}
			// si se pulsa s ó S => stop = true
			if( (cvWaitKey(5) & 255) == 83 || (cvWaitKey(5) & 255) == 115 ){
				Params->stop = true;
			}
			// si se pulsa v ó V => grab = true
			if( (cvWaitKey(5) & 255) == 'v' || (cvWaitKey(5) & 255) == 'V' ){
				Params->Grab = true;
			}
			// PAUSA
			if(Params->pause){
				cvShowImage( "Visualización", frameData->Frame );
				if (SHOW_BG_REMOVAL == 1){
					cvShowImage("Background", frameData->BGModel);
					cvShowImage( "Foreground",frameData->FG);
				//	cvShowImage( "OldForeground",frameData->OldFG);
				//	cvShowImage( "Foreground+Oldforeground", frameData->ImAdd);
				}
				if ( SHOW_MOTION_TEMPLATE == 1){
					cvShowImage( "Motion",frameData->ImMotion);
				}
				fflush( stdin);
				if( cvWaitKey(0) == 'f' || cvWaitKey(0) == 'F'){
					cvSaveImage( "Captura.jpg", frameData->Frame);
				}
			}
			// si se pulsa c ó C  => pause = false
			if( (cvWaitKey(5) & 255) == 67 || (cvWaitKey(5) & 255) == 99 ){
				Params->pause = false;
			}
			if(Params->Grab){
//
//				static int first = 0;
//				char videoName[30];
//				for( int i = 0; i < 1000; i++ ){
//							sprintf(nombreVideo,"Muestra%d.avi",i);
//							if( !existe( videoName ) ) {
//								break;
//							}
//				}
//				writer = iniciarAvi( cap, nombreVideo);
//				cvWriteFrame( VWriter,frameData->Frame);
			}
			// si se pulsa v comienza a grabar un video hasta pulsar S
	//		if( opcion == 'v' || opcion == 'V'){
	//			Params->Grab = true;
	//		}
			//STOP
			int posBuf = pos;
			while(Params->stop){
				visualizarBuffer( frameBuf,Flat, &posBuf );
				// mientras no se presione c ó C ( continue ) continuamos en el while
				if(!Params->stop) break;
			}
			irAlFinal( frameBuf );
		}//FIN OPCIONES
	}// FIN VISUALIZACION
}

void dibujarBlobs( IplImage* Imagen,tlcde* flies ){

	STFly* fly;

	if(flies!= NULL && flies->numeroDeElementos>0) {
		for( int i = 0; i <  flies->numeroDeElementos; i++ ){
			// obtener lista de blobs.
			fly = (STFly*)obtener(i, flies);
			// Dibujar un trianguno isósceles que representa el blob
			CvPoint A   = cvPoint( cvRound( fly->posicion.x + fly->a*cos(fly->orientacion*CV_PI/180) ),
								   cvRound( fly->posicion.y - fly->a*sin(fly->orientacion*CV_PI/180)));
			CvPoint Mcb = cvPoint( cvRound( fly->posicion.x - fly->a*cos(fly->orientacion*CV_PI/180)),
								   cvRound( fly->posicion.y + fly->a*sin(fly->orientacion*CV_PI/180)));
			CvPoint B =   cvPoint( cvRound( Mcb.x + fly->b*cos( (fly->orientacion+90)*CV_PI/180) ),
								   cvRound( Mcb.y - fly->b*sin( (fly->orientacion+90)*CV_PI/180)));
			CvPoint C =   cvPoint( cvRound( Mcb.x + fly->b*cos( (fly->orientacion-90)*CV_PI/180) ),
								   cvRound( Mcb.y - fly->b*sin( (fly->orientacion-90)*CV_PI/180)));
			if( fly->Estado == 1){
				cvLine( Imagen,A,B,fly->Color,1,CV_AA, 0 );
				cvLine( Imagen,B,C,fly->Color,1,CV_AA, 0 );
				cvLine( Imagen,C,A,fly->Color,1,CV_AA, 0 );
			}
			else{
				cvLine( Imagen,A,B,CVX_WHITE,1,CV_AA, 0 );
				cvLine( Imagen,B,C,CVX_WHITE,1,CV_AA, 0 );
				cvLine( Imagen,C,A,CVX_WHITE,1,CV_AA, 0 );
			}
			muestrearPosicion( flies, 1 );
//			Mat img(Imagen);
//			rectangle(img,
//					Point(fly->Roi.x,fly->Roi.y),
//					Point(fly->Roi.x+fly->Roi.width,fly->Roi.y+fly->Roi.height),
//					Scalar(255), 1,8,0);
//			imshow("CamShift Demo",img);
			// visualizar direccion
			//double op_angle = 360.0 - fly->direccion;  // adjust for images with top-left origin
			//cvCircle( frameData->Frame, center, cvRound(magnitude*1.2), color, 3, CV_AA, 0 );

			double magnitude = 30;
			cvLine( Imagen,
					fly->posicion,
					cvPoint( cvRound( fly->posicion.x + magnitude*cos(fly->direccion*CV_PI/180)),
							 cvRound( fly->posicion.y - magnitude*sin(fly->direccion*CV_PI/180))  ),
					CVX_RED,
					1, CV_AA, 0 );
			visualizarId( Imagen,fly->posicion, fly->etiqueta, fly->Color);
		}
	}
	else return;
}

void VisualizarFr( STFrame* frameData, StaticBGModel* Flat ){

	DefaultVParams( &Params);

	if (SHOW_VISUALIZATION ){
		// DIBUJAR PLATO
		if( DETECTAR_PLATO ){
			cvCircle( frameData->Frame, cvPoint( Flat->PCentroX,Flat->PCentroY ), 3, CV_RGB(0,0,0), -1, 8, 0 );
			cvCircle( frameData->Frame, cvPoint(Flat->PCentroX,Flat->PCentroY ),Flat->PRadio, CV_RGB(0,0,0),2 );
		}

		// DIBUJAR BLOBS Y DIRECCIÓN DE DESPLAZAMIENTO
		dibujarBlobs( frameData->Frame, frameData->Flies );
		mostrarFliesFrame(frameData);
		// MOSTRAR IMAGENES
		cvShowImage( "Visualización", frameData->Frame );
		if (SHOW_BG_REMOVAL){
				cvShowImage("Background", frameData->BGModel);
				cvShowImage( "Foreground",frameData->FG);
			//	cvShowImage( "OldForeground",frameData->OldFG);
			//	cvShowImage( "Foreground+Oldforeground", frameData->ImAdd);
		}
		if ( SHOW_MOTION_TEMPLATE )	cvShowImage( "Motion",frameData->ImMotion);
		if(SHOW_KALMAN)  			cvShowImage( "Filtro de Kalman", frameData->ImKalman );
	}

}

void ShowStatDataFr( IplImage* Im  ){

	CvFont fuente1;
	CvFont fuente2;

	char NFrame[100];
	char TProcesF[100];
	char TProces[100];
	char PComplet[100];
	char FPS[100];

	sprintf(NFrame,"Frame %.0f ",NumFrame);
	sprintf(TProcesF,"Tiempo de procesado del Frame : %5.4g ms", TiempoFrame);
	sprintf(TProces,"Segundos de video procesados: %.3f seg ", TiempoGlobal/1000);
	sprintf(PComplet,"Porcentaje completado: %.2f %% ",(NumFrame/TotalFrames)*100 );
	sprintf(FPS,"FPS: %.2f ",(1000/TiempoFrame));

	cvInitFont( &fuente1, CV_FONT_HERSHEY_PLAIN, 1, 1, 0, 1, 8);
	cvInitFont( &fuente2, CV_FONT_HERSHEY_PLAIN, 0.7, 0.7, 0, 1, 8);

	cvPutText( Im, NFrame, cvPoint( 10,20), &fuente1, CVX_BLUE );
	cvPutText( Im, TProcesF, cvPoint( 10,40), &fuente2, CVX_WHITE );
	cvPutText( Im, TProces, cvPoint( 10,60), &fuente2, CVX_WHITE);
	cvPutText( Im, PComplet, cvPoint( 10,80), &fuente2, CVX_WHITE);
	cvPutText( Im, FPS, cvPoint( 10,100), &fuente1, CVX_RED);
}
// Genera una imagen que representa el llenado del buffer
void VerEstadoBuffer( IplImage* Imagen,int num ){
	static int count = 0 ;
	float anchoBuf = 200; // longitud en pixels del ancho del buffer
	IplImage* buffer;
	char PComplet[100];
	CvFont fuente1;
	float porcentaje;

	CvSize size = cvSize(Imagen->width,Imagen->height); // get current frame size
	buffer = cvCreateImage(size, IPL_DEPTH_8U, 3);
	cvZero( buffer);

	cvRectangle( buffer, cvPoint( 218, 228 ),cvPoint( 422, 252), CVX_WHITE, 1 );
	cvRectangle( buffer, cvPoint( 220, 230 ),cvPoint( 220 + count , 250 ), CVX_GREEN, -1 );

	count = count + 4;
	porcentaje = (count/anchoBuf)*100;
	sprintf(PComplet," %.0f %% ",porcentaje );
//	cvInitFont( &fuente1, CV_FONT_HERSHEY_PLAIN, 1, 1, 0, 1, 8);
	cvInitFont( &fuente1, CV_FONT_HERSHEY_PLAIN, 1, 1, 0, 1, 8);
	cvPutText( buffer, PComplet,  cvPoint( 300,265), &fuente1, CVX_WHITE );
	cvPutText( buffer, "Llenando Buffer",cvPoint( 250,225 ) ,&fuente1,CVX_WHITE );
	cvShowImage( "Visualización", buffer );
	cvReleaseImage(&buffer);
//	cvWaitKey(0);
}

void visualizarId(IplImage* Imagen,CvPoint pos, int id , CvScalar color ){

	char etiqueta[10];
	CvFont fuente1;
	CvPoint origen;

	origen = cvPoint( pos.x-5 , pos.y - 15);

	sprintf(etiqueta,"%d",id );
	cvInitFont( &fuente1, CV_FONT_HERSHEY_PLAIN, 1, 1, 0, 1, 8);
	cvPutText( Imagen, etiqueta,  origen, &fuente1, color );
}


//Representamos los blobs mediante triangulos isosceles
// dibujamos triangulo isosceles de altura el eje mayor de la elipse, formando el segmento
// (A,mcb), y de anchura el eje menor dando lugar al segmento (B,C), perpendicular
// a (A,mcb) cuyo centro es mcb. La unión de A,B,C dará el triangulo resultante.


void visualizarBuffer( tlcde* Buffer,StaticBGModel* Flat, int *posBuf ){

	STFrame* frameData;
	unsigned char opcion;

	Params->Grab = false;
	fflush( stdin);
	opcion = cvWaitKey(0);
	// si pulsamos +, visualizamos el siguiente elemento del buffer
	if( opcion == 43 ){
		if (*posBuf < Buffer->numeroDeElementos-1) *posBuf=*posBuf+1;
	}
	// si pulsamos -, visualizamos el elemento anterior del buffer
	if( opcion == 45 ){
		if ( *posBuf > 0 ) *posBuf=*posBuf-1;
	}
	// si pulsamos i ó I, visualizamos el primer elemento del buffer
	if( opcion == 49 || opcion == 69)  {
		*posBuf = 0;
	}
	// si pulsamos f +o F, visualizamos el último elemento del buffer
	if( opcion == 70 || opcion == 102)  {
		*posBuf = Buffer->numeroDeElementos-1;
	}
	// tomar instantanea del frame
	if( opcion == 'g' || opcion == 'G'){
		irAl( *posBuf, Buffer );
		frameData = (STFrame*)obtenerActual(Buffer);
		cvSaveImage( "Captura.jpg", frameData->Frame);
	}
	irAl( *posBuf,Buffer );
	frameData = (STFrame*)obtenerActual(Buffer);
	CvFont fuente1;
	char PBuf[100];
	sprintf(PBuf," Posicion del Buffer: %d ",*posBuf );
	cvInitFont( &fuente1, CV_FONT_HERSHEY_PLAIN, 1, 1, 0, 1, 8);
	cvPutText( frameData->Frame, PBuf,  cvPoint( 10,frameData->Frame->height-10), &fuente1, CVX_RED );
	VisualizarFr( frameData, Flat );

	if( opcion == 'C' || opcion == 'c' ){
			Params->stop = false;
	}
}

void DefaultVParams( VParams **Parameters){
    //init parameters
	 VParams *params;

    if( *Parameters == NULL )
      {
    	params = ( VParams *) malloc( sizeof( VParams) );

    	params->pause = false;
    	params->stop = false;
    	params->Grab = false;
    	*Parameters = params;
    }
}
void releaseVParams( VParams **Parameters);
//void VerEstadoSHModel( IplImage* Imagen,int num ){
//
//}

//void VerEstadoBGModel( IplImage* Imagen ){
//
//	static int count = 0 ;
//	float anchoBuf = 200; // longitud en pixels del ancho del buffer
//	IplImage* bgmod;
//	char PComplet[100];
//	CvFont fuente1;
//	float porcentaje;
//
//	CvSize size = cvSize(Imagen->width,Imagen->height); // get current frame size
//	bgmod= cvCreateImage(size, IPL_DEPTH_8U, 3);
//	cvZero( bgmod);
//
//	cvRectangle( bgmod, cvPoint( 218, 228 ),cvPoint( 422, 252), CVX_WHITE, 1 );
//	cvRectangle( bgmod, cvPoint( 220, 230 ),cvPoint( 220 + count , 250 ), CVX_GREEN, -1 );
//
//	count = count + 4;
//	porcentaje = (count/anchoBuf)*100;
//	sprintf(PComplet," %.0f %% ",porcentaje );
////	cvInitFont( &fuente1, CV_FONT_HERSHEY_PLAIN, 1, 1, 0, 1, 8);
//	cvInitFont( &fuente1, CV_FONT_HERSHEY_PLAIN, 1, 1, 0, 1, 8);
//	cvPutText(bgmod, PComplet,  cvPoint( 300,265), &fuente1, CVX_WHITE );
//	cvPutText( bgmod, "Modelando fondo...",cvPoint( 250,225 ) ,&fuente1,CVX_WHITE );
//	cvShowImage( "Visualización", bgmod );
//	cvReleaseImage(&bgmod);
//}

// Creación de ventanas
void CreateWindows( ){

//	cvNamedWindow( "Drosophila.avi", CV_WINDOW_AUTOSIZE );
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
// Destrucción de ventanas y parametros de visualización
void DestroyWindows( ){

	//cvDestroyWindow( "Drosophila.avi" );
	free(Params);

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

}

