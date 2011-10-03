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

// Inicialmente implementar solo el aumento y disminucion del umbral sin casos especiales
#include "validacion.hpp"

//void Validacion(IplImage *Imagen,
//		STCapas* Capa,
//		SHModel* SH,
//		STFlies* Flie,
//		CvRect Segroi,
//		BGModelParams* BGParams,
//		ValParams* VParams ){
//
//	double Pxi; // La probabilidad del blob actual
//	bool Exceso;//Flag que indica si la pxi minima no se alcanza x exceso o por defecto.
//	double Circul; // Para almacenar la circularidad del blob actual
//	int N; // Permite establecer la probabilidad minima ( Vparams->PxiMin )
//	bool segmentacion = false;
//
//	// establecemos los parámetros de validación por defecto
//	if ( VParams == NULL ){
//		setValParams( VParams);
//	}
//	// establecemos parámetros para los umbrales en la resta de fondo por defecto
//	if ( BGParams == NULL){
//		setBGModParams( BGParams);
//	}
//// Recorremos los blobs uno por uno y los validamos.
//
//	// bucle for desde el primero hasta el ultimo individuo de la estructura mosca del frame actual
//
//	for( Flie; Flie->siguiente != NULL; Flie = Flie->siguiente ){
//		Pxi = CalcProbMosca( SH , Flie );
//		Circul = CalcCircul( Flie );
//		Exceso = CalcProbUmbral( SH, VParams ); /// calcula VParams->UmbralProb
//		// si no alcanza la P(xi) minima o bien tiene mucha circularidad
//		if( ( Pxi < VParams->UmbralProb) || ( Circul > VParams->UmbralCirc) ){
//			if( Pxi < VParams->UmbralProb ){
//				// comprobamos si no alcanza la probabilidad minima devido
//				// a un area superior o inferior al rango establecido
//				if( Exceso ){
//					VParams->MaxLowTH = ObtenerMaximo(Imagen, Capa ,Flie->Roi );
//					VParams->PxiMin = VParams->UmbralProb / N;
//					while( !segmentacion &&
//							BGParams->LOW_THRESHOLD < VParams->MaxLowTH &&
//							Pxi > VParams->PxiMin){
//						/* Incrementar paulatinamente el umbral de resta de fondo hasta el
//						 * máximo comprobando si hay o no segmentación, si se supera o no
//						 *  el umbral y si la pxi es inferior o superior al PxiMin
//						 */
//						/* El máximo umbral será el pixel del blob con mayor distancia normalizada
//						 * al background. En general dicho punto coincidirá aproximadamente con el
//						 * centro del blob habrá un tamaño minimo ( Pxi minima ) a partir del cual
//						 * deje de reducirse el umbral y un valor maximo para el umbral ( MaxLowTH )
//						 * Se puede ponderar con el valor del pixel del centro.
//						 */
//
//						// Incrementar umbral
//							BGParams->LOW_THRESHOLD += 1;
//						// Resta de fondo
//						BackgroundDifference( Imagen, Capa->BGModel,Capa->IDesv,Capa->FG,BGParams, Flie->Roi);
//						// Segmentar
//						int Nc = segmentacion(Imagen, Capa, Roi, Flie);
//						// comprobar si hay segmentacion
//						if ( Nc > 1 ) segmentacion = true;
//
//						// Calcular Pxi de los blob resultantes
//
//
//					}
//					// Validar las condiciones de salida del bucle
//					/* Si se consige segmentar (en 2 o mas )el blob borrar el blob actual de la estructura mosca,
//					 * sustituirlo por uno de los segmentados y añadir al final el/los nuevo/s blob/s
//					 * hacerlo aqui o modificar la función segmentación para que pueda hacerlo
//					 */
//					// Si hay segmentacion, no se alcanza el umbral y la pxi > pximin para los 2 (o mas) blobs
//
//						//Insertar en la estructura e incrementar el puntero Flie para no volver a analizarlos
//
//					// si no hay segmentacion y se alcanza el umbral
//
//						//aplicar algoritmo EM y añadir nuevos blobs a la estructura
//
//					// si no hay segmentacion y la pxi < pximin ( caso de sombra )
//
//						//decrementar el umbral hasta que se alcance el umbral de prob ( UmbralProb )
//
//					/////// Casos especiales/////// Cuando se cumplen a la vez dos o mas condiciones de salida.
//					// Si hay segmentacion y pxi < pximin, eliminar blob que no cumple la pxi ( puede ser una sombra)
//
//					// Si hay se consigue segmentar se continua aumentando el umbral
//
//
//				}
//				// en caso de no superar el umbral por defecto de area
//				else{
//					/* Disminuir paulatinamente el umbral de resta asta una desviacion tipica
//					 * comprobando en cada iteración si Pxi alcanza el umbral */
//
//
//
//
//					/* Si Pxi no alcanza el umbral comprobar si se trata de una parte de
//					 * la mosca (ala, patas ) que se ha movido. Hacer crecer Pxi mediante
//					 * tecnicas de crecimiento de regiones basadas en el color de la
//					 * imagen original hasta que se alcance la Pxi minima.
//					 */
//
//					/* Si Pxi continua sin alcanzar el nivel, eliminar el blob de la estuctura.
//					 * Esta técnica pemite eliminar los fantasmas del foreground y ruido
//					 */
//				}
//			}
//			// Circularidad elevada
//			else{
//				// Mediante esta técnica de variación dinámica del umbral
//				//se logran eliminar las sombras
//				// Antes de nada comprobar tamaño;
//				// si es muy pequeño ignoramos el que tenga una elevada circularidad
//			}
//		}
//		Flie = Flie->siguiente;
//	}
//}
//void setValParams( ValParams* Parameters){
//	ValParams* Params;
//	Params = ( ValParams *) malloc( sizeof( ValParams) );
//	if( Parameters == NULL )
//	  {
//		Params->UmbralCirc = 0;
//		Params->UmbralProb = 0;
//		Params->MaxDecLTHIters = 20;
//		Params->MaxIncLTHIters= 20;
//		Params->MaxLowTH = 20;
//		Params->PxiMin = 0;
//		Params->MinLowTH = 1;
//	}
//	else
//	{
//		Params = Parameters;
//	}
//}
//void setBGModParams( BGModelParams* Parameters){
//	BGModelParams* Params;
//	 Params = ( BGModelParams *) malloc( sizeof( BGModelParams) );
//	    if( Parameters == NULL )
//	      {
//	    	Params->FRAMES_TRAINING = 0;
//	    	Params->ALPHA = 0 ;
//	    	Params->MORFOLOGIA = 0;
//	    	Params->CVCLOSE_ITR = 0;
//	    	Params->MAX_CONTOUR_AREA = 0 ;
//	    	Params->MIN_CONTOUR_AREA = 0;
//	    	Params->HIGHT_THRESHOLD = 20;
//	    	Params->LOW_THRESHOLD = 10;
//	    }
//	    else
//	    {
//	        Params = Parameters;
//	    }
//}
//double CalcProbMosca( SHModel* SH , STFlies* Flie ){
//	double Pxi;
//	Pxi = exp( -abs(PI*Flie->a*Flie->b - SH->FlyAreaMed) / SH->FlyAreaDes);
//	return Pxi;
//}
//double CalcCircul( STFlies* Flie){
//	double circularidad;
//	circularidad = ( 4*PI*Flie->a*Flie->b*PI)/(Flie->perimetro*Flie->perimetro);
//	return circularidad;
//}
//int CalcProbUmbral( SHModel* SH,ValParams* VParams ){
//
//	float area;
//	double Pth;
//	area = VParams->UmbralProb*SH->FlyAreaDes + SH->FlyAreaMed;
//	Pth = exp( -abs(area - SH->FlyAreaMed) / SH->FlyAreaDes);
//	if ( abs(area - SH->FlyAreaMed)<0 ) return 0;
//	else return 1;
//}
//int ObtenerMaximo(IplImage* Imagen, STCapas* Capa,CvRect Roi ){
//	// obtener matriz de distancias normalizadas al background
//	if (SHOW_VALIDATION_DATA == 1) printf(" \n\n Busqueda del máximo umbral...");
//	IplImage* Idif = 0;
//	CvSize size = cvSize(Imagen->width,Imagen->height); // get current frame size
//	if( !IDif || IDif->width != size.width || IDif->height != size.height ) {
//		        cvReleaseImage( &IDif );
//		        cvReleaseImage( &pesos );
//		        IDif=cvCreateImage(cvSize(Capa->BGModel->width,Capa->BGModel->height), IPL_DEPTH_8U, 1); // imagen diferencia abs(I(pi)-u(p(i))
//		        pesos=cvCreateImage(cvSize(Capa->BGModel->width,Capa->BGModel->height), IPL_DEPTH_8U, 1);//Imagen resultado wi ( pesos)
//		        cvZero( IDif);
//		        cvZero(pesos);
//	}
//	// |I(p)-u(p)|/0(p)
//	cvAbsDiff(Imagen,Capa->BGModel,IDif);
//	cvDiv(IDif,Capa->IDesv,pesos);
//	// Buscar máximo
//	int Maximo = 0;
//	for (int y = Roi.y; y< Roi.y + Roi.height; y++){
//		uchar* ptr1 = (uchar*) ( Capa->FG->imageData + y*Capa->FG->widthStep + 1*Roi.x);
//		uchar* ptr2 = (uchar*) ( pesos->imageData + y*pesos->widthStep + 1*Roi.x);
//		if (SHOW_VALIDATION_DATA == 1) printf(" \n\n");
//		for (int x = 0; x<Roi.width; x++){
//			if (SHOW_VALIDATION_DATA == 1) {
//				if( ( y == Roi.y) && ( x == 0) ){
//					printf("\n Origen: ( %d , %d )\n\n",(x + Roi.x),y);
//				}
//				printf("%d\t", ptr2[x]);
//			}
//			if ( ptr1[x] == 255 ){
//				if (ptr2[x] > Maximo ) Maximo = ptr2[x];
//			}
//		}
//	}
//	printf("\n Maximo: %d ", Maximo);
//}
