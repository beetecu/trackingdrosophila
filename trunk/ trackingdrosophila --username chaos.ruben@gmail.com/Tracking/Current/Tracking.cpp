/*!
 * Tracking.cpp
 *
 * Cuerpo principal del algoritmo de rastreo.
 *
 *  Created on: 27/06/2011
 *      Author: chao
 */

#include "Tracking.h"

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
	int ok = Inicializacion(frame, &Flat, &NewCap, &Shape, &BGParams,&BGModel, argc, argv);
	if (!ok ) return -1;

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
		Tracking( );
		//////////  VISUALIZAR     ////////////
		Visualizacion();
		//////////  GUARDAR DATOS ////////////
		// Se mantienen en memoria las estructuras correspondientes a STRUCT_BUFFER_LENGTH frames
		// ( buffer de datos) e IMAGE_BUFFER_LENGHT ( buffer de imagenes ).
		if( FrameCountRel >= STRUCT_BUFFER_LENGTH){

			mostrarLista(Flie);
			// Una vez que se llenan los buffer se almacenan los datos del primer frame en fichero
			AlmacenarDatos( Flie, nombreFichero );
			 // por cada nuevo frame se libera el espacio del primer frame y se
			 // actualizan los punteros del siguiente frame
			LiberarMemoria( &Flie , &Capa );

		}
	}
    ///////// POSTPROCESADO //////////
	AnalisisEstadistico();

	///////// LIBERAR MEMORIA ////////

	// completar. tras añadir buffers liberar memoria de todos los elementos (no solo del puntero actual)
	free(Capa);
	free(BGModel);
	free(Flat);
	free(Flie);
	DeallocateImages( );
	DeallocateImagesBGM();

	cvReleaseCapture(&g_capture);

	DestroyWindows( );

}


int Inicializacion( IplImage* frame,
					STFlat** Flat,
					STCapas** Capa ,
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

	// Iniciar estructura del buffer de las capas y crear imagenes
//	BuffSTCapas **buf = NULL;
//	int last = IMAGE_BUFFER_LENGTH; // indice al ultimo elemento del buffer.

//	if( buf == 0 ) {
//		buf = (IplImage**)malloc(IMAGE_BUFFER_LENGTH*sizeof(buf[0]));
//		memset( buf, 0, IMAGE_BUFFER_LENGTH*sizeof(buf[0]));
//	}
	// creación de imagenes a utilizar


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

	// Iniciar estructura para capas del nuevo frame
	STCapas* newcap = NULL;
	newcap = ( STCapas *) malloc( sizeof( STCapas));
	InitNewFrameCaps( frame, newcap );
	*Capa = newcap;
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

//		ShapeModel( g_capture, Shape , Capa->ImFMask, Flat->DataFROI );


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
	cvCopy( BGModel->Imed, NewCap->BGModel);
	cvCopy( BGModel->IDesv, NewCap->IDesv);

	return hecho = 1;
}

/*  Esta función realiza las siguientes acciones:
 *
 * - Limpieza del foreground en tres etapas :
 *   1_Actualización de fondo y resta de fondo obteniendo el foreground
 *   2_Nueva actualización y resta usando la máscacara de foreground
 *   Redefinición de la máscara de foreground mediante ajuste por elipses ( en segmentacion)
 *   3_Repetir de nuevo el ciclo de actualizacion-resta-segmentación obteniendo el foreground y
 *   el background definitivo
 * - Obtención de los parámetros de los blobs ( en segmentación )
 * - Rellena la lista lineal doblemente enlazada ( FlieTemp )con los datos de cada uno de los blobs
 * - Rellena la estructura NewCap con las nuevas imagenes.
 * - Finalmente enlaza la lista FlieTemp a la lista (buffer) Flie
 *   y la estructura NewCap al buffer Capas   */
