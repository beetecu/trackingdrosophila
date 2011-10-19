/*!
 * Libreria.cpp
 *
 *  Created on: 12/07/2011
 *      Author: chao
 */

#include "Libreria.h"

void invertirBW( IplImage* Imagen ){
	for( int y=0;  y< Imagen->height ; y++){
			uchar* ptr = (uchar*) ( Imagen->imageData + y*Imagen->widthStep);

			for (int x = 0; x<Imagen->width; x++) {
				if (ptr[x] == 255) ptr[x] = 0;
				else ptr[x] = 255;
			}
	}
}
// Recibe la imagen del video y devuelve la imagen en un canal de niveles
// de gris con el plato extraido
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


///////////////////// Interfaz para manipular una lcde //////////////////////////////
//
//// Mostrar un mensaje de error y abortar el programa
//void error()
//{
//  printf("Insuficiente memoria\n");
//  exit(1);
//}

// Crear un nuevo elemento
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

///! Borra el elemento apuntado de la posición posición i.
///! Devuelve un puntero al área de datos del objeto borrado
///! o NULL si la lista está vacía. Si la posición es el último
///! y el único elemento, se inicia la lista. Si es el último
///! pero no el único, actual apuntará al nuevo último. Si no
///! es el último, acual apuntará al siguiente elemento.
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

///! Borra el elemento apuntado por actual.
///! Devuelve un puntero al área de datos del objeto borrado
///! o NULL si la lista está vacía.Si la posición es el último
///! y el único elemento, se inicia la lista. Si es el último
///! pero no el único, actual apuntará al nuevo último. Si no
///! es el último, acual apuntará al siguiente elemento.
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

