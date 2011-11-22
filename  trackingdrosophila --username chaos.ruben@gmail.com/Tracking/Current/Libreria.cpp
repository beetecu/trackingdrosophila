/*!
 * Libreria.cpp
 *
 *  Created on: 12/07/2011
 *      Author: chao
 */

#include "Libreria.h"

// Tratamiento de Imagenes

int RetryCap( CvCapture* g_capture ){
	IplImage* frame = cvQueryFrame(g_capture); // intentar de nuevo
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

void invertirBW( IplImage* Imagen )
{
	//añadir para invertir con  roi
	if (Imagen->roi){
		for( int y=Imagen->roi->yOffset;  y< Imagen->roi->yOffset + Imagen->roi->height ; y++){

				uchar* ptr = (uchar*) ( Imagen->imageData + y*Imagen->widthStep + 1*Imagen->roi->xOffset);

				for (int x = 0; x<Imagen->roi->width; x++) {
					if (ptr[x] == 255) ptr[x] = 0;
					else ptr[x] = 255;
				}
		}
	}
	else{
		for( int y=0;  y< Imagen->height ; y++){
				uchar* ptr = (uchar*) ( Imagen->imageData + y*Imagen->widthStep);

				for (int x = 0; x<Imagen->width; x++) {
					if (ptr[x] == 255) ptr[x] = 0;
					else ptr[x] = 255;
				}
		}
	}
}

// Recibe la imagen del video y devuelve la imagen en un canal de niveles
// de gris con el plato extraido.

void ImPreProcess( IplImage* src,IplImage* dst, IplImage* ImFMask,bool bin, CvRect ROI){

//cvNamedWindow( "Im", CV_WINDOW_AUTOSIZE);
	// Imagen a un canal de niveles de gris
	cvCvtColor( src, dst, CV_BGR2GRAY);
	cvSetImageROI( dst, ROI );
	if (bin == true){
		cvAdaptiveThreshold( dst, dst,
			255, CV_ADAPTIVE_THRESH_MEAN_C,CV_THRESH_BINARY,75,40);
		//bin = false;
	}
// Filtrado gaussiano 5x

	cvSmooth(dst,dst,CV_GAUSSIAN,5,5);

// Extraccion del plato

	cvResetImageROI( dst );
	cvAndS(dst, cvRealScalar( 0 ) , dst, ImFMask );
}

void verMatrizIm( IplImage* Im, CvRect roi){

	for (int y = roi.y; y< roi.y + roi.height; y++){
		uchar* ptr1 = (uchar*) ( Im->imageData + y*Im->widthStep + 1*roi.x);

		printf(" \n\n"); // espacio entre filas

		for (int x = 0; x<roi.width; x++){

			if( ( y == roi.y) && ( x == 0) ){
				printf("\n Origen: ( %d , %d )",(x + roi.x),y);
				printf(" Width = %d  Height = %d \n\n",roi.width,roi.height);
			}
			printf("%d\t", ptr1[x]); // columnas

		}
	}

}

void muestrearLinea( IplImage* rawImage, CvPoint pt1,CvPoint pt2, int num_frs){

	static int count = 0;
	int max_buffer;
	int	r[num_frs];
	CvLineIterator iterator;

	if(!existe("SamplelinesPC.csv")) crearFichero("SamplelinesPC.csv");

	FILE *fptr = fopen("SLine120_220_267_5000.csv","a"); // Store the data here

	// MAIN PROCESSING LOOP:
	//
	if( count < num_frs){
		max_buffer = cvInitLineIterator(rawImage,pt1,pt2,&iterator,8,0);
		for(int j=0; j<max_buffer; j++){
			fprintf(fptr,"%d;", iterator.ptr[0]); //Write value
			CV_NEXT_LINE_POINT(iterator); //Step to the next pixel
		}
		// OUTPUT THE DATA IN ROWS:
		//
		fprintf(fptr,"\n");
		count++;
	}
	fclose(fptr);
}

///////////////////// INTERFAZ PARA MANIPULAR UNA LCDE //////////////////////////////

//


// Crear un nuevo elemento de la lista

Elemento *nuevoElemento()
{
  Elemento *q = (Elemento *)malloc(sizeof(Elemento));
  if (!q) error(4);
  return q;
}

void iniciarLcde(tlcde *lcde)
{
  lcde->ultimo = lcde->actual = NULL;
  lcde->numeroDeElementos = 0;
  lcde->posicion = -1;
}

void insertar(void *e, tlcde *lcde)
{
  // Obtener los parámetros de la lcde
  Elemento *ultimo = lcde->ultimo;
  Elemento *actual = lcde->actual;
  int numeroDeElementos = lcde->numeroDeElementos;
  int posicion = lcde->posicion;

  // Añadir un nuevo elemento a la lista a continuación
  // del elemento actual; el nuevo elemento pasa a ser el
  // actual
  Elemento *q = NULL;

  if (ultimo == NULL) // lista vacía
  {
    ultimo = nuevoElemento();
    // Las dos líneas siguientes inician una lista circular
    ultimo->anterior = ultimo;
    ultimo->siguiente = ultimo;
    ultimo->dato = e;      // asignar datos
    actual = ultimo;
    posicion = 0;          // ya hay un elemento en la lista
  }
  else // existe una lista
  {
    q = nuevoElemento();

    // Insertar el nuevo elemento después del actual
    actual->siguiente->anterior = q;
    q->siguiente = actual->siguiente;
    actual->siguiente = q;
    q->anterior = actual;
    q->dato = e;

    // Actualizar parámetros.
    posicion++;

    // Si el elemento actual es el último, el nuevo elemento
    // pasa a ser el actual y el último.
    if( actual == ultimo )
      ultimo = q;

    actual = q; // el nuevo elemento pasa a ser el actual
  } // fin else

  numeroDeElementos++; // incrementar el número de elementos

  // Actualizar parámetros de la lcde
  lcde->ultimo = ultimo;
  lcde->actual = actual;
  lcde->numeroDeElementos = numeroDeElementos;
  lcde->posicion = posicion;
}


void *borrarEl(int i,tlcde *lcde)
{
	int n = 0;
	// comprobar si la posicion está en los limites
	if ( i >= lcde->numeroDeElementos || i < 0 ) return NULL;
	// posicionarse en el elemento i
	irAlPrincipio( lcde );
	for( n = 0; n < i; n++ ) irAlSiguiente( lcde );
	// borrar
	  // Obtener los parámetros de la lcde
	  Elemento *ultimo = lcde->ultimo;
	  Elemento *actual = lcde->actual;
	  int numeroDeElementos = lcde->numeroDeElementos;
	  int posicion = lcde->posicion;

	  // La función borrar devuelve los datos del elemento
	  // apuntado por actual y lo elimina de la lista.
	  Elemento *q = NULL;
	  void *datos = NULL;

	  if( ultimo == NULL ) return NULL;  // lista vacía.
	  if( actual == ultimo ) // se trata del último elemento.
	  {
	    if( numeroDeElementos == 1 ) // hay un solo elemento
	    {
	      datos = ultimo->dato;
	      free(ultimo);
	      ultimo = actual = NULL;
	      numeroDeElementos = 0;
	      posicion = -1;
	    }
	    else // hay más de un elemento
	    {
	      actual = ultimo->anterior;
	      ultimo->siguiente->anterior = actual;
	      actual->siguiente = ultimo->siguiente;
	      datos = ultimo->dato;
	      free(ultimo);
	      ultimo = actual;
	      posicion--;
	      numeroDeElementos--;
	    }  // fin del bloque else
	  }    // fin del bloque if( actual == ultimo )
	  else // el elemento a borrar no es el último
	  {
	    q = actual->siguiente;
	    actual->anterior->siguiente = q;
	    q->anterior = actual->anterior;
	    datos = actual->dato;
	    free(actual);
	    actual = q;
	    numeroDeElementos--;
	  }

	  // Actualizar parámetros de la lcde
	  lcde->ultimo = ultimo;
	  lcde->actual = actual;
	  lcde->numeroDeElementos = numeroDeElementos;
	  lcde->posicion = posicion;

	  return datos;

}

void *borrar(tlcde *lcde)
{
  // Obtener los parámetros de la lcde
  Elemento *ultimo = lcde->ultimo;
  Elemento *actual = lcde->actual;
  int numeroDeElementos = lcde->numeroDeElementos;
  int posicion = lcde->posicion;

  // La función borrar devuelve los datos del elemento
  // apuntado por actual y lo elimina de la lista.
  Elemento *q = NULL;
  void *datos = NULL;

  if( ultimo == NULL ) return NULL;  // lista vacía.
  if( actual == ultimo ) // se trata del último elemento.
  {
    if( numeroDeElementos == 1 ) // hay un solo elemento
    {
      datos = ultimo->dato;
      free(ultimo);
      datos = NULL;
      ultimo = actual = NULL;
      numeroDeElementos = 0;
      posicion = -1;
    }
    else // hay más de un elemento
    {
      actual = ultimo->anterior;
      ultimo->siguiente->anterior = actual;
      actual->siguiente = ultimo->siguiente;
      datos = ultimo->dato;
      free(ultimo);
      ultimo = actual;
      posicion--;
      numeroDeElementos--;
    }  // fin del bloque else
  }    // fin del bloque if( actual == ultimo )
  else // el elemento a borrar no es el último
  {
    q = actual->siguiente;
    actual->anterior->siguiente = q;
    q->anterior = actual->anterior;
    datos = actual->dato;
    free(actual);
    actual = q;
    numeroDeElementos--;
  }

  // Actualizar parámetros de la lcde
  lcde->ultimo = ultimo;
  lcde->actual = actual;
  lcde->numeroDeElementos = numeroDeElementos;
  lcde->posicion = posicion;

  return datos;
}



void irAlSiguiente(tlcde *lcde)
{
  // Avanza la posición actual al siguiente elemento.
  if (lcde->posicion < lcde->numeroDeElementos - 1)
  {
    lcde->actual = lcde->actual->siguiente;
    lcde->posicion++;
  }
}

void irAlAnterior(tlcde *lcde)
{
  // Retrasa la posición actual al elemento anterior.
  if ( lcde->posicion > 0 )
  {
    lcde->actual = lcde->actual->anterior;
    lcde->posicion--;
  }
}

void irAlPrincipio(tlcde *lcde)
{
  // Hace que la posición actual sea el principio de la lista.
  lcde->actual = lcde->ultimo->siguiente;
  lcde->posicion = 0;
}

void irAlFinal(tlcde *lcde)
{
  // El final de la lista es ahora la posición actual.
  lcde->actual = lcde->ultimo;
  lcde->posicion = lcde->numeroDeElementos - 1;
}

int irAl(int i, tlcde *lcde)
{
  int n = 0;
  if (i >= lcde->numeroDeElementos || i < 0) return 0;

  irAlPrincipio(lcde);
  // Posicionarse en el elemento i
  for (n = 0; n < i; n++)
    irAlSiguiente(lcde);
  return 1;
}

void *obtenerActual(tlcde *lcde)
{
  // La función obtener devuelve el puntero a los datos
  // asociados con el elemento actual.
  if ( lcde->ultimo == NULL ) return NULL; // lista vacía

  return lcde->actual->dato;
}

void *obtener(int i, tlcde *lcde)
{
  // La función obtener devuelve el puntero a los datos
  // asociados con el elemento de índice i.
  if (!irAl(i, lcde)) return NULL;
  return obtenerActual(lcde);
}

void modificar(void *pNuevosDatos, tlcde *lcde)
{
  // La función modificar establece nuevos datos para el
  // elemento actual.
  if(lcde->ultimo == NULL) return; // lista vacía

  lcde->actual->dato = pNuevosDatos;
}

void anyadirAlFinal(void *e, tlcde *lcde ){
	irAlFinal( lcde );
	insertar( e, lcde);
}

void anyadirAlPrincipio(void *e, tlcde *lcde ){
	irAlPrincipio( lcde );
	insertar( e, lcde);
}

/////////////////////// INTERFACE PARA GESTIONAR LISTA FLIES //////////////////////////

int dibujarFG( tlcde* flies, IplImage* dst,bool clear){
	STFly* fly;
	if( flies->numeroDeElementos < 1 ) return 0;
	if( dst == NULL ) return 0;
	if ( clear ) cvZero( dst );
	irAlPrincipio( flies);
	for( int j = 0; j < flies->numeroDeElementos; j++){
		fly = (STFly*)obtener( j, flies);
		if( fly->Estado ){
			CvSize axes = cvSize( cvRound(fly->a) , cvRound(fly->b) );
			cvEllipse( dst, fly->posicion, axes, fly->orientacion, 0, 360, cvScalar( 255,0,0,0), -1, 8);
		}
	}
	return 1;
}

int dibujarBG( tlcde* flies, IplImage* dst, bool clear){
	STFly* fly;
	if( flies->numeroDeElementos < 1 ) return 0;
	if( dst == NULL ) return 0;
	if ( clear ) cvZero( dst );
	irAlPrincipio( flies);
	for( int j = 0; j < flies->numeroDeElementos; j++){
		fly = (STFly*)obtener( j, flies);
		if( !fly->Estado ){
			CvSize axes = cvSize( cvRound(fly->a) , cvRound(fly->b) );
			cvEllipse( dst, fly->posicion, axes, fly->orientacion, 0, 360, cvScalar( 255,0,0,0), -1, 8);
		}
	}
	return 1;
}

int dibujarBGFG( tlcde* flies, IplImage* dst,bool clear){
	STFly* fly;
	if( flies->numeroDeElementos < 1 ) return 0;
	if( dst == NULL ) return 0;
	if ( clear ) cvZero( dst );
	irAlPrincipio( flies);
	for( int j = 0; j < flies->numeroDeElementos; j++){
		fly = (STFly*)obtener( j, flies);
		CvSize axes = cvSize( cvRound(fly->a) , cvRound(fly->b) );
		cvEllipse( dst, fly->posicion, axes, fly->orientacion, 0, 360, cvScalar( 255,0,0,0), -1, 8);
	}
	return 1;
}


void mostrarListaFlies(int pos,tlcde *lista)
{
	int n;
	tlcde* flies;
	STFrame* frameData;
	int pos_act;
	pos_act = lista->posicion;

	if (pos >= lista->numeroDeElementos || pos < 0) return;
	  irAlPrincipio(lista);
	  // Posicionarse en el frame pos
	for (n = 0; n < pos; n++) irAlSiguiente(lista);
	// obtenemos la lista de flies
	frameData = (STFrame*)obtenerActual( lista );
	flies = frameData->Flies;
	// Mostrar todos los elementos de la lista
	int i = 0, tam = flies->numeroDeElementos;
	STFly* flydata = NULL;
	for(int j = 0; j < 6; j++){
		while( i < tam ){
			flydata = (STFly*)obtener(i, flies);

			if (j == 0){
				if (i == 0) printf( "\netiquetas");
				printf( "\t%d",flydata->etiqueta);
			}
			if( j == 1 ){
				if (i == 0) printf( "\nPosiciones");
				int x,y;
				x = flydata->posicion.x;
				y = flydata->posicion.y;
				printf( "\t%d %d",x,y);
			}
			if( j == 2 ){
				if (i == 0) printf( "\nOrientacion");
				printf( "\t%0.1f",flydata->orientacion);
			}
			if( j == 3 ){
				if (i == 0) printf( "\nDirección");
				printf( "\t%0.1f",flydata->direccion);
			}
			if( j == 4 ){
				if (i == 0) printf( "\nEstado\t");
				printf( "\t%d",flydata->Estado);
			}
			if( j == 5 ){
				if (i == 0) printf( "\nNumFrame");
				printf( "\t%d",flydata->num_frame);
			}
			i++;
		}
		i=0;
	}
	if (tam = 0 ) printf(" Lista vacía\n");
	// regresamos lista al punto en que estaba antes de llamar a la funcion de mostrar lista.
	irAlPrincipio(lista);
	for (n = 0; n < pos_act; n++) irAlSiguiente(lista);
}

void liberarListaFlies(tlcde *lista)
{
  // Borrar todos los elementos de la lista
  STFly *flydata = NULL;
  // Comprobar si hay elementos
  if (lista->numeroDeElementos == 0 ) return;
  // borrar: borra siempre el elemento actual

  irAlPrincipio(lista);
  flydata = (STFly *)borrar(lista);
  while (flydata)
  {
    free(flydata); // borrar el área de datos del elemento eliminado
    flydata = NULL;
    flydata = (STFly *)borrar(lista);
  }
}
// si el ultimo parámetro no es nullindica
void enlazarFlies( STFly* flyAnterior, STFly* flyActual, tlcde* ids ){
	// si la actual ya habia sido etiquetada dejamos su etiqueta
	if( flyActual->etiqueta && ids ) dejarId(flyActual,ids);
	flyActual->etiqueta = flyAnterior->etiqueta;
	flyActual->Color = flyAnterior->Color;
	float distancia;
	//Establecemos la dirección y el modulo del vector de desplazamiento
	EUDistance( flyAnterior->posicion,flyActual->posicion, &flyAnterior->direccion, &distancia );
	flyActual->dstTotal = flyAnterior->dstTotal + distancia;
//	SetTita( flyActual, flyAnterior );
}
/// Haya la distancia ecuclidea entre dos puntos. Establece el modulo y y el argumento en grados.
///
void EUDistance( CvPoint posicion1, CvPoint posicion2, float* direccion, float* distancia){


	float b;
	float a;
	b = posicion2.x - posicion1.x;
	a =  posicion2.y - posicion1.y;

	if( ( b < 0)&&(a < 0) )
	{
		*direccion = atan( b / a );
		//resolvemos ambiguedad debida a los signos en la atan
		*direccion = *direccion + PI;
		*direccion = ( (*direccion) *180)/PI; // a grados
	}
	else if( ( b == 0) && ( a == 0) ){
		*direccion = -1;
	}
	else if( (b == 0)&&( a != 0) ){
		if (a < 0) *direccion = 180;
		else *direccion = 0;
	}
	else if( (b != 0)&&( a == 0) ){
		if( b > 0 ) *direccion = 270;
		else *direccion = 90;
	}
	else {
		*direccion = atan( b / a );
		*direccion = ( (*direccion) *180)/PI;
	}

	// calcular distancia para comprobar si hay desplazamiento. si es menor que un umbral
	// consideramos que esta quieto.
	*distancia = sqrt( pow( (posicion2.y-posicion1.y) ,2 )  + pow( (posicion2.x - posicion1.x) ,2 ) );

}

/// resuelve la ambiguedad en la orientaciónpara cada cuadrante
///estableciendo ésta en función de la dirección del desplazamiento
/// en la decisión de si no se modifica el ángulo o bien se suma o resta pi
/// se considera mayores y menores estrictos, de forma que si la dif absoluta
///entre la direccion y la orientación es exactamnte 90, no se modifica la orient
//.Siempre devuelve un ángulo entre 0 y 359º

void SetTita( STFly* flyAnterior,STFly* flyActual,double angle ){

	//flyAnterior->direccion = angle;
	flyActual->direccion = angle;

	if(  flyActual->orientacion >= 0 && flyActual->orientacion < 90 ){
		if(  (flyActual->direccion > (flyActual->orientacion+90) )&&
			 (flyActual->direccion < (flyActual->orientacion+270 ) )  ){
			// no devolvemos un ángulo mayor de 359
			if( flyActual->orientacion >= 180 )  flyActual->orientacion -= 180;
			else flyActual->orientacion += 180;
		}
	}
	if(  flyActual->orientacion >= 90 && flyActual->orientacion < 270 ){
		if(  (flyActual->direccion < (flyActual->orientacion-90) )||
			 (flyActual->direccion > (flyActual->orientacion+90) )  ){
			if( flyActual->orientacion >= 180 )  flyActual->orientacion -= 180;
			else flyActual->orientacion += 180;
		}
	}
	if(  flyActual->orientacion >= 270 && flyActual->orientacion < 360 ){
		if(  (flyActual->direccion > (flyActual->orientacion-90) )&&
			 (flyActual->direccion < (flyActual->orientacion-270 ) )  ){
			// no devolvemos un ángulo mayor de 359
			if( flyActual->orientacion >= 180 )  flyActual->orientacion -= 180;
			else flyActual->orientacion += 180;
		}
	}

	// si la dirección y la orientación difieren en más de 90º o en menos de 270
//	if ( ( abs( flyActual->orientacion - angle ) > 90) || (abs( flyActual->orientacion - angle ) < 270) )  {
//		// establecemos la orientación según la dirección. Si sobrepasa los 360 le restamos 180
//		// si no los sobrepasa se los sumamos. Así el resultado siempre estará entre 0 y 359.
//		if (flyActual->orientacion >= 180) flyActual->orientacion = flyActual->orientacion - 180;
//		else flyActual->orientacion = flyActual->orientacion + 180;
//		//flyActual->orientacion = flyAnterior->orientacion;
//	}

}
///////////////////// Interfaz para gestionar buffer //////////////////////////////


///! Borra y libera el espacio del primer elemento del buffer ( el frame mas antiguo )
///! El puntero actual seguirá apuntando al mismo elemento que apuntaba antes de llamar
///! a la función.

int liberarPrimero(tlcde *FramesBuf ){

	STFrame *frameData = NULL;
	STFrame *frDataTemp = NULL;

//Guardamos la posición actual.
	int i = FramesBuf->posicion;
//

	if( FramesBuf->numeroDeElementos == 0 ) {printf("\nBuffer vacio");return 1;}
	irAl(0, FramesBuf);
	frameData = (STFrame*)obtenerActual( FramesBuf );
	 // por cada nuevo frame se libera el espacio del primer frame
	liberarListaFlies( frameData->Flies );
	free( frameData->Flies);
	//borra el primer elemento
	cvReleaseImage(&frameData->Frame);
	cvReleaseImage(&frameData->BGModel);
	cvReleaseImage(&frameData->FG);
	cvReleaseImage(&frameData->IDesv);
	cvReleaseImage(&frameData->ImMotion);
	cvReleaseImage(&frameData->OldFG);
	frameData = (STFrame *)borrar( FramesBuf );
	if( !frameData ) {
		printf( "Se ha borrado el último elemento.Buffer vacio" );
		frameData = NULL;
		return 0;
	}
	else{
		free( frameData );
		frameData = NULL;
		irAl(i - 1, FramesBuf);
		return 1;
	}
}

void liberarSTFrame( STFrame* frameData ){
	liberarListaFlies( frameData->Flies);
	free( frameData->Flies );
	cvReleaseImage(&frameData->BGModel);
	cvReleaseImage(&frameData->FG);
	cvReleaseImage(&frameData->IDesv);
	cvReleaseImage(&frameData->ImMotion);
	cvReleaseImage(&frameData->OldFG);
    free(frameData); // borrar el área de datos del elemento eliminado
}

void liberarBuffer(tlcde *FramesBuf)
{
  // Borrar todos los elementos del buffer
  STFrame *frameData = NULL;
  if (FramesBuf->numeroDeElementos == 0 ) return;
  // borrar: borra siempre el elemento actual
  irAlPrincipio(FramesBuf);
  frameData = (STFrame *)borrar( FramesBuf );
  while (frameData)
  {
	liberarListaFlies( frameData->Flies);
	free( frameData->Flies );
	cvReleaseImage(&frameData->BGModel);
	cvReleaseImage(&frameData->FG);
	cvReleaseImage(&frameData->IDesv);
	cvReleaseImage(&frameData->ImMotion);
	cvReleaseImage(&frameData->OldFG);
    free(frameData); // borrar el área de datos del elemento eliminado
    frameData = (STFrame *)borrar( FramesBuf );
  }
}

///////////////////////// INTERFACE PARA GESTIONAR IDENTIDADES /////////////////////



void CrearIdentidades(tlcde* Identities){

	Identity* Id;
	RNG rng(0xFFFFFFFF); // para generar un color aleatorio
//	char nombre[10];
//	nombre = "Vidal";
//	nombre = "Pepe";
//	nombre = "Tomas";
//	nombre = "Pablo";
	int i = NUMBER_OF_IDENTITIES-1;
	for(i=NUMBER_OF_IDENTITIES-1; i >= 0 ; i--){
		Id = ( Identity* )malloc( sizeof(Identity ));
		Id->etiqueta = i + 1;
		Id->color = randomColor(rng);
		anyadirAlFinal( Id, Identities);
	}
}

void liberarIdentidades(tlcde* lista){
	  // Borrar todos los elementos de la lista
	Identity* id;
	  // Comprobar si hay elementos
	  if (lista->numeroDeElementos == 0 ) return;
	  // borrar: borra siempre el elemento actual
	  irAlPrincipio( lista );
	  id = (Identity *)borrar(lista);
	  int i = 0;
	  while( id ){
		  free (id);
		  id = NULL;
		  id = (Identity *)borrar(lista);
	  }
}

void asignarNuevaId( STFly* fly, tlcde* identities){
	Identity *id;
	id = (Identity* )borrarEl( identities->numeroDeElementos - 1, identities);
	fly->etiqueta = id->etiqueta;
	fly->Color = id->color;
	free(id);
	id = NULL;
}

void dejarId( STFly* fly, tlcde* identities ){
	Identity *Id;
	Id = ( Identity* )malloc( sizeof(Identity ));
	Id->etiqueta = fly->etiqueta;
	Id->color = fly->Color;
	anyadirAlFinal( Id , identities );
}

void mostrarIds( tlcde* Ids){

	Identity* id;
	irAlFinal(Ids);

	for(int i = 0; i <Ids->numeroDeElementos ; i++ ){
		id = (Identity*)obtener(i, Ids);
		printf("Id = %d\n", id->etiqueta);
	}
}

void reasignarIds(){

}

static Scalar randomColor(RNG& rng)
{
    int icolor = (unsigned)rng;
    return Scalar(icolor&255, (icolor>>8)&255, (icolor>>16)&255);
}

/////////////////////////// GESTION FICHEROS //////////////////////////////

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

//!< El algoritmo mantiene un buffer de 50 frames ( 2 seg aprox ) para los datos y para las imagenes.
//!< Una vez que se han llenado los buffer, se almacena en fichero los datos de la lista
//!< Flies correspondientes al primer frame. La posición actual no se modifica.
//!< Si la lista está vacía mostrará un error. Si se ha guardado con éxito devuelve un uno.

int GuardarPrimero( tlcde* framesBuf , char *nombreFichero){

	tlcde* Flies = NULL;
	FILE * pf;
	STFly* fly = NULL;
	int posicion = framesBuf->posicion;

	if (framesBuf->numeroDeElementos == 0) {printf("\nBuffer vacío\n");return 0;}

	// obtenemos la direccion del primer elemento
	irAlPrincipio( framesBuf );
	STFrame* frameData = (STFrame*)obtenerActual( framesBuf );
	//obtenemos la lista

	Flies = frameData->Flies;
	if (Flies->numeroDeElementos == 0) {printf("\nElemento no guardado.Lista vacía\n");return 1;}
	int i = 0, tam = Flies->numeroDeElementos;
	// Abrir el fichero nombreFichero para añadir "a".
	if ((pf = fopen(nombreFichero, "a")) == NULL)
	{
	printf("El fichero no puede abrirse.");
	exit(1);
	}
	while( i < tam ){
		fly = (STFly*)obtener(i, Flies);
		fwrite(&fly, sizeof(STFly), 1, pf);
		if (ferror(pf))
		{
		  perror("Error durante la escritura");
		  exit(2);
		}
		i++;
	}
	fclose(pf);
	irAl( posicion, framesBuf);
	return 1;
}



