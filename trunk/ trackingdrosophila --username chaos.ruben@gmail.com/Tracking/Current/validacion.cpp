/*
 * validacion.cpp
 *
 *  Created on: 22/09/2011
 *      Author: chao
 *
 *  Esta función se encarga de validar los datos de la estructura rellenada por segmentación
 *
 */
// OJO en limpieza de foreground se hace un filtrado por area
// Creo que hay que añadir en limpieza de fg el uso de roi.
// Ojo, no se puede usar find contours por que cambiaria el orden de los elementos
// opciones: Manejarse con las estructuras o
// almacenar la CvSec de los contornos obtenida en segmentacion
//
#include "validacion.hpp"

void Validacion(IplImage *Imagen, STCapas* Capa, SHModel* SH, STFlies* Flie, CvRect Segroi, BGModelParams* BGParams, ValParams* VParams ){

	// establecemos los parámetros de validación por defecto
	if ( VParams == NULL ){
		setValParams( VParams);
	}
	// establecemos parámetros para los umbrales en la resta de fondo por defecto
	if ( BGParams == NULL){
		setBGModParams( BGParams);
	}
	// Recorremos los blobs uno por uno y los validamos.

//		double area = cvContourArea( c );
//		double perimetro = cvContourPerimeter( c );

		double Pxi;
		double MaxPth; //Si la probabilidad supera este valor es objeto válido
		bool Exceso;//Flag que indica si la pxi minima no se alcanza x exceso o por defecto.
		double Circul;

// añadir bucle for desde el primero hasta el ultimo individuo de la estructura mosca del frame actual

		Pxi = CalcProbMosca( SH , Flie );
		Circul = CalcCircul( Flie );
		Exceso = CalcProbUmbral( SH, VParams );
		// si no alcanza la P(xi) minima o bien tiene mucha circularidad
		if( ( Pxi < VParams->UmbralProb) || ( Circul > VParams->UmbralCirc) ){
			if( Pxi < VParams->UmbralProb ){
				// comprobamos si no alcanza la probabilidad minima debido
				// a un area superior o inferior al rango establecido
				if( Exceso ){

//					ObtenerMaximo( Capa->BGModel,Capa->IDesv,Capa->FG,Flie->Roi );

					/* Incrementar paulatinamente el umbral de resta de fondo hasta el máximo comprobando
					 * si hay segmentación.
					 */
					/* El máximo umbral puede ser el pixel del blob con mayor distancia normalizada
					 * al background. En general dicho punto coincidirá aproximadamente con el centro del blob
					 * Habrá un tamaño minimo ( Pxi minima ) a partir del cual deje de reducirse el umbral.
					 * Se puede ponderar con el valor del pixel del centro
					 *
					 */



					/* Si se consige segmentar (en 2 o mas )el blob borrar el blob actual de la estructura mosca,
					 * sustituirlo por uno de los segmentados y añadir al final el/los nuevo/s blob/s
					 * hacerlo aqui o modificar la funición segmentación para que pueda hacerlo
					 */

					// Si no se consigue segmentar se continua aumentando el umbral

					/* Si se llega al máximo umbral y no hay segmentación aplicar algoritmo EM
					 * y añadir nuevos blobs a la estructura
					 */


				}
				// en caso de no superar el umbral por defecto de area
				else{
					/* Disminuir paulatinamente el umbral de resta asta una desviacion tipica
					 * comprobando en cada iteración si Pxi alcanza el umbral */




					/* Si Pxi no alcanza el umbral comprobar si se trata de una parte de
					 * la mosca (ala, patas ) que se ha movido. Hacer crecer Pxi mediante
					 * tecnicas de crecimiento de regiones basadas en el color de la
					 * imagen original hasta que se alcance la Pxi minima.
					 */

					/* Si Pxi continua sin alcanzar el nivel, eliminar el contorno.
					 * Esta técnica pemite eliminar los fantasmas del foreground y ruido
					 */
				}
			}
			// Circularidad elevada
			else{
				// Mediante esta técnica de variación dinámica del umbral
				//se logran eliminar las sombras
				// Antes de nada comprobar tamaño;
				// si es muy pequeño ignoramos el que tenga una elevada circularidad
			}
		}

}
void setValParams( ValParams* Parameters){
	ValParams* Params;
	Params = ( ValParams *) malloc( sizeof( ValParams) );
	if( Parameters == NULL )
	  {
		Params->UmbralCirc = 0;
		Params->UmbralProb = 0;
		Params->MaxDecLTHIters = 20;
		Params->MaxIncLTHIters= 20;
		Params->MaxLowTH = 20;
		Params->MinLowTH = 1;
	}
	else
	{
		Params = Parameters;
	}
}
void setBGModParams( BGModelParams* Parameters){
	BGModelParams* Params;
	 Params = ( BGModelParams *) malloc( sizeof( BGModelParams) );
	    if( Parameters == NULL )
	      {
	    	Params->FRAMES_TRAINING = 0;
	    	Params->ALPHA = 0 ;
	    	Params->MORFOLOGIA = 0;
	    	Params->CVCLOSE_ITR = 0;
	    	Params->MAX_CONTOUR_AREA = 0 ;
	    	Params->MIN_CONTOUR_AREA = 0;
	    	Params->HIGHT_THRESHOLD = 20;
	    	Params->LOW_THRESHOLD = 10;
	    }
	    else
	    {
	        Params = Parameters;
	    }
}
double CalcProbMosca( SHModel* SH , STFlies* Flie ){
	double Pxi;
	Pxi = exp( -abs(PI*Flie->a*Flie->b - SH->FlyAreaMed) / SH->FlyAreaDes);
	return Pxi;
}
double CalcCircul( STFlies* Flie){
	double circularidad;
	circularidad = ( 4*PI*Flie->a*Flie->b*PI)/(Flie->perimetro*Flie->perimetro);
	return circularidad;
}
int CalcProbUmbral( SHModel* SH,ValParams* VParams ){

	float area;
	double Pth;
	area = VParams->UmbralProb*SH->FlyAreaDes + SH->FlyAreaMed;
	Pth = exp( -abs(area - SH->FlyAreaMed) / SH->FlyAreaDes);
	if ( abs(area - SH->FlyAreaMed)<0 ) return 0;
	else return 1;
}
