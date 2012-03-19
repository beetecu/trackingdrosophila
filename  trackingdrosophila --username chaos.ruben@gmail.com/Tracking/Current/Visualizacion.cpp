/*
 * Visualizacion.cpp
 *
 *  Created on: 27/10/2011
 *      Author: chao
 */

#include "Visualizacion.hpp"
#include <opencv2/video/tracking.hpp>
#include <ctype.h>

using namespace cv;
using namespace std;

extern float TiempoGlobal ;
extern float TiempoFrame;
extern double NumFrame ; /// contador de frames absolutos ( incluyendo preprocesado )



IplImage* ImVisual = NULL;
IplImage* Window = NULL;
IplImage* ImScale= NULL;


void VisualizarEl( int pos, tlcde* frameBuf, StaticBGModel* Flat, CvCapture* Cap, CvVideoWriter* Writer, VisParams* visParams ){

	struct timeval tif;
	float TiempoParcial;
	STFrame* frameData;
	tlcde* flies;
	VisParams* params = NULL;

#ifdef MEDIR_TIEMPOS gettimeofday(&tif, NULL);
#endif
	printf("\n4)Visualización:\n");


	// Establecer la posición en del bufer para visualizar
	if(visParams->VisualPos == -1) irAl( pos, frameBuf );
	else irAl(visParams->VisualPos, frameBuf );

	// OBTENER FRAME
	frameData = (STFrame*)obtenerActual(frameBuf);
	flies = frameData->Flies;

	if (SHOW_VISUALIZATION == 1||GRABAR_VISUALIZACION == 1){
		// Establecer parámetros
		if (visParams == NULL)	AllocDefaultVisParams(&params, frameData->Frame );

		if(!ImVisual) {
			ImVisual = cvCreateImage( cvGetSize( frameData->Frame ),8,3);
			//CreateWindows( ImVisual );
		}
		cvCopy(frameData->Frame,ImVisual);

		// DIBUJAR PLATO
		if( DETECTAR_PLATO ){
			cvCircle( ImVisual, cvPoint( Flat->PCentroX,Flat->PCentroY ), 3, CV_RGB(0,0,0), -1, 8, 0 );
			cvCircle( ImVisual, cvPoint(Flat->PCentroX,Flat->PCentroY ),Flat->PRadio, CV_RGB(0,0,0),2 );
		}

		// DIBUJAR BLOBS Y DIRECCIÓN DE DESPLAZAMIENTO
		dibujarBlobs( ImVisual, frameData->Flies );

		// MOSTRAR datos estadísticos en la ventana de visualización
		ShowStatDataFr( frameData->Stats, ImVisual, visParams );

		// GUARDAR VISUALIZACION
		if( GRABAR_VISUALIZACION){
			cvWriteFrame( Writer,ImVisual);
		}
		printf("Hecho\n");
		if (SHOW_VISUALIZATION){
			// MOSTRAMOS IMAGENES
			printf( "\t-Mostrando ventana de visualización...");
			cvShowImage( "Visualización", ImVisual );
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
					visParams->pause = true;
				}
				// si se pulsa s ó S => stop = true
				if( (cvWaitKey(5) & 255) == 83 || (cvWaitKey(5) & 255) == 115 ){
					visParams->stop = true;
				}
				// si se pulsa v ó V => grab = true
				if( (cvWaitKey(5) & 255) == 'v' || (cvWaitKey(5) & 255) == 'V' ){
					visParams->Grab = true;
				}
				// PAUSA
				if(visParams->pause){
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
					visParams->pause = false;
				}
				if(visParams->Grab){
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
		//			visParams->Grab = true;
		//		}
				//STOP

				while(visParams->stop){
					if(visParams->VisualPos == -1) visParams->VisualPos = pos;
					visualizarBuffer( frameBuf,Flat, visParams, Writer);
					// mientras no se presione c ó C ( continue ) continuamos en el while
					if(!visParams->stop) break;
				}
				irAlFinal( frameBuf );
			}//FIN OPCIONES
		} // FIN VISUALIZAR
		if (visParams == NULL) free( params );
	}// FIN VISUALIZAR O GRABAR
#ifdef MEDIR_TIEMPOS
	TiempoParcial = obtenerTiempo( tif , NULL);
	printf("Visualización finalizada.Tiempo total %5.4g ms\n", TiempoParcial);
#endif
}