void Procesado(){

	gettimeofday(&ti, NULL);

	ImPreProcess( frame, Imagen, BGModel->ImFMask, 0, Flat->DataFROI);

	cvCopy( NewCap->BGModel, BGTemp); // guardamos una copia del modelo original
	cvCopy(NewCap->IDesv,DETemp);	//NewCap se sobrescribe
	// Iniciar estructura para capas del nuevo frame

	STCapas* NewCap = NULL;
	NewCap = ( STCapas *) malloc( sizeof( STCapas));

	InitNewFrameCaps( Imagen, NewCap );

	cvCopy(  BGTemp,NewCap->BGModel);
	cvCopy(DETemp,NewCap->IDesv);


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
		// Primera actualización del fondo
		// establecer parametros
		InitialBGModelParams( BGParams);

		UpdateBGModel( Imagen, NewCap->BGModel,NewCap->IDesv, BGParams, Flat->DataFROI, NewCap->FG );

		gettimeofday(&tf, NULL);
		TiempoParcial= (tf.tv_sec - ti.tv_sec)*1000 + \
										(tf.tv_usec - ti.tv_usec)/1000.0;
		printf("Background update: %5.4g ms\n", TiempoParcial);

		/////// BACKGROUND DIFERENCE. Obtención de la máscara del foreground
		gettimeofday(&ti, NULL);

		BackgroundDifference( Imagen, NewCap->BGModel,NewCap->IDesv, NewCap->FG ,BGParams, Flat->DataFROI);

		gettimeofday(&tf, NULL);
		TiempoParcial= (tf.tv_sec - ti.tv_sec)*1000 + \
									(tf.tv_usec - ti.tv_usec)/1000.0;
		printf("Obtención de máscara de Foreground : %5.4g ms\n", TiempoParcial);
		/////// SEGMENTACION
		if( i > 0 ){
			gettimeofday(&ti, NULL);
			printf( "Segmentando Foreground...");

			segmentacion(Imagen, NewCap, Flat->DataFROI,&FlieTemp,BGModel->ImFMask);

			gettimeofday(&tf, NULL);
			TiempoParcial= (tf.tv_sec - ti.tv_sec)*1000 + \
									(tf.tv_usec - ti.tv_usec)/1000.0;
			printf(" %5.4g ms\n", TiempoParcial);
		}
		// en la ultima iteracion nos kedamos con ultimo BGModel obtenido
		if (i < 2 ){
			cvCopy( BGTemp, NewCap->BGModel );
			cvCopy( DETemp, NewCap->IDesv );
		}
		/////// VALIDACIÓN
		// solo en la última iteracion
//			if (i > 1){
//				gettimeofday(&ti, NULL);
//				printf( "\nValidando contornos...");
//
//		//		Validacion(Imagen, Capa , Shape, Flat->DataFROI, Flie, NULL, NULL);
//
//				gettimeofday(&tf, NULL);
//				TiempoParcial= (tf.tv_sec - ti.tv_sec)*1000 +
//										(tf.tv_usec - ti.tv_usec)/1000.0;
//				printf(" %5.4g ms\n", TiempoParcial);
//			}

	}
	// Una vez validada añadimos ( al final ) las estructuras a las listas (buffers).
	AnyadirCapas( &NewCap, &Capa);
	AnyadirFlies( &FlieTemp, &Flie );

}
void Tracking(){
	gettimeofday(&ti, NULL);
	cvZero( Capa->ImMotion);
	if ( SHOW_MOTION_TEMPLATE == 1){
		MotionTemplate( Capa->FG, Capa->ImMotion);
	}


//		OpticalFlowLK( Capa->FGTemp, ImOpFlowX, ImOpFlowY );

	cvCircle( Capa->ImMotion, cvPoint( Flat->PCentroX,Flat->PCentroY ), 3, CV_RGB(0,255,0), -1, 8, 0 );
	cvCircle( Capa->ImMotion, cvPoint(Flat->PCentroX,Flat->PCentroY ),Flat->PRadio, CV_RGB(0,255,0),2 );
	gettimeofday(&tf, NULL);
	TiempoParcial= (tf.tv_sec - ti.tv_sec)*1000 + \
											(tf.tv_usec - ti.tv_usec)/1000.0;
	printf("Tracking: %5.4g ms\n", TiempoParcial);
}

void InitNewFrameCaps(IplImage* I, STCapas *Capa ){

	CvSize size = cvGetSize( I );
	Capa->BGModel = cvCreateImage(size,8,1);
	Capa->FG = cvCreateImage(size,8,1);
	Capa->IDesv = cvCreateImage(size,8,1);
	Capa->OldFG = cvCreateImage(size,8,1);
	Capa->ImMotion = cvCreateImage( size, 8, 3 );
	Capa->ImMotion->origin = I->origin;
	cvZero( Capa->BGModel );
	cvZero( Capa->FG );
	cvZero( Capa->IDesv );
	cvZero( Capa->ImMotion);

	Capa->anterior_frame = NULL;
	Capa->siguiente_frame = NULL;
	Capa->num_frame = FrameCountAbs;
}

