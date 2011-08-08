/*
 * Libreria.cpp
 *
 *  Created on: 12/07/2011
 *      Author: chao
 */
#include <opencv2/imgproc/imgproc_c.h>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <stdio.h>
#include "Interfaz_Lista.h"


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
void PreProcesado( IplImage* frame,IplImage* Im, IplImage* ImFMask,bool bin){

//cvNamedWindow( "Im", CV_WINDOW_AUTOSIZE);
	// Imagen a un canal de niveles de gris
	cvCvtColor( frame, Im, CV_BGR2GRAY);
	if (bin == true){
		cvAdaptiveThreshold( Im, Im,
			255, CV_ADAPTIVE_THRESH_MEAN_C,CV_THRESH_BINARY,75,40);
		//bin = false;
	}
// Filtrado gaussiano 5x
	cvSmooth(Im,Im,CV_GAUSSIAN,5,0,0,0);

// Obtención de Imagen a procesar
	cvCopy( Im, Im );

// Extraccion del plato
	cvAndS(Im, cvRealScalar( 0 ) , Im, ImFMask );

//			cvShowImage( "Im",Im );
//			cvWaitKey(0);
}
//#endif

// Interfaz lista Lista lineal simplemente en lazada

Elemento *nuevoElemento(){

	Elemento *nuevo = (Elemento *)malloc(sizeof(Elemento));

	}

// Inicializar la Lista

void iniciarLista(Lista *lista){

	lista->primero = NULL;
	lista->numeroDeElementos = 0;

	}

// Añadir un elemento a la lista

int anyadir(int i, void *e, Lista *lista){

	int n = 0;
	Elemento *nuevo = NULL, *p = lista->primero;
	Elemento *elemAnterior = p, *elemActual = p;

	// Crear el elemento a añadir

	nuevo = nuevoElemento();
	if (!nuevo){
		error(4);
		return 0;
	}
	nuevo->dato = e; // asignar el puntero que referencia los datos
	nuevo->siguiente = NULL;

	// Si la lista apuntada por p está vacía, añadirlo sin más

	if (lista->numeroDeElementos == 0)
	  {
	    // Añadir el primer elemento

		lista->primero = nuevo;
	    lista->numeroDeElementos++;
	    return 1;
	  }

	// Si la lista no está vacía, encontrar el punto de inserción.
	// Posicionarse en el elemento i

	for (n = 0; n < i; n++)
	  {
	    elemAnterior = elemActual;
	    elemActual = elemActual->siguiente;
	  }

	 nuevo->siguiente = elemActual;
	 elemAnterior->siguiente = nuevo;

	 lista->primero = p;
	 lista->numeroDeElementos++;
	 return 1;
	 }

// Añadir un elemento al final

int anyadirAlFinal(void *e, Lista *lista)
{
  return anyadir(lista->numeroDeElementos, e, lista);
}

void *obtener(int i, Lista *lista)
{
	int n = 0;
	Elemento *nuevo = lista->primero; // apunta al primer elemento

  // Obtener el elemento de la posición i

	if (i >= lista->numeroDeElementos || i < 0)
    return NULL; // índice fuera de límites


  // Posicionarse en el elemento i

	for (n = 0; n < i; n++)
    nuevo = nuevo->siguiente;

  // Retornar los datos

	return nuevo->dato;

}
void *obtenerPrimero( Lista *lista){
	return obtener( 0, lista);
}
