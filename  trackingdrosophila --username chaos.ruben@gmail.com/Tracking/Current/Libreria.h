/*
 * Libreria.h
 *
 *  Created on: 09/08/2011
 *      Author: chao
 */

#include "VideoTracker.hpp"
#include "Errores.hpp"

#ifndef LIBRERIA_H_
#define LIBRERIA_H_

/// Medida de tiempos

#ifndef CLK_TCK
#define CLK_TCK CLOCKS_PER_SEC
#endif

#ifndef _TIEMPOS_
#define _TIEMPOS_

int gettimeofday( struct timeval *tv, struct timezone *tz );

#endif //_TIEMPOS_

/////////////// Tratamiento de imagenes /////////////////
#ifndef _IMAGEN_
#define _IMAGEN_
/// Función para obtener el número de frames en algunos S.O Linux
int getAVIFrames( char* );

//! \brief Recibe una imagen RGB 8 bit y devuelve una imagen en escala de grises con un
//!   flitrado gaussiano 5x. Si el último parámetro es verdadero, se binariza. Si
//!   el parámetro ImFMask no es NULL se aplica la máscara.
    /*!
      \param src : Imagen fuente de 8 bit RGB.
      \param dst : Imagen destino de 8 bit de niveles de gris.
      \param ImFMask : Máscara.
      \param bin: : Si está activo se aplica un adaptiveTreshold a la imagen antes de filtrar.
      \return Imagen preprocesada
    */
void ImPreProcess( IplImage* src,IplImage* dst, IplImage* ImFMask,bool bin, CvRect ROI);

void invertirBW( IplImage* Imagen );

#endif // _IMAGEN_


// Interfaz para manipular una lcde //////////////////////////////
//
#ifndef _ELEMENTO_H
#define _ELEMENTO_H

	// Tipo Elemento (un elemento de la lista) ///////////////////////
	typedef struct s
	{
	  void *dato;          // área de datos
	  struct s *anterior;  // puntero al elemento anterior
	  struct s *siguiente; // puntero al elemento siguiente
	} Elemento;


#endif // _ELEMENTO_H


#ifndef _INTERFAZ_LCSE_H
#define _INTERFAZ_LCSE_H

// Parámetros de la lista
typedef struct
{
  Elemento *ultimo;      // apuntará siempre al último elemento
  Elemento *actual;      // apuntará siempre al elemento accedido
  int numeroDeElementos; // número de elementos de la lista
  int posicion;          // índice del elemento apuntado por actual
} tlcde;

#endif //_INTERFAZ_LCSE_H

#ifndef _INTERFAZ_LCSE_
#define _INTERFAZ_LCSE_

// Crear un nuevo elemento
Elemento *nuevoElemento();

// Iniciar una estructura de tipo tlcde
void iniciarLcde(tlcde *lcde);

// Añadir un nuevo elemento a la lista a continuación
// del elemento actual; el nuevo elemento pasa a ser el
// actual
void insertar(void *e, tlcde *lcde);

// La función borrar devuelve los datos del elemento
// apuntado por actual y lo elimina de la lista.
void *borrar(tlcde *lcde);

void *borrarEl(int i,tlcde *lcde);

// Avanzar la posición actual al siguiente elemento.
void irAlSiguiente(tlcde *lcde);

// Retrasar la posición actual al elemento anterior.
void irAlAnterior(tlcde *lcde);

// Hacer que la posición actual sea el principio de la lista.
void irAlPrincipio(tlcde *lcde);

// El final de la lista es ahora la posición actual.
void irAlFinal(tlcde *lcde);

// Posicionarse en el elemento i
int irAl(int i, tlcde *lcde);

// Devolver el puntero a los datos asociados
// con el elemento actual
void *obtenerActual(tlcde *lcde);

// Devolver el puntero a los datos asociados
// con el elemento de índice i
void *obtener(int i, tlcde *lcde);

// Establecer nuevos datos para el elemento actual
void modificar(void *pNuevosDatos, tlcde *lcde);

void anyadirAlFinal(void *e, tlcde *lcde );

//tlcde* fusionarListas( tlcde* lcde1, tlcde* lcde2);

#endif // _INTERFAZ_LCSE_


#ifndef _BUFFER_
#define _BUFFER_

void mostrarListaFlies(int pos,tlcde *lista);

void liberarListaFlies(tlcde *lista);

int liberarPrimero(tlcde *FramesBuf );

void liberarBuffer(tlcde *FramesBuf);

void liberarSTFrame( STFrame* frameData );

#endif //_BUFFER_

#ifndef _FICHEROS_
#define _FIHEROS_

int existe(char *nombreFichero);

void crearFichero(char *nombreFichero );

int GuardarPrimero( tlcde* framesBuf , char *nombreFichero);

#endif //_FICHEROS_
#endif // LIBRERIA_H_
