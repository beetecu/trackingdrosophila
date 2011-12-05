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


void VisualizarEl( int pos, tlcde* frameBuf, StaticBGModel* Flat ){

	DefaultVParams( &Params);

	STFrame* frameData;
	tlcde* flies;
	STFly* fly;


	irAl( pos, frameBuf );
	frameData = (STFrame*)obtenerActual(frameBuf);
	flies = frameData->Flies;
//	if(!frameData->Frame) frameData->Frame = cvCreateImage(cvGetSize( frameData->Frame ), 8,3);

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
		if(frameData->Flies!= NULL && frameData->Flies->numeroDeElementos>0) {
			//Dibujamos los blobs
			cvCopy( frameData->Frame,frameData->Frame);
			for( int i = 0; i <  flies->numeroDeElementos; i++ ){
				fly = (STFly*)obtener(i, flies);
				CvSize axes = cvSize( cvRound(fly->a) , cvRound(fly->b) );
				float angle;
				if ( fly->orientacion >=0 && fly->orientacion < 180) angle = 180 - fly->orientacion;
				else angle = (360-fly->orientacion)+180;
				if( fly->Estado == 1){
					cvEllipse( frameData->Frame, fly->posicion, axes, angle, 0, 360, fly->Color, 1, 8);
					// dibujamos triangulo CA,AB,BC
					cvLine( frameData->Frame,
							cvPoint( cvRound( fly->posicion.x - fly->a/2 ),
									cvRound( fly->posicion.y + fly->b/2 )  ),
							cvPoint( cvRound( fly->posicion.x - fly->a/2 ),
									 cvRound( fly->posicion.y - fly->b/2 )  ),

							fly->Color,
							1,CV_AA, 0 );
					cvLine( frameData->Frame,
							cvPoint( cvRound( fly->posicion.x - fly->a/2 ),
									 cvRound( fly->posicion.y - fly->b/2 )  ),
							cvPoint( cvRound( fly->posicion.x + fly->a/2 ),
									 fly->posicion.y ),
							fly->Color,
							1,CV_AA, 0 );
					cvLine( frameData->Frame,
							cvPoint( cvRound( fly->posicion.x + fly->a/2 ),
														 fly->posicion.y ),
							cvPoint( cvRound( fly->posicion.x - fly->a/2 ),
									 cvRound( fly->posicion.y + fly->b/2 )  ),
							fly->Color,
							1,CV_AA, 0 );
//					cvLine( frameData->Frame,
//							fly->posicion,
//							cvPoint( cvRound( fly->posicion.x + 1.5*fly->a*cos(fly->orientacion*CV_PI/180) ),
//									 cvRound( fly->posicion.y - 1.5*fly->a*sin(fly->orientacion*CV_PI/180) )  ),
//							fly->Color,
//							1,CV_AA, 0 );
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
		}
//		cvLine( frameData->Frame,
//						cvPoint( 193, 231 ),
//						cvPoint( 447, 231 ),
//								CVX_RED,
//								1, CV_AA, 0 );
		// Mostramos imagenes
		cvShowImage( "Visualización", frameData->Frame );
		if (SHOW_BG_REMOVAL == 1){
				cvShowImage("Background", frameData->BGModel);
				cvShowImage( "Foreground",frameData->FG);
		}
		if ( SHOW_MOTION_TEMPLATE == 1)	cvShowImage( "Motion",frameData->ImMotion);

		// si se pulsa p ó P  => pause = true
		if( (cvWaitKey(5) & 255) == 80 || (cvWaitKey(5) & 255) == 112 ){
			Params->pause = true;
		}
		if(Params->pause){
			cvShowImage( "Visualización", frameData->Frame );
			if (SHOW_BG_REMOVAL == 1){
					cvShowImage("Background", frameData->BGModel);
					cvShowImage( "Foreground",frameData->FG);
			}
			if ( SHOW_MOTION_TEMPLATE == 1)	cvShowImage( "Motion",frameData->ImMotion);
			fflush( stdin);
			if( cvWaitKey(0) == 'g' || cvWaitKey(0) == 'G'){
				irAl( pos, frameBuf );
				frameData = (STFrame*)obtenerActual(frameBuf);
				cvSaveImage( "Captura.jpg", frameData->Frame);
			}
		}
		// si se pulsa c ó C  => pause = false
		if( (cvWaitKey(5) & 255) == 67 || (cvWaitKey(5) & 255) == 99 ){
			Params->pause = false;
		}
		// si se pulsa v comienza a grabar un video hasta pulsar S
//		if( opcion == 'v' || opcion == 'V'){
//			Params->Grab = true;
//		}
		// si se pulsa s ó S => stop = true
		if( (cvWaitKey(5) & 255) == 83 || (cvWaitKey(5) & 255) == 115 ){
			Params->stop = true;
		}
		int posBuf = pos;
		// mientras no se presione c ó C ( continue )
		while(Params->stop){
			unsigned char opcion;
			Params->Grab = false;
			fflush( stdin);
			opcion = cvWaitKey(0);

			// si pulsamos +, visualizamos el siguiente elemento del buffer
			if( opcion == 43 ){
				if (posBuf < frameBuf->numeroDeElementos-1) posBuf += 1;
			}
			// si pulsamos -, visualizamos el elemento anterior del buffer
			if( opcion == 45 ){
				if ( posBuf > 0 ) posBuf-=1;
			}
			// si pulsamos i ó I, visualizamos el primer elemento del buffer
			if( opcion == 49 || opcion == 69)  {
				posBuf = 0;
			}
			// si pulsamos f +o F, visualizamos el último elemento del buffer
			if( opcion == 70 || opcion == 102)  {
				posBuf = frameBuf->numeroDeElementos-1;
			}
			// tomar instantanea del frame
			if( opcion == 'g' || opcion == 'G'){
				irAl( posBuf, frameBuf );
				frameData = (STFrame*)obtenerActual(frameBuf);
				cvSaveImage( "Captura.jpg", frameData->Frame);
			}
			irAl( posBuf, frameBuf );
			frameData = (STFrame*)obtenerActual(frameBuf);
			CvFont fuente1;
			char PBuf[100];
			sprintf(PBuf," Posicion del Buffer: %d ",posBuf );
			cvInitFont( &fuente1, CV_FONT_HERSHEY_PLAIN, 1, 1, 0, 1, 8);
			cvPutText( frameData->Frame, PBuf,  cvPoint( 10,frameData->Frame->height-10), &fuente1, CVX_RED );
			VisualizarFr( frameData, Flat );
			if( opcion == 67 || opcion == 99 ){
					Params->stop = false;
			}
		}
		irAlFinal( frameBuf );
	}
}

void VisualizarFr( STFrame* frameData, StaticBGModel* Flat ){

	DefaultVParams( &Params);

	tlcde* flies;
	STFly* fly;

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
		if(frameData->Flies!= NULL && frameData->Flies->numeroDeElementos>0) {
			flies = frameData->Flies;

			for( int i = 0; i <  flies->numeroDeElementos; i++ ){
				fly = (STFly*)obtener(i, flies);
				float angle;
				if ( fly->orientacion >=0 && fly->orientacion < 180) angle = 180 - fly->orientacion;
				else angle = (360-fly->orientacion)+180;
				CvSize axes = cvSize( cvRound(fly->a) , cvRound(fly->b) );
				cvEllipse( frameData->Frame, fly->posicion, axes, angle, 0, 360, fly->Color, 1, 8);
				if( fly->Estado == 1){
					// dibujamos triangulo CA,AB,BC
					cvLine( frameData->Frame,
							cvPoint( cvRound( fly->posicion.x - fly->a/2 ),
									cvRound( fly->posicion.y + fly->b/2 )  ),
							cvPoint( cvRound( fly->posicion.x - fly->a/2 ),
									 cvRound( fly->posicion.y - fly->b/2 )  ),

							fly->Color,
							1,CV_AA, 0 );
					cvLine( frameData->Frame,
							cvPoint( cvRound( fly->posicion.x - fly->a/2 ),
									 cvRound( fly->posicion.y - fly->b/2 )  ),
							cvPoint( cvRound( fly->posicion.x + fly->a/2 ),
									 fly->posicion.y ),
							fly->Color,
							1,CV_AA, 0 );
					cvLine( frameData->Frame,
							cvPoint( cvRound( fly->posicion.x + fly->a/2 ),
									                     fly->posicion.y ),
							cvPoint( cvRound( fly->posicion.x - fly->a/2 ),
									 cvRound( fly->posicion.y + fly->b/2 )  ),
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
		}
		// Mostramos imagenes
		cvShowImage( "Visualización", frameData->Frame );
		if (SHOW_BG_REMOVAL == 1){
				cvShowImage("Background", frameData->BGModel);
				cvShowImage( "Foreground",frameData->FG);
		}
		if ( SHOW_MOTION_TEMPLATE == 1)	cvShowImage( "Motion",frameData->ImMotion);
	}
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

void DefaultVParams( VParams **Parameters){
    //init parameters
	 VParams *params;

    if( *Parameters == NULL )
      {
    	params = ( VParams *) malloc( sizeof( VParams) );

    	params->pause = false;
    	params->stop = false;
    	*Parameters = params;
    }



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
	cvDestroyWindow( "Motion" );
}