///! - Añade la estuctura de las capas correspondientes al nuevo frame al final de la
///!   lista lineal doblemente enlazada ( buffer de imagenes ).
void AnyadirCapas( STCapas** NewCap, STCapas** Capa){

	STCapas* Nuevo;
	STCapas* Actual;
	Actual = *Capa;
	Nuevo = *NewCap;
	// Primera iteración
	if ( !Actual){
		Nuevo->num_elementos = 1;
		*Capa = Nuevo;
	}
	else{

		Nuevo->anterior_frame = Actual;
	//	Nuevo->siguiente_frame = NULL;
		Actual->siguiente_frame = Nuevo;

		Nuevo->num_elementos = Actual->num_elementos + 1;
		*Capa = Nuevo;
	}
}

///! - Añade al final de la lista Flie los nuevos elementos ya validados de FlieTemp.
///! - Se hace que cada elemento del frame anterior apunte al primero del frame siguiente
///! - Se hace que cada elemento del frame siguiente apunte al primero del frame anterior
void AnyadirFlies( STFlies** FlieTemp, STFlies **Flie ){

	STFlies* Nuevo;
	STFlies* Actual;
	Actual = *Flie;
	Nuevo = *FlieTemp;
	// Primera iteracion
	if (Actual == NULL){
		// Nuevo al primer elemento y de paso rellenamos el numero de frame
		for( int k = 0; k < Nuevo->num_Flies_frame - 1 ; k++ ) {
			Nuevo->num_frame = FrameCountAbs;
			Nuevo = Nuevo->anterior;
		}
		Nuevo->num_frame = FrameCountAbs;
		Nuevo->anterior = NULL;
		Nuevo->anterior_frame = NULL;
		// Nuevo al último.
		for( int k = 0; k < Nuevo->num_Flies_frame - 1 ; k++ ) {
			Nuevo = Nuevo->siguiente;
			Nuevo ->siguiente_frame = NULL;
		}
		Nuevo ->siguiente_frame = NULL;
		*Flie = Nuevo;
	}
	else{
		Actual = *Flie;

		// Enlazamos ambas listas. Vamos hasta el primer elemento de FlieTemp ( nuevo frame )
		for( int i = 0 ; i < Nuevo->num_Flies_frame - 1; i++,Nuevo = Nuevo->anterior){
			Nuevo->num_frame = FrameCountAbs; // rellenamos el numero de frame
		}
		Nuevo->num_frame = FrameCountAbs;
		// El último elemento de Actual apunta al primero de Nuevo
		Actual->siguiente = Nuevo;
		// El primer elemento de FlieTemp apunta al último de Flie
		Nuevo->anterior = Actual;

		// Enlazamos frame actual y nuevo.

		// regresamos el puntero Actual al primer elemento del frame actual
		for( int i = 0 ; i < Actual->num_Flies_frame - 1; i++, Actual = Actual->anterior);
		// Todos los elementos del frame nuevo tendrán un miembro que apunta el primer
		// elemento del frame actual
		for( int i = 0 ; i < Nuevo->num_Flies_frame -1; i++, Nuevo = Nuevo->siguiente){
			Nuevo->anterior_frame = Actual;
		}
		Nuevo->anterior_frame = Actual;
		Nuevo->siguiente_frame = NULL; // El último a null
		// regresamos el puntero Nuevo al primer elemento del frame nuevo
		for( int i = 0 ; i < (Nuevo->num_Flies_frame - 1) ; i++, Nuevo =Nuevo->anterior);
		// Todos los elementos del frame actual tienen en su estructura un miembro que
		// apunta al primer elemento del frame nuevo.

		for (int j = 0; j < (Actual->num_Flies_frame - 1) ; j++, Actual= Actual->siguiente ){
			Actual->siguiente_frame = Nuevo;
		}
		Actual->siguiente_frame = Nuevo;
		// flie siempre quedará apuntando al último elemento
		while( Nuevo->siguiente != NULL) Nuevo = Nuevo->siguiente;
		*Flie = Nuevo;
	}
}


///! El algoritmo mantiene un buffer de 50 frames ( 2 seg aprox ) para los datos y para las
///! imagenes.
///! Una vez que se han llenado los buffer, se almacena en fichero los datos correspondientes
///! al primer frame
void AlmacenarDatos( STFlies* Flie , char *nombreFichero){

	FILE * pf;

	// Abrir el fichero nombreFichero para escribir "w".
	if ((pf = fopen(nombreFichero, "a")) == NULL)
	{
	printf("El fichero no puede abrirse.");
	exit(1);
	}
	// nos movemos al principio de la estructura
	while( Flie->anterior != NULL) Flie = Flie->anterior;
	int frame_actual = Flie->num_frame;
	// almacenamos el primer frame del buffer en el fichero.
	do
	{
		// Almacenar un registro en el fichero.
		fwrite(&Flie, sizeof(STFlies), 1, pf);
		if (ferror(pf))
		{
		  perror("Error durante la escritura");
		  exit(2);
		}
		Flie = Flie->siguiente;
	}
	while (Flie->num_frame == frame_actual);
	fclose(pf);
}

