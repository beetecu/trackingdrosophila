/*
 * cabecera.h
 *
 *  Created on: 19/07/2011
 *      Author: german
 */


#include <opencv2/imgproc/imgproc_c.h>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <stdio.h>





#ifndef INTERFAZ_LISTA_H_
#define INTERFAZ_LISTA_H_


typedef struct s
{
		void *dato;

		struct s *siguiente; // puntero al siguiente elemento

} Elemento;

typedef struct
{
  Elemento *primero;       // apuntará siempre al primer elemento ( NULL si lista vacia)
  int numeroDeElementos; // número de elementos de la lista

} Lista;

// Crear un nuevo elemento

Elemento *nuevoElemento();

// Inicializar la Lista

void iniciarLista(Lista *lista);

// Añadir un elemento a la lista

int anyadir(int i, void *e, Lista *lista);

// Añadir un elemento al final

int anyadirAlFinal(void *e, Lista *lista);

void *obtener(int i, Lista *lista);

void *obtenerPrimero( Lista *lista);

void error(int);

#endif
