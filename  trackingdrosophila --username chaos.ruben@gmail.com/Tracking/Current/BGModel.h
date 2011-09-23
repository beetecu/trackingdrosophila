/*!
 * BGModel.h
 *
 *  Created on: 19/07/2011
 *      Author: chao
 */

#ifndef BGMODEL_H_
#define BGMODEL_H_

#include "VideoTracker.hpp"


//#define FRAMES_TRAINING 20 // Nº de frames para el aprendizaje del fondo
//#define HIGHT_THRESHOLD 3 // Umbral para la resta de fondo
//#define LOW_THRESHOLD 3
//#define ALPHA 0.5	// Parámetro para actualización dinámica del modelo
//#define CVCLOSE_ITR 1
//#define MAX_CONTOUR_AREA  200
//#define MIN_CONTOUR_AREA  5

typedef struct{
	//Parametros del modelo
	int FRAMES_TRAINING ;
	int HIGHT_THRESHOLD;
	int LOW_THRESHOLD ;
	double ALPHA;
	//Parametros de limpieza de foreground
	bool MORFOLOGIA ; // si esta a 1 se aplica erosión y cierre
	int CVCLOSE_ITR ;
	int MAX_CONTOUR_AREA;
	int MIN_CONTOUR_AREA;
}BGModelParams;

// PROTOTIPOS DE FUNCIONES //

//! \brief Inicializa el modelo de fondo como una Gaussiana con una estimación
//! de la mediana y de la de desviación típica según:
//! Para el aprendizaje del fondo toma un número de frames igual a FRAMES_TRAINING
/*!
	  \param t_capture : Imagen fuente de 8 bit de niveles de gris preprocesada.
	  \param BG : Imagen fuente de 8 bit de niveles de gris. Contiene la estimación de la mediana de cada pixel
	  \param ImMask : Máscara  de 8 bit de niveles de gris para el preprocesdo (extraccion del plato).
	*/
void initBGGModel( CvCapture* t_capture, IplImage* BG,IplImage *DE, IplImage* ImMask,BGModelParams* Param, CvRect ROI);

IplImage* getBinaryImage(IplImage * image);
//! \brief Recibe una imagen en escala de grises preprocesada. En la primera ejecución inicializa el modelo. Estima a la mediana
//!	en BGMod y la varianza en IvarF según:
/*! Mediana:
//!	\f[
//!		mu(p)= median\I_t(p)
//!		\f]
    /*!
      \param ImGray : Imagen fuente de 8 bit niveles de gris.
      \param BGMod : Imagen de fondo sobre la que se estima la mediana
    */

void accumulateBackground( IplImage* ImGray, IplImage* BGMod,IplImage *Ides,CvRect DataROI, IplImage* mask );
//! \brief Recibe una imagen en escala de grises preprocesada. estima a la mediana
//!	en BGMod y la varianza en IvarF según:
/*! Mediana:
//!	\f[
//!		mu(p)= median\I_t(p)
//!		\f]
    /*
      \param tmp_frame : Imagen fuente de 8 bit niveles de gris.
      \param *Cap Puntero a estructura que almacena las distintas capas del modelo de fondo.
      \param DataROI: Contiene los datos para establecer la ROI
      \param Mask: Nos permite actualizar el fondo de forma selectiva.
    */
void UpdateBGModel(IplImage * tmp_frame, IplImage* BGModel,IplImage* DESVI,BGModelParams* Param,  CvRect DataROI, IplImage* Mask = NULL);

//! \brief Crea una mascara binaria (0,255) donde 255 significa primer  plano
/*!
      \param ImGray : Imagen fuente de 8 bit de niveles de gris preprocesada.
      \param Cap : Imagen fuente de 8 bit de niveles de gris. Contiene la estimación de la mediana de cada pixel
      \param fg : Imagen destino ( máscara ) de 8 bit de niveles de gris.

    */

void RunningBGGModel( IplImage* Image, IplImage* median, IplImage* IdesvT,double ALPHA, CvRect dataroi );
//! \brief Crea una mascara binaria (0,255) donde 255 significa primer  plano.
//! Se establece un umbral bajo ( LOW_THRESHOLD ) para los valores de la normal
//! tipificada a partir del cual el píxel es foreground ( 255 )
/*!
      \param ImGray : Imagen fuente de 8 bit de niveles de gris preprocesada.
      \param bg_model : Imagen fuente de 8 bit de niveles de gris. Contiene la estimación de la mediana de cada pixel
      \param fg : Imagen destino ( máscara ) de 8 bit de niveles de gris.
      \param DataROI: Contiene los datos para establecer la ROI
    */
void BackgroundDifference( IplImage* ImGray, IplImage* bg_model,IplImage* Ides,IplImage* fg,BGModelParams* Param, CvRect dataroi);

//! \brief Aplica Componentes conexas para limpieza de la imagen:
//! - Realiza operaciones de morphologia para elimirar ruido.
//! - Establece un área máxima y mínimo para que el contorno no sea borrado
//! - Aplica un umbral alto para los valores de la normal tipificada. Si alguno
//! de los píxeles del blob clasificado como foreground con el LOW_THRESHOLD no
//! desaparece al aplicar el HIGHT_THRESHOLD, es un blob válido.
//! (- Establece el nº máximo de contornos a devolver )
//!
/*!
      \param FG : Imagen fuente de 8 bit de niveles de gris que contine la imagen a limpiar

    */
void FGCleanup( IplImage* FG, IplImage* DES, BGModelParams* Param, CvRect dataroi);

void onTrackbarSlide(int pos, BGModelParams* Param);
void error(int err);
void AllocateImagesBGM( IplImage *I );
void DeallocateImagesBGM();


#endif /* BGMODEL_H_ */