///! Libera la memoria correspondiente al primer frame de los buffer.
///! Una vez que se han llenado los buffer,
///! se libera el espacio tanto de los datos como de las imagenes del primer frame

void LiberarMemoria( STFlies **Flie, STCapas **Capa){
/* Estructura STFLies */
	STFlies* flie;
	flie = *Flie;
	while( flie->anterior_frame != NULL) flie = flie->anterior_frame;
	int frame_actual = flie->num_frame;
	while( flie->num_frame == frame_actual){
		*Flie = flie;
		flie = flie->siguiente;
		free(*Flie);
		*Flie = NULL;
	}
	/// reasignamos punteros del segundo frame ( ahora primero )
	flie->anterior = NULL;
	frame_actual = flie->num_frame;
	while( frame_actual == flie->num_frame ){
		flie->anterior_frame = NULL;
	    flie = flie->siguiente;
	}
	*Flie = flie;
	/// regresamos puntero al final del buffer
	while( flie->siguiente != NULL) flie = flie->siguiente;
	*Flie = flie;

/* Buffer imagenes */
	STCapas *Cap;
	Cap = *Capa;
	/// regresamos el puntero al inicio del buffer
	while( Cap->anterior_frame != NULL) Cap = Cap->anterior_frame;
	cvReleaseImage(&Cap->BGModel);
	cvReleaseImage(&Cap->FG);
	cvReleaseImage(&Cap->IDesv);
	cvReleaseImage(&Cap->OldFG);
	cvReleaseImage(&Cap->ImMotion);
	*Capa = Cap;
	Cap = Cap->siguiente_frame;
	Cap->anterior_frame = NULL;
	free(*Capa);
	*Capa = NULL;
	/// regresamos puntero al final del buffer
	while( Cap->siguiente_frame != NULL) Cap = Cap->siguiente_frame;
	*Capa = Cap;
}

void Visualizacion(){
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
			cvShowImage("Background", Capa->BGModel);
//				cvShowImage( "Foreground",Capa->FG);

	//		cvWaitKey(0);
	}
	if (SHOW_OPTICAL_FLOW == 1){
	cvShowImage( "Flujo Optico X", ImOpFlowX );
	cvShowImage( "Flujo Optico Y", ImOpFlowY);
	}
	if ( SHOW_MOTION_TEMPLATE == 1){
		cvShowImage( "Motion",Capa->ImMotion);
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
}



void AnalisisEstadistico(){
	printf( "Rastreo finalizado con éxito ..." );
	printf( "Comenzando análisis estadístico de los datos obtenidos ...\n" );
	printf( "Análisis finalizado ...\n" );
}
void AllocateImages( IplImage* I , StaticBGModel* bgmodel){

	// Crear imagenes y redimensionarlas en caso de que cambien su tamaño

	CvSize size = cvGetSize( I );

//	if( !Capa->BGModel ||
//		 Capa->BGModel->width != size.width ||
//		 Capa->BGModel->height != size.height ) {
//
//		cvReleaseImage( &Capa->BGModel );
//		cvReleaseImage( &Capa->FG );
//		cvReleaseImage( &Capa->IDesv );
//		cvReleaseImage( &Capa->ImFMask );
//		cvReleaseImage( &Capa->ImRois );
//		cvReleaseImage( &Capa->OldFG );
//		cvReleaseImage( &Capa->ImMotion );




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
	cvReleaseImage( &BGTemp1);
	cvReleaseImage( &DETemp1);

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

void mostrarLista(STFlies *Flie)
{
	// Mostrar todos los elementos de la lista

	while (Flie->anterior != NULL) Flie = Flie->anterior;
	while( Flie->siguiente != NULL)
	{
		printf("\n Etiqueta %d\n  Orientacion %.02f\n  Numero frame %d\n  ",
				Flie->etiqueta,Flie->orientacion,Flie->num_frame);
		//printf("\n Vertical : %f, Horizontal: %f, Punto : %f",Flies->,Flies->VH,Flies->punto1.x );
		Flie = Flie->siguiente;
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