void VisualizarFr( STFrame* frameData, StaticBGModel* Flat,CvVideoWriter* Writer, VisParams* visParams ){


	if(!ImVisual) ImVisual = cvCreateImage( cvGetSize( frameData->Frame ),8,3);

	if (SHOW_VISUALIZATION == 1||GRABAR_VISUALIZACION == 1){

		if(!ImVisual) ImVisual = cvCreateImage( cvGetSize( frameData->Frame ),8,1);

		cvCopy(frameData->Frame,ImVisual);

		// DIBUJAR PLATO
		if( DETECTAR_PLATO ){
			cvCircle( ImVisual, cvPoint( Flat->PCentroX,Flat->PCentroY ), 3, CV_RGB(0,0,0), -1, 8, 0 );
			cvCircle( ImVisual, cvPoint(Flat->PCentroX,Flat->PCentroY ),Flat->PRadio, CV_RGB(0,0,0),2 );
		}

		// DIBUJAR BLOBS Y DIRECCIÓN DE DESPLAZAMIENTO
		dibujarBlobs( ImVisual, frameData->Flies );

		// MOSTRAR datos estadísticos en la ventana de visualización
		ShowStatDataFr( frameData->Stats, ImVisual,visParams);

		// GUARDAR VISUALIZACION
		if( GRABAR_VISUALIZACION){
			cvWriteFrame( Writer,ImVisual);
		}
		printf("Hecho\n");
		if (SHOW_VISUALIZATION){
			// MOSTRAMOS IMAGENES
			printf( "\t-Mostrando ventana de visualización...");
			cvShowImage( "Visualización", ImVisual );
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
		}
	}

}

void DraWWindow( IplImage* ImRaw, StaticBGModel* BGModel, CvVideoWriter* Writer, VisParams* params, tlcde* flies, STStatFrame* Stats, int num  ){

	CvRect ROI;
	CvSize size;


	if (SHOW_VISUALIZATION == 1 || GRABAR_VISUALIZACION == 1){


		if(!ImVisual) ImVisual = cvCreateImage( cvGetSize(ImRaw),8,3);
		if(!Window)	Window = cvCreateImage( params->Resolucion,8,3);

		if( ImRaw->nChannels == 1)	cvCvtColor( ImRaw , ImVisual, CV_GRAY2BGR);
		else cvCopy(ImRaw,ImVisual);

		// DIBUJAR PLATO
		if( BGModel->PRadio > 0 ){
			cvCircle( ImVisual, cvPoint( BGModel->PCentroX,BGModel->PCentroY ), 3, CV_RGB(0,0,0), -1, 8, 0 );
			cvCircle( ImVisual, cvPoint(BGModel->PCentroX,BGModel->PCentroY ),BGModel->PRadio, CV_RGB(0,0,0),2 );
		}

		// DIBUJAR BLOBS Y DIRECCIÓN DE DESPLAZAMIENTO
		if( flies != NULL && flies->numeroDeElementos>0 ) dibujarBlobs( ImVisual, flies );

		cvZero( Window);

		// DIBUJAR VENTANA DE VIDEO

		// reescalar si la imagen es de 320 o menos la escalamos al doble
		if(ImVisual->width <= 320){

			if(! ImScale) {
					size = cvSize(2*ImRaw->width,2*ImRaw->height);
					ImScale = cvCreateImage( size ,8,3);
			}
			SetVisParams(params, ImScale );
			cvPyrUp( ImVisual, ImScale,IPL_GAUSSIAN_5x5);
		}
		else ImScale = ImVisual ;

		if( num < 4 ){
			Incrustar( Window, ImScale, Window, params->ROIPreProces);
			cvRectangle( Window, cvPoint(params->ROIPreProces.x,params->ROIPreProces.y),cvPoint(params->ROIPreProces.x + params->ROIPreProces.width,params->ROIPreProces.y + params->ROIPreProces.height), CVX_WHITE, 1 );

		}
		else{
			DibujarFondo( params );
			Incrustar( Window, ImScale, Window, params->ROITracking);
		}
		IncrustarTxt(params, num );
		// MOSTRAR datos estadísticos en la ventana de visualización
		if( Stats) ShowStatDataFr( Stats, Window, params);

		// GUARDAR VISUALIZACION
//		if( GRABAR_VISUALIZACION){
//			cvWriteFrame( Writer,Window);
//		}
		cvShowImage("TrackingDrosophila",Window);
		printf("Hecho\n");

		//free(VParams);
	}
}

