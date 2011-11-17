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


void VisualizarEl( int pos, tlcde* frameBuf, StaticBGModel* Flat ){

	STFrame* frameData;
	tlcde* flies;
	STFly* fly;

	irAl( pos, frameBuf );
	frameData = (STFrame*)obtenerActual(frameBuf);
	flies = frameData->Flies;
	// si no se ha llenado el buffer y se pide visualizar el primero, esperar a llenar buffer.
	if ( ( frameBuf->numeroDeElementos < IMAGE_BUFFER_LENGTH-1) && pos == 0 ){
		VerEstadoBuffer(frameData->Frame, frameBuf->numeroDeElementos );
		return;
	}

	if (SHOW_VISUALIZATION == 1){

	//Obtenemos la Imagen donde se visualizarán los resultados

	//Dibujamos el plato en la imagen de visualizacion
	if( DETECTAR_PLATO ){
		cvCircle( frameData->Frame, cvPoint( Flat->PCentroX,Flat->PCentroY ), 3, CV_RGB(0,0,0), -1, 8, 0 );
		cvCircle( frameData->Frame, cvPoint(Flat->PCentroX,Flat->PCentroY ),Flat->PRadio, CV_RGB(0,0,0),2 );
	}
	// Dibujamos la ROI
//	cvRectangle( frameData->Frame,
//			cvPoint(Flat->PCentroX-Flat->PRadio, Flat->PCentroY-Flat->PRadio),
//			cvPoint(Flat->PCentroX + Flat->PRadio,Flat->PCentroY + Flat->PRadio),
//			CV_RGB(255,0,0),2);

	//Dibujamos los blobs

	for( int i = 0; i <  flies->numeroDeElementos; i++ ){
		fly = (STFly*)obtener(i, flies);
		CvSize axes = cvSize( cvRound(fly->a) , cvRound(fly->b) );

		if( fly->Estado == 1){
			cvEllipse( frameData->Frame, fly->posicion, axes, fly->orientacion, 0, 360, fly->Color, 1, 8);
			cvLine( frameData->Frame,
					fly->posicion,
					cvPoint( cvRound( fly->posicion.x - 1.5*fly->a*cos(fly->orientacion*CV_PI/180) ),
							 cvRound( fly->posicion.y - 1.5*fly->a*sin(fly->orientacion*CV_PI/180) )  ),
					fly->Color,
					1,CV_AA, 0 );
		}
		else{
			cvEllipse( frameData->Frame, fly->posicion, axes, fly->orientacion, 0, 360, fly->Color, 1, 8);
			cvLine( frameData->Frame,
								fly->posicion,
								cvPoint( fly->posicion.x ,
										 fly->posicion.y ),
								fly->Color,
								8,CV_AA, 0 );
		}
		// visualizar direccion
		//double op_angle = 360.0 - fly->direccion;  // adjust for images with top-left origin
		//cvCircle( frameData->Frame, center, cvRound(magnitude*1.2), color, 3, CV_AA, 0 );
		double magnitude = 30;
		cvLine( frameData->Frame,
				fly->posicion,
				cvPoint( cvRound( fly->posicion.x + magnitude*cos(fly->direccion*CV_PI/180)),
						 cvRound( fly->posicion.y - magnitude*sin(fly->direccion*CV_PI/180))  ),
				CVX_RED,
				1, CV_AA, 0 );
		visualizarId( frameData->Frame,fly->posicion, fly->etiqueta, fly->Color);

	}

//	ShowStatDataFr( frameData->Frame );
	cvShowImage( "Visualización", frameData->Frame );


	}
	// Mostramos imagenes

	//
	if (SHOW_BG_REMOVAL == 1){
			cvShowImage("Background", frameData->BGModel);
			cvShowImage( "Foreground",frameData->FG);
	}

	if ( SHOW_MOTION_TEMPLATE == 1){
		cvShowImage( "Motion",frameData->ImMotion);
		}
	cvWaitKey(0);
	irAlFinal( frameBuf );
}

void VisualizarFr( STFrame* frameData, StaticBGModel* Flat ){


	tlcde* flies;
	STFly* fly;

	flies = frameData->Flies;
	if (SHOW_VISUALIZATION == 1){

	//Obtenemos la Imagen donde se visualizarán los resultados

	//Dibujamos el plato en la imagen de visualizacion
	if( DETECTAR_PLATO ){
		cvCircle( frameData->Frame, cvPoint( Flat->PCentroX,Flat->PCentroY ), 3, CV_RGB(0,0,0), -1, 8, 0 );
		cvCircle( frameData->Frame, cvPoint(Flat->PCentroX,Flat->PCentroY ),Flat->PRadio, CV_RGB(0,0,0),2 );
	}
	// Dibujamos la ROI
//	cvRectangle( frameData->Frame,
//			cvPoint(Flat->PCentroX-Flat->PRadio, Flat->PCentroY-Flat->PRadio),
//			cvPoint(Flat->PCentroX + Flat->PRadio,Flat->PCentroY + Flat->PRadio),
//			CV_RGB(255,0,0),2);

	//Dibujamos los blobs
	int i = 0;
	while( i <  flies->numeroDeElementos ){
		fly = (STFly*)obtener(i, flies);
		CvSize axes = cvSize( cvRound(fly->a) , cvRound(fly->b) );
		cvEllipse( frameData->Frame, fly->posicion, axes, fly->orientacion, 0, 360, fly->Color, 1, 8);
		if( fly->Estado == 1){
			cvLine( frameData->Frame,
					fly->posicion,
					cvPoint( cvRound( fly->posicion.x + 1.5*fly->a*cos(fly->orientacion*CV_PI/180) ),
							 cvRound( fly->posicion.y - 1.5*fly->a*sin(fly->orientacion*CV_PI/180) )  ),
					fly->Color,
					1,CV_AA, 0 );
		}
		else{
			cvLine( frameData->Frame,
								fly->posicion,
								cvPoint( fly->posicion.x ,
										 fly->posicion.y ),
								fly->Color,
								8,CV_AA, 0 );
		}
		// visualizar direccion
		double op_angle = 360.0 - fly->direccion;  // adjust for images with top-left origin
		//cvCircle( frameData->Frame, center, cvRound(magnitude*1.2), color, 3, CV_AA, 0 );
		double magnitude = 30;
		cvLine( frameData->Frame,
				fly->posicion,
				cvPoint( cvRound( fly->posicion.x + magnitude*cos(op_angle*CV_PI/180)),
						 cvRound( fly->posicion.y - magnitude*sin(op_angle*CV_PI/180))  ),
				CVX_RED,
				1, CV_AA, 0 );
		visualizarId( frameData->Frame,fly->posicion, fly->etiqueta, fly->Color);
		i++;
	}

//	ShowStatDataFr( frameData->Frame );
	cvShowImage( "Visualización", frameData->Frame );


	}
	// Mostramos imagenes

	//
	if (SHOW_BG_REMOVAL == 1){
			cvShowImage("Background", frameData->BGModel);
			cvShowImage( "Foreground",frameData->FG);
//			cvWaitKey(0);

	}

	if ( SHOW_MOTION_TEMPLATE == 1){
		cvShowImage( "Motion",frameData->ImMotion);
		}
//	cvWaitKey(0);

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
// Destruccion de ventanas
void DestroyWindows( ){

	//cvDestroyWindow( "Drosophila.avi" );


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
