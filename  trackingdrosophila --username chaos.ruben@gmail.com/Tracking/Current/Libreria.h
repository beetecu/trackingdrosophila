/*
 * Libreria.h
 *
 *  Created on: 09/08/2011
 *      Author: chao
 */


#include "Interfaz_Lista.h"

#ifndef LIBRERIA_H_
#define LIBRERIA_H_

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
Elemento *nuevoElemento();
void iniciarLista(Lista *lista);
int anyadir(int i, void *e, Lista *lista);
int anyadirAlFinal(void *e, Lista *lista);
void *obtener(int i, Lista *lista);
void *obtenerPrimero( Lista *lista);

#endif /* LIBRERIA_H_ */