void DraWPresent(  ){

	IplImage* Logo;
	VisParams* vParams = NULL;
	CvFont fuente1;
	CvFont fuente2;
	AllocDefaultVisParams(&vParams, NULL );
	if(!Window)	Window = cvCreateImage( vParams->Resolucion,8,3);
	cvZero( Window );

	//CreateWindows( ImVisual );
	CvRect rect = cvRect(0,0,Window->width,Window->height);

	IncrustarLogo("logos.jpg", Window , CENTRAR ,vParams->DelayLogo,false);
	desvanecer( Window, 10 ,rect);
	IncrustarLogo("logo-opencv.jpg", Window , CENTRAR ,vParams->DelayLogo,false);
	desvanecer( Window, 10 , rect);
	IncrustarLogo("Logo_IBGM.png", Window , CENTRAR_INF,vParams->DelayLogo,false);
	desvanecer( Window, 10 ,rect);
	IncrustarLogo("LogoUVAnegro.jpg", Window , CENTRAR,vParams->DelayLogo,false);
	cvWaitKey(1000);
	desvanecer( Window, 10 ,rect);
	cvInitFont( &fuente1, CV_FONT_HERSHEY_COMPLEX, 2, 2, 0, 3, CV_AA);
//	cvInitFont( &fuente2, CV_FONT_HERSHEY_PLAIN, 0.5, 0.5, 0, 1, CV_AA);
	cvInitFont( &fuente2, CV_FONT_HERSHEY_PLAIN, 1, 1, 0, 1, 8);
	CvSize textsize = getTextSize("Tracking Drosophila", CV_FONT_HERSHEY_COMPLEX, 2, 5, 0);
	CvSize textsize2 = getTextSize("Presione una tecla para comenzar", CV_FONT_HERSHEY_PLAIN, 1, 1, 0);

	for( int i = 0; i < 255; i += 2 )
	{
		cvPutText( Window, "Tracking Drosophila",
				cvPoint( (Window->width-textsize.width)/2,( Window->height-textsize.height)/2),
				&fuente1,
				cvScalar(i,i,i) );
		cvShowImage("TrackingDrosophila", Window);
		cvWaitKey(5);
	}
	int comenzar = 0;
	for( int i = 0; i < 255; i += 5 )
	{
		cvPutText( Window, "Presione una tecla para comenzar...",
				cvPoint( (Window->width-textsize2.width)/2,( Window->height-textsize2.height)/2+ 50),
				&fuente2,
				cvScalar(i,i,i) );
		cvShowImage("TrackingDrosophila", Window);
		if(cvWaitKey(5) >= 0){
			comenzar = 1;
			break;
		}
		if(i == 250 ){
			for( i ; i > 0; i -= 5 )
			{
				cvPutText( Window, "Presione una tecla para comenzar...",
				cvPoint( (Window->width-textsize2.width)/2,( Window->height-textsize2.height)/2+ 50),
				&fuente2,
				cvScalar(i,i,i) );
				cvShowImage("TrackingDrosophila", Window);
				if(cvWaitKey(2) >= 0){
					comenzar = 1;
					break;
				}
			}
			i = 0;
		}
		if(comenzar) break;
	}


//	IncrustarLogo("logo_subversion_.png", ImLogos, cvPoint( 0,0 ),vParams->DelayLogo,true);

//	CvSize textsize = getTextSize("OpenCV forever!", CV_FONT_HERSHEY_COMPLEX, 3, 5, 0);
//	CvPoint org((width - textsize.width)/2, (height - textsize.height)/2);
//	Mat image2;
//	for( i = 0; i < 255; i += 2 )
//	{
//		image2 = Window  - Scalar::all(i);
//		putText(image2, "Tracking Drosophila", org, CV_FONT_HERSHEY_COMPLEX, 3,
//				Scalar(i, i, i), 5, lineType);
//
//		imshow("TrackingDrosophila", image2);
//		if(waitKey(5) >= 0)
//			return 0;
//	}

	free( vParams );


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



void ShowStatDataFr( STStatFrame* Stats,IplImage* Window,VisParams* visParams ){

	CvFont fuente1;
	CvFont fuente2;

	char NFrame[100];
	char TProcesF[100];
	char TProces[100];
	char PComplet[100];
	char FPS[100];

	sprintf(NFrame,"Frame %d ",Stats->numFrame);
	sprintf(TProcesF,"Tiempo de procesado del Frame: %5.4g ms",Stats->TiempoFrame);
	sprintf(TProces,"Segundos de video procesados: %0.f seg ", Stats->TiempoGlobal);
	sprintf(PComplet,"Porcentaje completado: %.2f %% ",(Stats->numFrame/Stats->totalFrames)*100 );
	sprintf(FPS,"FPS: %.2f ",(1000/Stats->TiempoFrame));

	cvInitFont( &fuente1, CV_FONT_HERSHEY_PLAIN, 1.1, 1.1, 0, 1, 8);
	cvInitFont( &fuente2, CV_FONT_HERSHEY_PLAIN, 0.9, 0.9, 0, 1, 8);

	CvSize textsize = getTextSize(TProcesF, CV_FONT_HERSHEY_PLAIN, 1.1, 1, 0);

	// estadísticas Frame
	cvPutText( Window, NFrame, cvPoint( 11,32), &fuente1, CVX_BLUE );
	cvPutText( Window, TProcesF, cvPoint( 11,52), &fuente2, CVX_WHITE );
	cvPutText( Window, TProces, cvPoint( 11,72), &fuente2, CVX_WHITE);
	cvPutText( Window, PComplet, cvPoint( 11,92), &fuente2, CVX_WHITE);
	cvPutText( Window, FPS, cvPoint( 11,112), &fuente1, CVX_RED);
	// Barra de progreso
	float contadorX = 0;
	float x;
//	if( Stats->totalFrames > 0){
//		contadorX = contadorX + (visParams->BPrWidth*Stats->numFrame/ Stats->totalFrames);
//		x =  contadorX ;
//		cvRectangle( Window, cvPoint(  (Window->width-visParams->BPrWidth )/2,( Window->height - 30 - Window->height/64) ),
//							cvPoint( (Window->width-visParams->BPrWidth )/2 + cvRound(x), (Window->height - 30 + Window->height/64) ), CVX_BLUE, -1 );
//	}
}
// Genera una imagen que representa el llenado del buffer
void VerEstadoBuffer( IplImage* Imagen,int num, VisParams* params,int max ){
	static int count = 0 ;
	static float anchoBuf; // longitud en pixels del ancho del buffer
	char PComplet[100];
	CvFont fuente1;
	CvFont fuente2;
	float porcentaje;

	if (SHOW_VISUALIZATION == 1 || GRABAR_VISUALIZACION == 1){


		//if(!ImVisual) ImVisual = cvCreateImage( cvGetSize( frameData->Frame ),8,3);
		if(!Window){
			Window = cvCreateImage( params->Resolucion,8,3);
		}
		if(Imagen->width <= 320){
			SetVisParams(params, ImScale );
		}
		anchoBuf = Window->width/2;
		cvZero( Window);
		// rectángulo roi video
//		cvRectangle( Window, cvPoint(params->ROITracking.x,params->ROITracking.y),cvPoint(params->ROITracking.x + params->ROITracking.width,params->ROITracking.y + params->ROITracking.height), CVX_WHITE, 1 );
		// rectángulo exterior
		cvRectangle( Window, cvPoint(  (Window->width-anchoBuf)/2-2,( Window->height/2 + 60 - Window->height/64)-2 ),
						cvPoint((Window->width + anchoBuf)/2+2, (Window->height/2 + 60 + Window->height/64)+2 ), CVX_WHITE, 1 );
		// rectángulo interior
		count = count + Window->width/2/max;
		porcentaje = (count/anchoBuf)*100;
		if( num == max-1 ){
			count = Window->width/2;
			porcentaje = 100;
		}
		cvRectangle( Window, cvPoint( (Window->width-anchoBuf)/2,Window->height/2 + 60 - Window->height/64 ),
		cvPoint( (Window->width-anchoBuf)/2 + count, Window->height/2 + 60 + Window->height/64) , CVX_GREEN, -1 );
		// rectangulo imagen
//		cvRectangle( Window, cvPoint(params->ROIPreProces.x,params->ROIPreProces.y),cvPoint(params->ROIPreProces.x + params->ROIPreProces.width,params->ROIPreProces.y + params->ROIPreProces.height), CVX_WHITE, 1 );
//		contadorX = contadorX + (visParams->BPrWidth*Stats->numFrame/ Stats->totalFrames);
//				x =  contadorX ;

		sprintf(PComplet," %.0f %% ",porcentaje );
	//	cvInitFont( &fuente1, CV_FONT_HERSHEY_PLAIN, 1, 1, 0, 1, 8);

		cvInitFont( &fuente2, CV_FONT_HERSHEY_PLAIN, 2, 2, 0, 1, 8);
		CvSize textsize1;
		CvSize textsize2;
		textsize1 = getTextSize("Llenando Buffer...", CV_FONT_HERSHEY_PLAIN, 2, 2, 0);
		textsize2 = getTextSize(PComplet, CV_FONT_HERSHEY_PLAIN, 2, 2, 0);

		//cvInitFont( &fuente1, CV_FONT_HERSHEY_PLAIN, 2, 2, 0, 1, 8);
		cvPutText( Window, PComplet,  cvPoint((Window->width-textsize2.width)/2, Window->height/2), &fuente2, CVX_WHITE );
		cvPutText( Window, "Llenando Buffer...", cvPoint( (Window->width-textsize1.width)/2,params->ROIPreProces.y + params->ROIPreProces.height + 30), &fuente2, CVX_WHITE );


		if( num == IMAGE_BUFFER_LENGTH - 1){
			if(SHOW_WINDOW) Transicion3( "",params, 20 );
		}

		cvShowImage( "TrackingDrosophila", Window );
	}
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


void visualizarBuffer( tlcde* Buffer,StaticBGModel* Flat, VisParams* Params,CvVideoWriter* writer ){

	STFrame* frameData;

	unsigned char opcion = 0;

	Params->Grab = false;
	fflush( stdin);


	irAl( Params->VisualPos,Buffer );
	frameData = (STFrame*)obtenerActual(Buffer);
//	cvCopy(frameData->Frame,ImVisual);


	VisualizarFr( frameData, Flat, writer,Params);
	CvFont fuente1;
	char PBuf[100];
	sprintf(PBuf," Posicion del Buffer: %d ",Params->VisualPos );
	cvInitFont( &fuente1, CV_FONT_HERSHEY_PLAIN, 1, 1, 0, 1, 8);
	cvPutText( ImVisual, PBuf,  cvPoint( 10,frameData->Frame->height-10), &fuente1, CVX_RED );
	cvShowImage( "Visualización", ImVisual );
	opcion = cvWaitKey(0);
	if( opcion == 'C' || opcion == 'c' ){
			Params->stop = false;
	}
	// si pulsamos +, visualizamos el siguiente elemento del buffer
	if( opcion == 43 ){
		if (Params->VisualPos < Buffer->numeroDeElementos-1) Params->VisualPos=Params->VisualPos+1;
	}
	// si pulsamos -, visualizamos el elemento anterior del buffer
	if( opcion == 45 ){
		if ( Params->VisualPos > 0 ) Params->VisualPos=Params->VisualPos-1;
	}
	// si pulsamos i ó I, visualizamos el primer elemento del buffer
	if( opcion == 49 || opcion == 69)  {
		Params->VisualPos = 0;
	}
	// si pulsamos f +o F, visualizamos el último elemento del buffer
	if( opcion == 70 || opcion == 102)  {
		Params->VisualPos = Buffer->numeroDeElementos-1;
	}
	// tomar instantanea del frame
	if( opcion == 'g' || opcion == 'G'){
		irAl( Params->VisualPos, Buffer );
		frameData = (STFrame*)obtenerActual(Buffer);
		cvSaveImage( "Captura.jpg", frameData->Frame);
	}
}

void DibujarFondo( VisParams* params){

	CvFont fuente1;

	cvInitFont( &fuente1, CV_FONT_HERSHEY_PLAIN, 1, 1, 0, 1, 8);
	cvPutText( Window, "Progreso:",
							cvPoint( (Window->width-params->BPrWidth )/2-2, Window->height - 50 ),
							&fuente1,
							CVX_WHITE );
	// rectangulo de imagen
	cvRectangle( Window, cvPoint(params->ROITracking.x-1,params->ROITracking.y-1),cvPoint(params->ROITracking.x + params->ROITracking.width+1,params->ROITracking.y + params->ROITracking.height+1), CVX_WHITE, -1 );
	// rectangulo de barra progreso
	cvRectangle( Window, cvPoint(  (Window->width-params->BPrWidth )/2-2,( Window->height - 30 - Window->height/64)-2 ),
							cvPoint((Window->width + params->BPrWidth)/2+2, (Window->height - 30 + Window->height/64)+2 ), CVX_WHITE, 1 );
	// rectangulos de datos estadisticos
	cvRectangle( Window, cvPoint(  10,10 ),
						cvPoint( params->ROITracking.x-10-1, (params->ROITracking.y + params->ROITracking.height)/2-5), cvScalar(109, 109, 109), -1 );
		cvRectangle( Window, cvPoint(  10,(params->ROITracking.y + params->ROITracking.height)/2+5 ),
						cvPoint(params->ROITracking.x-10-1, params->ROITracking.y + params->ROITracking.height+1 ), cvScalar(109, 109, 109), -1 );
		cvRectangle( Window, cvPoint(  10,params->ROITracking.y + params->ROITracking.height+1+ 10),
						cvPoint(Window->width - 10, Window->height - 65), cvScalar(109, 109, 109), -1 );

}
void Incrustar( IplImage* src1, IplImage* src2, IplImage* dst, CvRect ROI ){

	int y2 = 0;
	for (int y = ROI.y; y < ROI.y + src2->height; y++){
		uchar* ptr1 = (uchar*) ( src1->imageData + y*src1->widthStep + 3*ROI.x);
		uchar* ptr2 = (uchar*) ( src2->imageData + y2*src2->widthStep );
		for (int x = 0; x<src2->width; x++){
			ptr1[3*x ] = ptr2[3*x ];
			ptr1[3*x +1] = ptr2[3*x + 1 ];
			ptr1[3*x +2] = ptr2[3*x + 2 ];

		}
		y2++;;
	}
}

void IncrustarTxt(VisParams* VParams, int num){
	    CvFont fuente1;
	    CvFont fuente2;
	    CvPoint Orig;
		char Texto[100];

		cvInitFont( &fuente1, CV_FONT_HERSHEY_PLAIN, 2, 2, 0, 1, 8);
		cvInitFont( &fuente2, CV_FONT_HERSHEY_PLAIN, 1, 1, 0, 1, 8);
		CvSize textsize ;
		switch( num ){
			case 1:
				sprintf(Texto,"Buscando plato...");
					textsize = getTextSize(Texto, CV_FONT_HERSHEY_PLAIN, 2, 1, 0);
					cvPutText( Window, Texto, cvPoint( (Window->width-textsize.width)/2,VParams->ROIPreProces.y + VParams->ROIPreProces.height + 30), &fuente1, CVX_RED );

				break;
			case 2:
				sprintf(Texto,"Aprendiendo fondo...");
				textsize = getTextSize(Texto, CV_FONT_HERSHEY_PLAIN, 2, 1, 0);
				cvPutText( Window, Texto, cvPoint( (Window->width-textsize.width)/2,VParams->ROIPreProces.y + VParams->ROIPreProces.height + 30), &fuente1, CVX_RED );
				break;
			case 3:
				sprintf(Texto,"Aprendiendo forma...");
				textsize = getTextSize(Texto, CV_FONT_HERSHEY_PLAIN, 2, 1, 0);
				cvPutText( Window, Texto, cvPoint( (Window->width-textsize.width)/2,VParams->ROIPreProces.y + VParams->ROIPreProces.height + 30), &fuente1, CVX_RED );
				break;
			case 4:
//				sprintf(Texto,"Rastreando...");
//				textsize = getTextSize(Texto, CV_FONT_HERSHEY_PLAIN, 2, 2, 0);
//				cvPutText( Window, Texto, cvPoint( (Window->width-textsize.width)/2,VParams->ROITracking.y + VParams->ROITracking.height + 30), &fuente1, CVX_RED );
			break;
			case 5:

				break;
		}
}

void IncrustarLogo(const char Cad[100] , IplImage* ImLogos, CvPoint Origen,int Delay, bool Clear){

	IplImage* Logo = NULL;
	CvPoint CentroIm;
	CvPoint Centrar;
	//CreateWindows( ImVisual );
	Logo = cvLoadImage( Cad ,1);
	if(!Logo) return;
	if( Origen.x == -1 || Origen.y == -1){
		Centrar = cvPoint( (ImLogos->width-Logo->width)/2,( ImLogos->height-Logo->height)/2);
		Origen = Centrar;
	}
	if( Origen.x == -2 || Origen.y == -2){
			Centrar = cvPoint( (Window->width-Logo->width)/2,( Window->height-Logo->height)/4 );
			Origen = Centrar;
	}
	if( Origen.x == -3 || Origen.y == -3){
				Centrar = cvPoint( (ImLogos->width-Logo->width)/2,(( ImLogos->height+Logo->height)/4));
				Origen = Centrar;
		}
	Incrustar( ImLogos, Logo, ImLogos,cvRect(Origen.x,Origen.y,ImLogos->width,ImLogos->height)  );
	cvShowImage("TrackingDrosophila",ImLogos);
	cvWaitKey(Delay);
	if( Clear ) cvZero( ImLogos );
	cvReleaseImage(&Logo);
}

void desvanecer( IplImage* Imagen ,int Delay, CvRect ROI ){

	if ( Imagen == NULL ) Imagen = Window;
	cvSetImageROI(Imagen,ROI);
	for( int i = 0; i < 255; i += 4 )
	{
		cvAddS(Imagen,cvScalar(-i,-i,-i), Imagen, NULL );
		if( ROI.x >= 0 && ROI.y >= 0 && ROI.x < Imagen->width && ROI.y < Imagen->height ){
			cvResetImageROI(Imagen);
			cvShowImage("TrackingDrosophila", Window);
			cvSetImageROI(Imagen,ROI);
		}
		else cvShowImage("TrackingDrosophila", Window);
		if(i<100) waitKey(Delay);
	}
	cvResetImageROI(Imagen);
	cvZero(Imagen);
}
// Transicione entre el inicio, el preprocesado y el procesado
void Transicion( const char texto[], int delay_up, int delay_on,int delay_down){

	CvFont fuente1;

	CvRect rect = cvRect(0,0,Window->width,Window->height);
	cvInitFont( &fuente1, CV_FONT_HERSHEY_PLAIN, 2, 2, 0, 1, CV_AA);
	CvSize textsize = getTextSize(texto, CV_FONT_HERSHEY_PLAIN, 2, 1, 0);

	desvanecer( Window, 20 ,rect);
	for( int i = 0; i < 255; i += 2 )
	{
		cvPutText( Window, texto,
				cvPoint( (Window->width-textsize.width)/2,( Window->height-textsize.height)/2),
				&fuente1,
				cvScalar(i,i,i) );
		cvShowImage("TrackingDrosophila", Window);
		cvWaitKey(delay_up);
	}
	cvWaitKey(delay_on);
	desvanecer( Window, delay_down , rect);

}

// transición entre partes del preprocesado
void Transicion2( const char texto[],VisParams* params, int delay_up ){

	CvFont fuente1;

	CvRect rect = cvRect(0,0,Window->width,Window->height);
	cvInitFont( &fuente1, CV_FONT_HERSHEY_PLAIN, 2, 2, 0, 1, 8);
	CvSize textsize = getTextSize(texto, CV_FONT_HERSHEY_PLAIN, 2, 1, 0);
	//desvanecer( Window, 20 ,rect);
	for( int i = 0; i < 255; i += 4 )
		{
		cvRectangle( Window, cvPoint(params->ROIPreProces.x,params->ROIPreProces.y),
				cvPoint(params->ROIPreProces.x + params->ROIPreProces.width,params->ROIPreProces.y + params->ROIPreProces.height),
				cvScalar(i,i,i), 1 );
			cvShowImage("TrackingDrosophila", Window);
			cvWaitKey(delay_up);
		}
	for( int i = 0; i < 255; i += 2 )
	{
		cvPutText( Window, texto,
				cvPoint( (Window->width-textsize.width)/2,params->ROIPreProces.y + params->ROIPreProces.height + 30),
				&fuente1,
				cvScalar(i,i,i) );
		cvShowImage("TrackingDrosophila", Window);
		cvWaitKey(delay_up);
	}

}
///Transición tras llenar el buffer
void Transicion3( const char texto[],VisParams* params, int delay_up ){

	CvFont fuente1;
	CvFont fuente2;

	CvRect rect = cvRect(0,0,Window->width,Window->height);
	cvInitFont( &fuente1, CV_FONT_HERSHEY_PLAIN, 2, 2, 0, 1, 8);
	cvInitFont( &fuente2, CV_FONT_HERSHEY_PLAIN, 1, 1, 0, 1, 8);
	CvSize textsize = getTextSize("Progreso:", CV_FONT_HERSHEY_PLAIN, 1, 1, 0);
	desvanecer( Window, 20 ,rect);
	// dibujar rectángulo de la imagen
	for( int i = 0; i < 255; i += 4 )
		{
		cvRectangle( Window, cvPoint(params->ROITracking.x-1,params->ROITracking.y-1),
				cvPoint(params->ROITracking.x + params->ROITracking.width+1,params->ROITracking.y + params->ROITracking.height+1), cvScalar(i,i,i), 1 );

		cvShowImage("TrackingDrosophila", Window);
		cvWaitKey(delay_up);
		}

	// Dibujar rectangulo exterior de la barra de progreso y rectangulos de estadísticas
	for( int i = 0; i < 255; i += 2 )
		{
		cvPutText( Window, "Progreso:",
						cvPoint( (Window->width-params->BPrWidth )/2-2, Window->height - 50 ),
						&fuente2,
						cvScalar(i,i,i) );
		cvRectangle( Window, cvPoint(  (Window->width-params->BPrWidth )/2-2,( Window->height - 30 - Window->height/64)-2 ),
						cvPoint((Window->width + params->BPrWidth)/2+2, (Window->height - 30 + Window->height/64)+2 ), cvScalar(i,i,i), 1 );

		cvShowImage("TrackingDrosophila", Window);
		cvWaitKey(delay_up);

		}
	for( int i = 0; i < 105; i += 2 )
	{
		cvRectangle( Window, cvPoint(  10,10 ),
						cvPoint( params->ROITracking.x-10-1, (params->ROITracking.y + params->ROITracking.height)/2-5), cvScalar(i, i, i), -1 );
		cvRectangle( Window, cvPoint(  10,(params->ROITracking.y + params->ROITracking.height)/2+5 ),
						cvPoint(params->ROITracking.x-10-1, params->ROITracking.y + params->ROITracking.height+1 ), cvScalar(i, i, i), -1 );
		cvRectangle( Window, cvPoint(  10,params->ROITracking.y + params->ROITracking.height+1+ 10),
						cvPoint(Window->width - 10, Window->height - 65), cvScalar(i, i, i), -1 );

		cvShowImage("TrackingDrosophila", Window);
		cvWaitKey(delay_up);

	}
	// Texto de estado
	for( int i = 0; i < 255; i += 2 )
	{
		cvPutText( Window, texto,
				cvPoint( (Window->width-textsize.width)/2,params->ROITracking.y + params->ROITracking.height + 30),
				&fuente1,
				cvScalar(i,i,i) );
		cvShowImage("TrackingDrosophila", Window);
		cvWaitKey(delay_up);
	}


}
// Transicion entre partes del preprocesado
void Transicion4(const char texto[],VisParams* params, int delay_down){

	CvFont fuente1;
	cvInitFont( &fuente1, CV_FONT_HERSHEY_PLAIN, 2, 2, 0, 1, 8);
	CvSize textsize ;
	textsize = getTextSize(texto, CV_FONT_HERSHEY_PLAIN, 2, 1, 0);
	cvPutText( Window, texto, cvPoint( (Window->width-textsize.width)/2,params->ROIPreProces.y + params->ROIPreProces.height + 30), &fuente1, CVX_GREEN );
	CvRect rect = cvRect(params->ROIPreProces.x +1,
						params->ROIPreProces.y +1,params->ROIPreProces.width -1,
						params->ROIPreProces.height -1);
	desvanecer( NULL, delay_down, rect);
}
void AllocDefaultVisParams( VisParams** visParams, IplImage* ImRef ){
    //init parameters
	VisParams *visparams;
	// si no han sido localizados hacerlo.

		visparams = ( VisParams *) malloc( sizeof( VisParams) );
		if(!visparams) {error(4); return;}
		visparams->pause = false;
		visparams->stop = false;
		visparams->Grab = false;
		visparams->VisualPos = -1;
		visparams->Resolucion = cvSize(1280,800 );
		if(!Window)	Window = cvCreateImage( visparams->Resolucion,8,3);
		if( ImRef ){
			if( ImRef->width <= 320){
			visparams->ROITracking = cvRect(visparams->Resolucion.width-(2*ImRef->width + 10),10,2*ImRef->width,2*ImRef->height);
			visparams->ROIPreProces = cvRect( (Window->width-2*ImRef->width)/2,( Window->height-2*ImRef->height)/2,2*ImRef->width,2*ImRef->height);
			}
			else{
				visparams->ROITracking = cvRect(visparams->Resolucion.width-(ImRef->width + 10),10,ImRef->width,ImRef->height);
				visparams->ROIPreProces = cvRect( (Window->width-ImRef->width)/2,( Window->height-ImRef->height)/2,ImRef->width,ImRef->height);
			}
		}
		visparams->DelayLogo = 2000;
		*visParams = visparams;

}
void SetVisParams(VisParams* visparams,IplImage* ImRef){

		visparams->pause = false;
		visparams->stop = false;
		visparams->Grab = false;
		visparams->VisualPos = -1;
		visparams->Resolucion = cvSize(1280,800 );
		if( ImRef ){
			visparams->ROITracking = cvRect(visparams->Resolucion.width-(ImRef->width + 10),10,ImRef->width,ImRef->height);
			visparams->ROIPreProces = cvRect( (Window->width-ImRef->width)/2,( Window->height-ImRef->height)/2,ImRef->width,ImRef->height);
		}
		visparams->DelayLogo = 2000;
		visparams->BPrWidth = visparams->Resolucion.width - 20;
	}


void releaseVisParams( VisParams *Parameters){
	free( Parameters);
	cvReleaseImage(&ImVisual);
	cvReleaseImage( &ImScale);
	cvReleaseImage(&Window);
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
void CreateWindows( IplImage* ImRef){

//	cvNamedWindow( "Drosophila.avi", CV_WINDOW_AUTOSIZE );
	if (SHOW_BG_REMOVAL == 1){
		cvNamedWindow( "Background",CV_WINDOW_AUTOSIZE);
		cvNamedWindow( "Foreground",CV_WINDOW_AUTOSIZE);
		cvMoveWindow("Background", 0, ImRef->height );
		cvMoveWindow("Foreground", ImRef->width, ImRef->height);
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
			cvMoveWindow("Visualización", 0, 0 );
			cvNamedWindow( "TrackingDrosophila",CV_WINDOW_AUTOSIZE);
			cvMoveWindow("TrackingDrosophila", 0, 0 );
	}
	//        cvNamedWindow( "Imagen", CV_WINDOW_AUTOSIZE);
    //	cvNamedWindow( "Region_Of_Interest", CV_WINDOW_AUTOSIZE);


}
// Destrucción de ventanas y parametros de visualización
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

}

