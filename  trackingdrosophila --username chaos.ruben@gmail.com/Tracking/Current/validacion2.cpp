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

///Parámetros Validación para procesado
ValParams* valParams = NULL;
BGModelParams* ValBGParams = NULL;

////////////////// VALIDACION /////////////////

void Validacion2(IplImage *Imagen, STFrame* FrameData, SHModel* SH,
		IplImage* Mask) {

	//Inicializar estructura para almacenar los datos cada mosca
	STFly *FlyData = NULL;


	int Exceso;//Flag que indica si la Pxi minima no se alcanza por exceso o por defecto.
	double Circul; // Para almacenar la circularidad del blob actual

	double Besthres = 0;// el mejor de los umbrales perteneciente a la mejor probabilidad
	double BestPxi = 0; // la mejor probabilidad obtenida para cada mosca

	// Establecemos los parámetros de validación por defecto

	// establecer los umbrales de resta iniciales

	if (valParams->privateParams == NULL) {
		PrivateValParams(SH, &valParams->privateParams);
	}

	// Establecemos parámetros para los umbrales en la resta de fondo por defecto
	ValBGParams->LOW_THRESHOLD = valParams->privateParams->LThInicio;
	ValBGParams->HIGHT_THRESHOLD = valParams->privateParams->HThInicio;
	if( SH->FlyAreaDes == 0){
		printf("hola");
	}

	// almacenar una copia del FG para restaurar el blob en caso de que falle la validación
	cvCopy(FrameData->FG, Mask);

	// Recorremos los blobs uno por uno y los validamos.
	// Bucle for desde el primero hasta el ultimo individuo de la Lista FrameData->Flie del frame actual.
	// Los blobs resultantes de la fisión se añaden al final para ser validados de nuevo
	if (SHOW_VALIDATION_DATA)
		mostrarFliesFrame(FrameData);
	for (int i = 0; i < FrameData->Flies->numeroDeElementos; i++) {

		// Almacenar la Roi del blob visitado antes de ser validado


		FlyData = (STFly *) obtener(i, FrameData->Flies);
		if (FlyData->flag_seg == true)
			continue; // si el blob ha sido validado y es correcto pasar al siguiente

		CvRect FlyDataRoi = FlyData->Roi;
		// Inicializar LOW_THRESHOLD y demas valores
		ValBGParams->LOW_THRESHOLD = valParams->privateParams->LThInicio;
		ValBGParams->HIGHT_THRESHOLD = valParams->privateParams->HThInicio;
		FlyData->bestTresh = ValBGParams->LOW_THRESHOLD;

		// calcular su circularidad
		Circul = CalcCircul(FlyData);

		// Comprobar si existe Exceso o Defecto de area, en caso de Exceso la probabilidad del blob es muy pequeña
		// y el blob puede corresponder a varios blobs que forman parte de una misma moscas o puede corresponder a
		// un espurio.
		Exceso = CalcProbFly(SH, FlyData, valParams); // Calcular la px de la mosca.
		BestPxi = FlyData->Px;
		STFly* MejorFly = FlyData;// = NULL;
		// comprobar si está dentro de los límites:

		//	Params->MaxLowTH = *ObtenerMaximo(Imagen, FrameData ,FlyData->Roi);

		// En caso de EXCESO de Area
		/* Incrementar paulatinamente el umbral de resta de fondo hasta
		 * que haya fisión o bien desaparzca el blob sin fisionarse o bien
		 * no se supere la pximin.
		 * - Si hay fisión:
		 * 		-si los blobs tienen una probabilidad mayor que la pximin fin segmentacion
		 * 		-si alguno de ellos tiene una probabilidad menor a la pximin, restaurar original y aplicar EM
		 * - Si no hay fisión: Si el blob desaparece antes de alcanzar la pximin o
		 *  bien alcanza la pximin, restablecer el tamaño original del blob para aplicar EM
		 *
		 */
		if (Exceso > 0) {
			//Inicializar estructura para almacenar la lista de los blobs divididos.
			tlcde* TempSeg = NULL;

			ValBGParams->LOW_THRESHOLD = valParams->privateParams->LThInicio;
			ValBGParams->HIGHT_THRESHOLD = valParams->privateParams->HThInicio;
			// sustituir por un for con el número máximo de iteraciones.
			int j = 0;
			while (1) {
				// primero probar con el umbral anterior. si el blob desaparece o no da un resutado satisfactorio
				// proceder desde el umbral predetermindado

				// Incrementar umbral
				ValBGParams->LOW_THRESHOLD += 1;
				j++;
				// Restar fondo
				BackgroundDifference(Imagen, FrameData->BGModel, NULL,
						FrameData->FG, ValBGParams, FlyDataRoi);
				//verMatrizIm(FrameData->FG, FlyDataRoi);
				// Segmentar
				TempSeg = segmentacion2(Imagen, FrameData->BGModel,
						FrameData->FG, FlyDataRoi, NULL);

				DraWWindow(FrameData->FG, NULL, NULL, SHOW_VALIDATION_IMAGES, COMPLETO );

				// Comprobar si se ha conseguido dividir el blob y verificar si el resultado es optimo

				// 1) El blob desaparece antes de alcanzar la pximin sin dividirse. Restauramos el blob y pasamos al siguiente elemento.
				if (TempSeg->numeroDeElementos < 1 || TempSeg == NULL) {
					RestaurarElMejor(FrameData->Flies, FrameData->FG, MejorFly,
							FlyDataRoi, i);
					DraWWindow(FrameData->FG, NULL, NULL, SHOW_VALIDATION_IMAGES,
							COMPLETO );
					// liberar lista
					liberarListaFlies(TempSeg);
					break;
				}

				// 2) mientras no haya fisión
				else if (TempSeg->numeroDeElementos == 1) {

					FlyData = (STFly *) obtener(0, TempSeg);
					Exceso = CalcProbFly(SH, FlyData, valParams);
					// no se rebasa la pximin.Almacenar la probabilidad mas alta y continuar
					if (Exceso > -1 && j < valParams->MaxIncLTHIters
							&& ValBGParams->LOW_THRESHOLD < valParams->MaxLowTH) {
						if (FlyData->Px >= BestPxi) {
							MejorFly = FlyData;
							BestPxi = FlyData->Px;
							Besthres = ValBGParams->LOW_THRESHOLD;
						}
						liberarListaFlies(TempSeg);
						free(TempSeg);
						TempSeg = NULL;

						continue;
					}
					// se ha rebasado la pxmin o bien se ha alcanzado el máximo de iteraciones o bien se ha llegado
					// al máximo valor permitido. Restaurar y pasar al siguiente
					else {
						RestaurarElMejor(FrameData->Flies, FrameData->FG,
								MejorFly, FlyDataRoi, i);
						//Restaurar(FrameData->Flies, FrameData->FG, Mask, FlyDataRoi, i);
						liberarListaFlies(TempSeg);
						DraWWindow(FrameData->FG, NULL, NULL, SHOW_VALIDATION_IMAGES,
								COMPLETO );

						break;
					}
				}
				// 3) Hay fisión
				else if (TempSeg->numeroDeElementos > 1) {
					// comprobar si las pxi son válidas. En caso de que todas lo sean eliminar de la lista el blob segmentado
					// y añadirlas a la lista al final.
					int exito = comprobarPxi(TempSeg, SH, valParams);
					if (exito) {
						borrarEl(i, FrameData->Flies); // se borra el elemento dividido
						for (int j = 0; j < TempSeg->numeroDeElementos; j++) { // se añaden los nuevos al final
							FlyData = (STFly *) obtener(j, TempSeg);
							anyadirAlFinal(FlyData, FrameData->Flies);
						}

						if (SHOW_VALIDATION_DATA)
							printf("\n Lista Flies tras FISION\n");
							mostrarFliesFrame(FrameData);
						i--; // decrementar i para no saltarnos un elemento de la lista

						break; // salir del while
					}
					// Se ha rebasado la pximin en alguno de los blobs resultantes. Restauramos y pasamos al sig. blob
					else {
						liberarListaFlies(TempSeg);
						Restaurar(FrameData->Flies, FrameData->FG, Mask,
								FlyDataRoi, i);

						break; // salir del while
					}
				}
			}// Fin del While
			//liberarListaFlies( TempSeg );
			if (TempSeg)
				free(TempSeg);
			TempSeg = NULL;
		} //Fin Exceso

		/* En el caso de DEFECTO disminuir el umbral, para maximizar la probabilidad del blob
		 * Si no es posible se elimina (spurio)
		 */

		else if (Exceso < 0) {

			STFly* MejorFly = NULL;
			CvRect newRoi;
			tlcde* TempSeg = NULL;

			ValBGParams->LOW_THRESHOLD = valParams->privateParams->LThInicio;
			ValBGParams->HIGHT_THRESHOLD = valParams->privateParams->HThInicio;

			newRoi = FlyDataRoi;
			BestPxi = 0;
			int j = 0;
			while (j < valParams->MaxDecLTHIters) {
				newRoi.height += 1;
				newRoi.width += 1;
				newRoi.x -= 1;
				newRoi.y -= 1;
				ValBGParams->LOW_THRESHOLD -= 1;
				if (ValBGParams->LOW_THRESHOLD == valParams->MinLowTH) {
					Restaurar(FrameData->Flies, FrameData->FG, Mask,
							FlyDataRoi, i);
					break;
				}

				//while(Exceso < 0 && ValBGParams->LOW_THRESHOLD > Params->MinLowTH && Params->MaxDecLTHIters){

				// Restar fondo
				BackgroundDifference(Imagen, FrameData->BGModel,
						FrameData->IDesvf, FrameData->FG, ValBGParams, newRoi);

				// Segmentar
				TempSeg = segmentacion2(Imagen, FrameData->BGModel,
						FrameData->FG, newRoi, NULL);
				//TempSeg = segmentacion(Imagen, FrameData, newRoi,NULL);
				DraWWindow(FrameData->FG, NULL, NULL, SHOW_VALIDATION_IMAGES, COMPLETO );

				if (TempSeg == NULL || TempSeg->numeroDeElementos < 1)
					continue;
				// Calcular la pxi
				FlyData = (STFly *) obtener(0, TempSeg);
				Exceso = CalcProbFly(SH, FlyData, valParams); // Calcular la px de la mosca.

				// mientras la probabilidad se incremente continuamos decrementando el umbral
				j++;
				if (FlyData->Px >= BestPxi) {
					MejorFly = (STFly *) obtener(0, TempSeg);
					BestPxi = FlyData->Px;
					Besthres = ValBGParams->LOW_THRESHOLD;
				} else if (FlyData->Px < BestPxi || j
						== valParams->MaxDecLTHIters) {
					// cuando deje de mejorar o bien se llegue al máximo de iteraciones
					//nos quedamos con aquel que dio la mejor probabilidad. Lo insertamos en la lista en la misma posición.
					DraWWindow(FrameData->FG, NULL, NULL, SHOW_VALIDATION_IMAGES,
							COMPLETO );
					sustituirEl(MejorFly, FrameData->Flies, i);
					break;
				}
				liberarListaFlies(TempSeg);
				if (TempSeg) {
					free(TempSeg);
					TempSeg = NULL;
				}

			}// FIN WHILE
			liberarListaFlies(TempSeg);
			if (TempSeg)
				free(TempSeg);

		}// FIN DEFECTO

	}//FOR

}//Fin de Validación

//tlcde* ValidarBLOB(tlcde* Flies, int posBlob, int numFlies, STFrame* FrameData,
//		SHModel* SH, ValParams* Params) {
//
//	//Inicializar estructura para almacenar los datos cada mosca
//	STFly *FlyData = NULL;
//	BGModelParams* ValBGParams = NULL;
//
//	int Exceso;//Flag que indica si la Pxi minima no se alcanza por exceso o por defecto.
//	double Circul; // Para almacenar la circularidad del blob actual
//
//	double Besthres = 0;// el mejor de los umbrales perteneciente a la mejor probabilidad
//	double BestPxi = 0; // la mejor probabilidad obtenida para cada mosca
//
//	// Establecemos los parámetros de validación por defecto
//	if (Params == NULL) {
//		iniciarValParams(&Params, SH);
//	}
//
//	// Establecemos parámetros para los umbrales en la resta de fondo por defecto
//	if (ValBGParams == NULL) {
//		iniciarBGModParams(&ValBGParams);
//	}
//
//	FlyData = (STFly*) obtener(posBlob, Flies);
//	CvRect FlyDataRoi = FlyData->Roi;
//	// Inicializar LOW_THRESHOLD y demas valores
//	setBGModParams(ValBGParams);
//	FlyData->bestTresh = ValBGParams->LOW_THRESHOLD;
//
//	// calcular su circularidad
//	Circul = CalcCircul(FlyData);
//
//	// Comprobar si existe Exceso o Defecto de area, en caso de Exceso la probabilidad del blob es muy pequeña
//	// y el blob puede corresponder a varios blobs que forman parte de una misma moscas o puede corresponder a
//	// un espurio.
//	Exceso = CalcProbFly(SH, FlyData, Params); // Calcular la px de la mosca.
//	BestPxi = FlyData->Px;
//	STFly* MejorFly = FlyData;// = NULL;

	/* Incrementar paulatinamente el umbral de resta de fondo hasta
	 * que haya fisión o bien desaparzca el blob sin fisionarse o bien
	 * no se supere la pximin.
	 * - Si hay fisión:
	 * 		-si los blobs tienen una probabilidad mayor que la pximin fin segmentacion
	 * 		-si alguno de ellos tiene una probabilidad menor a la pximin, restaurar original y aplicar EM
	 * - Si no hay fisión: Si el blob desaparece antes de alcanzar la pximin o
	 *  bien alcanza la pximin, restablecer el tamaño original del blob para aplicar EM
	 *
	 */
	//Inicializar estructura para almacenar la lista de los blobs divididos.
//	tlcde* TempSeg = NULL;
//
//	setBGModParams(ValBGParams);
	// sustituir por un for con el número máximo de iteraciones.
	//	while( !FlyData->flag_seg && !FlyData->failSeg ){
	//		// primero probar con el umbral anterior. si el blob desaparece o no da un resutado satisfactorio
	//		// proceder desde el umbral predeterminadado
	//
	//		// Incrementar umbral
	//		ValBGParams->LOW_THRESHOLD += 1;
	//
	//		// Restar fondo
	//		BackgroundDifference( FrameData->Frame, FrameData->BGModel, NULL ,FrameData->FG,ValBGParams, FlyDataRoi);
	//		//verMatrizIm(FrameData->FG, FlyDataRoi);
	//		// Segmentar
	//		TempSeg = segmentacion2(FrameData->Frame, FrameData->BGModel,FrameData->FG, FlyDataRoi, NULL);
	//
	//		if( SHOW_VALIDATION_DATA){
	//				cvShowImage( "Foreground",FrameData->FG);
	//				cvWaitKey(0);
	//		}
	//		// Comprobar si se ha conseguido dividir el blob y verificar si el resultado es optimo
	//
	//		// 1) No hay fisión. El blob desaparece antes de alcanzar la pximin. Restaurar blob y liberar lista.
	//		if(TempSeg->numeroDeElementos < 1 || TempSeg == NULL){
	//			dibujarBlob( FlyData, FrameData->FG );
	//
	//			if( SHOW_VALIDATION_DATA){
	//					cvShowImage( "Foreground",FrameData->FG);
	//					//cvWaitKey(0);
	//			}
	//			// liberar lista
	//			liberarListaFlies( TempSeg );
	//			if (TempSeg) free(TempSeg);
	//			TempSeg = NULL;
	//			// finalizamos.
	//			FlyData->failSeg = true;
	//		}
	//
	//		// 2) No hay fisión: Almacenar la probabilidad mas alta, así como el umbral correspondiente a esa probabilidad
	//		// y continuar incrementando el umbral en caso de no rebasar la pximin.
	//		else if(  TempSeg->numeroDeElementos == 1 ) {
	//
	//			FlyData=(STFly *)obtener(0, TempSeg);
	//			Exceso = CalcProbFly( SH , FlyData, Params);
	//			// no se rebasa la pximin continuar
	//			if (Exceso > -1 ){
	//				if(FlyData->Px > BestPxi ){
	//					MejorFly = (STFly*)borrarEl(0);
	//					BestPxi=MejorFly->Px;
	//					Besthres = ValBGParams->LOW_THRESHOLD;
	//				}
	//				free(TempSeg);
	//				TempSeg = NULL;
	//
	//			}
	//			// se ha rebasado la pxmin. Restaurar
	//			else {
	//				RestaurarElMejor( FrameData->Flies,  FrameData->FG, MejorFly,  FlyDataRoi, i );
	//				//Restaurar(FrameData->Flies, FrameData->FG, Mask, FlyDataRoi, i);
	//				liberarListaFlies( TempSeg );
	//				if( SHOW_VALIDATION_DATA){
	//						cvShowImage( "Foreground",FrameData->FG);
	//						cvWaitKey(0);
	//				}
	//
	//			}
	//		}
	//		// 3) Hay fisión
	//		else if ( TempSeg->numeroDeElementos > 1 ){
	//			// comprobar si las pxi son válidas. En caso de que todas lo sean eliminar de la lista el blob segmentado
	//			// y añadirlas a la lista al final.
	//			int exito = comprobarPxi( TempSeg, SH, Params);
	//			if (exito){
	//				borrarEl(i,FrameData->Flies); // se borra el elemento dividido
	//				for(int j = 0; j < TempSeg->numeroDeElementos; j++){ // se añaden los nuevos al final
	//					FlyData=(STFly *)obtener(j, TempSeg);
	//					anyadirAlFinal(FlyData, FrameData->Flies );
	//				}
	//				printf("\n Lista Flies tras FISION\n");
	//				if(SHOW_VALIDATION_DATA) mostrarFliesFrame( FrameData );
	//				i--; // decrementar i para no saltarnos un elemento de la lista
	//			}
	//			// Se ha rebasado la pximin en alguno de los blobs resultantes. Restauramos
	//			else{
	//				liberarListaFlies( TempSeg );
	//				Restaurar(FrameData->Flies, FrameData->FG, Mask, FlyDataRoi, i);
	//			}
	//		}
	//		free( TempSeg );
	//		TempSeg = NULL;
	//	}// Fin del While

//	if (TempSeg)
//		free(TempSeg);
//	TempSeg = NULL;
//
////	if (TempSeg)
//		free(TempSeg);
//	TempSeg = NULL;
//
//	return Flies;
//	return Flies;
//
//}//Fin de Validación


//Inicializar los parametros para el modelado de fondo


//Probabilidad de todas las moscas

double CalcProbTotal(tlcde* Lista, SHModel* SH, ValParams* Params,
		STFly* FlyData) {

	double PX = 1;
	double Pxi = 0;

	for (int i = 0; i < Lista->numeroDeElementos; i++) {

		FlyData = (STFly *) obtener(i, Lista);
		CalcProbFly(SH, FlyData, Params);
		PX = PX * Pxi;
	}

	return PX;
}

// Probabilidad de cada mosca
/*!\brief Calcula la pxi del blob y determina si ésta es correcta o no. En caso
 * de no serlo establece si es por exceso o por defecto de area
 * \f[
 * px(i) = exp( -| area(i) - AreaMedia | / AreaDes );
 * si area(i) > AreaMedia y px(i) < PxiMax Exceso = 1
 * si  area(i) < AreaMedia y px(i) > PxiMax Exceso = -1
 * sino Exceso = 0
 * ]\f
 *
 * @param SH Modelo de forma. Contiene el tamaño normal de los blobs ( media y desviación)
 * @param FlyData Blob que queremos comparar con el modelo
 * @param Params Parámetros de validación. Se usanla Pximin y Pximax. Éstas dependen del modelo de forma
 * @return 1 si exceso, -1 si defecto, 0 si dentro de límites.
 */
int CalcProbFly(SHModel* SH, STFly* FlyData, ValParams* Params) {

	int Exceso = 0;

	FlyData->Px = exp(-abs(FlyData->areaElipse - SH->FlyAreaMedia)
			/ SH->FlyAreaDes);
	//Si el area del blob es menor que la media, el error es por defecto,
	//si es mayor que la media el error será por exceso.
	if ((FlyData->areaElipse > SH->FlyAreaMedia) && (FlyData->Px
			< valParams->privateParams->PxiMax))
		Exceso = 1;
	else if ((FlyData->areaElipse < SH->FlyAreaMedia) && (FlyData->Px
			< valParams->privateParams->PxiMin))
		Exceso = -1;
	else
		Exceso = 0;
	// Comprobamos si está dentro de los límites
	return Exceso;

}

// Circularidad de cada mosca

double CalcCircul(STFly* FlyData) {
	double circularidad;
	circularidad = (4 * CV_PI * FlyData->a * FlyData->b * CV_PI)
			/ (FlyData->perimetro * FlyData->perimetro);
	return circularidad;
}

// Probabilidades y  Umbrales
/*!\brief Devuelve la probabilidad que se corresponde a la mínima área permitida
 * Ésta puede ser establecida en base a  la media menos UmbralL desviaciones estándar o
 * bien a un valor concreto de área.
 *
 * si areaMin == NULL  area_L = SH->FlyAreaMed - Umbral_L*SH->FlyAreaDes;
 * si no area_L = areaMin
 * Pth_L = exp( -abs(area_L - SH->FlyAreaMed) / SH->FlyAreaDes);
 *
 *
 * @param SH Modelo de forma( tamaño medio del blob y su desviación.
 * @param Umbral_L desviaciones estándar que se puede alejar el areaL de la media
 * @param areaMin Si NULL se establece la probabilidad en base a las desv estándar de la media
 * @return Pth_L Probabilidad asociada al area minima permitida
 */
float CalcPxMin(SHModel* SH, float Umbral_L, int areaMin) {

	float area_L; // area minima permitida. establece la probabilidad del umbral bajo.
	double Pth_L; // Probabilidad correspondiente a dicha area.

	if (!areaMin)
		area_L = SH->FlyAreaMedia - Umbral_L * SH->FlyAreaDes;
	else
		area_L = areaMin;

	Pth_L = exp(-abs(area_L - SH->FlyAreaMedia) / SH->FlyAreaDes);

	return Pth_L;

}

/*!\brief Devuelve la probabilidad que se corresponde a la máxima área permitida
 * Ésta puede ser establecida en base a  la media mas UmbralH desviaciones estándar o
 * bien a un valor concreto de área.
 *
 * si areaMax == NULL  area_H = SH->FlyAreaMed + Umbral_H*SH->FlyAreaDes;
 * si no area_H = areaMax
 * Pth_H = exp( -abs(area_H - SH->FlyAreaMed) / SH->FlyAreaDes);
 *
 *
 * @param SH Modelo de forma( tamaño medio del blob y su desviación.
 * @param Umbral_H desviaciones estándar que se puede alejar el areaH de la media
 * @param areaMax Si NULL se establece la probabilidad en base a las desv estándar de la media
 * @return Pth_H Probabilidad asociada al area minima permitida
 */
float CalcPxMax(SHModel* SH, float Umbral_H, int areaMax) {

	float area_H; // area que establece el umbral alto.
	double Pth_H; // Probabilidad correspondiente al umbral alto

	if (!areaMax)
		area_H = SH->FlyAreaMedia + Umbral_H * SH->FlyAreaDes;
	else
		area_H = areaMax;

	Pth_H = exp(-abs(area_H - SH->FlyAreaMedia) / SH->FlyAreaDes);
	return Pth_H;

}

// Obtener la maxima distancia de los pixeles al fondo

double* ObtenerMaximo(IplImage* Imagen, STFrame* FrameData, CvRect Roi) {
	// obtener matriz de distancias normalizadas al background
	if (SHOW_VALIDATION_DATA == 1)
		printf(" \n\n Busqueda del máximo umbral...");
	IplImage* IDif = 0;
	IplImage* peso = 0;
	CvSize size = cvSize(Imagen->width, Imagen->height); // get current frame size
	if (!IDif || IDif->width != size.width || IDif->height != size.height) {
		cvReleaseImage(&IDif);
		cvReleaseImage(&peso);
		IDif = cvCreateImage(cvSize(FrameData->BGModel->width,
				FrameData->BGModel->height), IPL_DEPTH_8U, 1); // imagen diferencia abs(I(pi)-u(p(i))
		peso = cvCreateImage(cvSize(FrameData->BGModel->width,
				FrameData->BGModel->height), IPL_DEPTH_32F, 1);//Imagen resultado wi ( pesos)
		cvZero(IDif);
		cvZero(peso);
	}

	// |I(p)-u(p)|/0(p)
	cvAbsDiff(Imagen, FrameData->BGModel, IDif);
	cvDiv(IDif, FrameData->IDesvf, peso);

	// Buscar máximo
	double* Maximo = 0;
	cvMinMaxLoc(peso, Maximo, 0, 0, 0, FrameData->FG);

	return Maximo;
}
/*!\brief Calcula la Pxi de cada una de las flies resultado de la segmentación. En función de ésta
 * decide sobre si se a finalizado, se segmentará de nuevo alguno de los blobs resultantes por ser
 * aun muy grandeo bien se descarta la segmentación por dar algún blob un área demasiado pequeña,
 * restaurando así el original.
 *
 * Si el blob es válido ( sin exceso ni defecto) se establece flag_seg true ( segmentado correctamente ). Si no
 * es válido por exceso se establece flag_seg a false y se devuelve 1 (exito): el blob con exceso será
 * segmentado de nuevo.
 * si no es válida por defecto, se establece flag_seg a false pero en este caso se devuelve un 0 (fallo). Se
 * restaura el blob original.
 *
 * si Exeso = 0  => segmentación correcta.
 * si Exceso = 1 => segmentación correcta pero aun hay exceso
 * si Exceso = -1 => fallo en segmentación. Uno de los blobs es demasiado pequeño.
 * @param TempSeg
 * @param SH
 * @param Params
 * @return 1 si éxito, 0 si fallo.
 */
int comprobarPxi(tlcde* TempSeg, SHModel* SH, ValParams* Params) {
	STFly* FlyData;
	int Exceso;
	for (int j = 0; j < TempSeg->numeroDeElementos; j++) {
		// Calcular Pxi del/los blob/s resultante/s
		FlyData = (STFly *) obtener(j, TempSeg);
		Exceso = CalcProbFly(SH, FlyData, Params);
		if (Exceso == 0)
			FlyData->flag_seg = true;
		else if (Exceso == 1)
			FlyData->flag_seg = false;
		else if (Exceso == -1)
			return 0; // algún elemento no es correca
	}
	return 1; // correcta. aquellos con flyseg = false serán segmentados de nuevo
}

void Restaurar(tlcde* lista, IplImage* Fg, IplImage* Origin, CvRect Roi,
		int elemento) {
	STFly* FlyData;
	FlyData = (STFly *) obtener(elemento, lista);
	FlyData->failSeg = true;
	// restaurar
	cvSetImageROI(Origin, Roi);
	cvSetImageROI(Fg, Roi);
	cvCopy(Origin, Fg);
	cvResetImageROI(Origin);
	cvResetImageROI(Fg);
}

void RestaurarElMejor(tlcde* lista, IplImage* Fg, STFly* MejorFly, CvRect Roi,
		int elemento) {

	// sustituir el blob de la lista por el mejor
	MejorFly->failSeg = true;
	sustituirEl(MejorFly, lista, elemento);
	// restaurar el Fg
	dibujarBlob(MejorFly, Fg);
}

void SetValidationParams() {

	//init parameters
	config_t cfg;
	config_setting_t *setting;
	char settingName[50];
	char configFile[30];
	char settingFather[50];

	int EXITO;
	int DEFAULT = false;

	// Reservar memoria
	// inicializar parámetros de modelo de fondo y de validación

	ValBGParams = (BGModelParams *) malloc(sizeof(BGModelParams));
	if (!ValBGParams) {
		error(4);
		exit(1);
	}

	valParams = (ValParams*) malloc(sizeof(ValParams));
	if (!valParams) {
		error(4);
		exit(1);
	}

	fprintf(stderr, "\nCargando parámetros de Validación:");
	fprintf(stderr,
			"\nCargando parámetros de umbralización y limpieza de primer plano...");

	config_init(&cfg);

	sprintf(configFile, "config.cfg");

	/* Leer archivo. si hay un error, informar y cargar configuración por defecto */
	if (!config_read_file(&cfg, configFile)) {
		fprintf(stderr, "%s:%d - %s\n", config_error_file(&cfg),
				config_error_line(&cfg), config_error_text(&cfg));

		fprintf(stderr, "Error al acceder al fichero de configuración %s .\n"
			" Estableciendo valores por defecto.\n", configFile);
		DefaultValParams( );

	} else {
		sprintf(settingFather, "Procesado");
		setting = config_lookup(&cfg, settingFather);
		/* Si no se se encuentra la setting o bien existe la variable hijo Auto y ésta es true, se establecen TODOS los valores por defecto.*/
		if (!setting) {
			fprintf(stderr, "Error.No se ha podido leer el campo %s.\n"
				" Estableciendo valores por defecto.\n", settingFather);
			DEFAULT = true;
		} else {
			sprintf(settingName, "Auto");
			/* Obtener el valor */
			EXITO = config_setting_lookup_bool(setting, settingName, &DEFAULT);
			if (!EXITO)
				DEFAULT = true;
			else if (EXITO && DEFAULT) {
				fprintf(stderr, "\n Opción Auto activada para el campo %s.\n"
					" Estableciendo valores por defecto.\n", settingFather);
			} else if (EXITO && !DEFAULT)
				fprintf(
						stderr,
						"\n Opción Auto desactivada para el campo %s.\n"
							" Estableciendo valores del fichero de configuración.\n",
						settingFather);
		}
		if (DEFAULT) {
			DefaultValParams();
		}
		// si el modo auto del padre no está activado comprobar si lo está el del hijo.
		else {

			// hijo 1
			sprintf(settingFather, "Procesado.Validacion");
			setting = config_lookup(&cfg, settingFather);
			/* Si no se se encuentra la setting o bien existe la variable hijo Auto y ésta es true, se establecen TODOS los valores por defecto.*/
			if (setting != NULL) {
				sprintf(settingName, "Auto");
				/* Obtener el valor */
				EXITO = config_setting_lookup_bool(setting, settingName,
						&DEFAULT);
				if (!EXITO)
					DEFAULT = true;
				else if (EXITO && DEFAULT)
					fprintf(stderr,
							"\n Opción Auto activada para el campo %s.\n"
								" Estableciendo valores por defecto.\n",
							settingFather);
				else if (EXITO && !DEFAULT)
					fprintf(
							stderr,
							"\n Opción Auto desactivada para el campo %s.\n"
								" Estableciendo valores del fichero de configuración.\n",
							settingFather);
			} else {
				DEFAULT = true;
				fprintf(stderr, "Error.No se ha podido leer el campo %s.\n"
					" Estableciendo valores por defecto.\n", settingFather);
			}
			if (DEFAULT) {
				DefaultValParams();
			} else {
				double val;
				sprintf(settingName, "MODEL_TYPE");
				if (!config_setting_lookup_int(setting, settingName,
						&ValBGParams->MODEL_TYPE)) {
					ValBGParams->MODEL_TYPE = MEDIAN_S_UP;
					fprintf(
							stderr,
							"No se encuentra la variable %s en el archivo de configuración o el tipo de dato es incorrecto.\n "
								"Establecer por defecto a %d \n", settingName,
							ValBGParams->MODEL_TYPE);

				}

				sprintf(settingName, "MORFOLOGIA");
				if (!config_setting_lookup_bool(setting, settingName,
						&ValBGParams->MORFOLOGIA)) {
					ValBGParams->MORFOLOGIA = true;
					fprintf(
							stderr,
							"No se encuentra la variable %s en el archivo de configuración o el tipo de dato es incorrecto.\n "
								"Establecer por defecto a %d \n", settingName,
							ValBGParams->MORFOLOGIA);

				}

				sprintf(settingName, "CVCLOSE_ITR");
				if (!config_setting_lookup_int(setting, settingName,
						&ValBGParams->CVCLOSE_ITR)) {
					ValBGParams->CVCLOSE_ITR = 1;
					fprintf(
							stderr,
							"No se encuentra la variable %s en el archivo de configuración o el tipo de dato es incorrecto.\n "
								"Establecer por defecto a %d \n", settingName,
							ValBGParams->CVCLOSE_ITR);

				}

				sprintf(settingName, "MAX_CONTOUR_AREA");
				if (!config_setting_lookup_int(setting, settingName,
						&ValBGParams->MAX_CONTOUR_AREA)) {
					ValBGParams->MAX_CONTOUR_AREA = 0;
					fprintf(
							stderr,
							"No se encuentra la variable %s en el archivo de configuración o el tipo de dato es incorrecto.\n "
								"Establecer por defecto a %d \n", settingName,
							ValBGParams->MAX_CONTOUR_AREA);
				}

				sprintf(settingName, "MIN_CONTOUR_AREA");
				if (!config_setting_lookup_int(setting, settingName,
						&ValBGParams->MIN_CONTOUR_AREA)) {
					ValBGParams->MIN_CONTOUR_AREA = 0; //5
					fprintf(
							stderr,
							"No se encuentra la variable %s en el archivo de configuración o el tipo de dato es incorrecto.\n "
								"Establecer por defecto a %d \n", settingName,
							ValBGParams->MIN_CONTOUR_AREA);

				}

				sprintf(settingName, "HIGHT_THRESHOLD");
				if (!config_setting_lookup_int(setting, settingName,
						&ValBGParams->HIGHT_THRESHOLD)) {
					ValBGParams->HIGHT_THRESHOLD = 20;
					fprintf(
							stderr,
							"No se encuentra la variable %s en el archivo de configuración o el tipo de dato es incorrecto.\n "
								"Establecer por defecto a %d \n",
							settingName, ValBGParams->HIGHT_THRESHOLD);

				} else {
					if ((!ValBGParams->HIGHT_THRESHOLD
							|| ValBGParams->HIGHT_THRESHOLD > 255)) {
						ValBGParams->HIGHT_THRESHOLD = 20;
						fprintf(stderr,
								"El valor de %s está fuera de límites\n "
									"Establecer por defecto %s a %d \n",
								settingName, settingName, ValBGParams->BG_Update);
					}

				}

				sprintf(settingName, "LOW_THRESHOLD");
				if (!config_setting_lookup_int(setting, settingName,
						&ValBGParams->LOW_THRESHOLD)) {
					ValBGParams->LOW_THRESHOLD = 15;
					fprintf(
							stderr,
							"No se encuentra la variable %s en el archivo de configuración o el tipo de dato es incorrecto.\n "
								"Establecer por defecto a %d \n", settingName,
							ValBGParams->LOW_THRESHOLD);
				} else {
					if ((!ValBGParams->LOW_THRESHOLD
							|| ValBGParams->LOW_THRESHOLD > 255)) {
						ValBGParams->LOW_THRESHOLD = 15;
						fprintf(stderr,
								"El valor de %s está fuera de límites\n "
									"Establecer por defecto %s a %d \n",
								settingName, settingName, ValBGParams->BG_Update);
						if (ValBGParams->LOW_THRESHOLD > 50)
							fprintf(stderr,
									"ADVERTENCIA: El valor de %s es muy elevado\n "
										"Establecer por defecto %s a %d \n",
									settingName, settingName,
									ValBGParams->LOW_THRESHOLD);
					}

				}

				fprintf(stderr,"\n Cargando parámetros específicos de validación...");

				sprintf(settingName, "UmbralCirc");
				if (!config_setting_lookup_int(setting, settingName,
						&valParams->UmbralCirc)) {
					valParams->UmbralCirc = 0;
					fprintf(
							stderr,
							"No se encuentra la variable %s en el archivo de configuración o el tipo de dato es incorrecto.\n "
								"Establecer por defecto a %d \n",
							settingName, valParams->UmbralCirc);
				}

				sprintf(settingName, "Umbral_H");
				if (!config_setting_lookup_float(setting, settingName,
						&val)) {
					valParams->Umbral_H = 5; //5
					fprintf(
							stderr,
							"No se encuentra la variable %s en el archivo de configuración o el tipo de dato es incorrecto.\n "
								"Establecer por defecto a %d \n",
							settingName, ValBGParams->MIN_CONTOUR_AREA);

				}else {
					if ((!val
							|| val > 255)) {
						val = 5;
						fprintf(stderr,
								"El valor de %s está fuera de límites\n "
									"Establecer por defecto %s a %0.1f \n",
								settingName, settingName,
								val);
					}
					valParams->Umbral_H = (float)val;
				}

				sprintf(settingName, "Umbral_L");
				if (!config_setting_lookup_float(setting, settingName,
						&val)) {
					valParams->Umbral_L = 5;
					fprintf(
							stderr,
							"No se encuentra la variable %s en el archivo de configuración o el tipo de dato es incorrecto.\n "
								"Establecer por defecto a %d \n",
							settingName, ValBGParams->HIGHT_THRESHOLD);

				} else {
					if ((!val
							|| val > 255)) {
						val = 5;
						fprintf(stderr,
								"El valor de %s está fuera de límites\n "
									"Establecer por defecto %s a %0.1f \n",
								settingName, settingName,
								valParams->Umbral_L);
					}
					valParams->Umbral_L = (float)val;
				}

				sprintf(settingName, "Umbral_MinArea");
				if (!config_setting_lookup_int(setting, settingName,
						&valParams->Umbral_MinArea)) {
					valParams->Umbral_MinArea = 0;
					fprintf(
							stderr,
							"No se encuentra la variable %s en el archivo de configuración o el tipo de dato es incorrecto.\n "
								"Establecer por defecto a %d \n",
							settingName, valParams->Umbral_MinArea);
				}

				sprintf(settingName, "MaxIncLTHIters");
				if (!config_setting_lookup_int(setting, settingName,
						&valParams->MaxIncLTHIters)) {
					valParams->MaxIncLTHIters = 100;
					fprintf(
							stderr,
							"No se encuentra la variable %s en el archivo de configuración o el tipo de dato es incorrecto.\n "
								"Establecer por defecto a %d \n",
							settingName, valParams->MaxIncLTHIters);
				}
				sprintf(settingName, "MaxLowTH");
				if (!config_setting_lookup_int(setting, settingName,
						&valParams->MaxLowTH)) {
					valParams->MaxLowTH = 1000;
					fprintf(
							stderr,
							"No se encuentra la variable %s en el archivo de configuración o el tipo de dato es incorrecto.\n "
								"Establecer por defecto a %d \n",
							settingName, valParams->MaxLowTH);
				}
				sprintf(settingName, "MinLowTH");
				if (!config_setting_lookup_int(setting, settingName,
						&valParams->MinLowTH)) {
					valParams->MinLowTH = 1; // límite inferior para el umbral bajo ( defecto )
					fprintf(
							stderr,
							"No se encuentra la variable %s en el archivo de configuración o el tipo de dato es incorrecto.\n "
								"Establecer por defecto a %d \n",
							settingName, valParams->MinLowTH);
				}

			}
		}// fin comprobar auto hijos
	} // fin lectura correcta fichero
	valParams->privateParams = NULL;
	ShowValParams(settingFather);

	//	ShowStatsParams( settingFather );
	config_destroy(&cfg);
}

void DefaultValParams() {

	DefaultBGValParams();

	valParams->UmbralCirc = 0;
	valParams->Umbral_H = 5; // Establece el tamaño máximo del blob válido en 5 desviaciones típicas de la media
	valParams->Umbral_L = 5; // Establece el tamaño mínimo del blob válido en 1.5 desviaciones típicas de la media
	valParams->Umbral_MinArea = 0; // Establece el tamaño mínimo del blob válido en base al área mínima permitida.
	valParams->MaxDecLTHIters = 15; // número máximo de veces que se podrá decrementar el umbral bajo.
	valParams->MaxIncLTHIters = 100; // número máximo de veces que se podrá incrementar el umbral bajo
	valParams->MaxLowTH = 1000; // límite superior para el umbral bajo ( exceso )
	valParams->MinLowTH = 1; // límite inferior para el umbral bajo ( defecto )
	valParams->privateParams = NULL;

}

void DefaultBGValParams() {

	ValBGParams->MODEL_TYPE = MEDIAN_S_UP;
	ValBGParams->MORFOLOGIA = true;
	ValBGParams->CVCLOSE_ITR = 1;
	ValBGParams->MAX_CONTOUR_AREA = 0; //200
	ValBGParams->MIN_CONTOUR_AREA = 0; //5
	ValBGParams->K = 0.6745;

	ValBGParams->HIGHT_THRESHOLD = 20;
	ValBGParams->LOW_THRESHOLD = 15;

}

void PrivateValParams(SHModel* SH, ValParamsPrivate** privateParams) {

	ValParamsPrivate* Params;


	Params = (ValParamsPrivate *) malloc(sizeof(ValParamsPrivate));
	if (!Params) {
		error(4);
		exit(1);
	}
	Params->PxiMax = CalcPxMax(SH, valParams->Umbral_H, NULL ); // establece la máxima probabilidad permitida para defecto.
	Params->PxiMin = CalcPxMin(SH, valParams->Umbral_L, valParams->Umbral_MinArea); // establece la mínima probabilidad permitida para exceso.
	Params->HThInicio = ValBGParams->HIGHT_THRESHOLD;
	Params->LThInicio = ValBGParams->LOW_THRESHOLD;
	*privateParams = Params;
}

void ShowValParams(char* Campo) {

	printf(" \nVariables para el campo %s : \n", Campo);
	printf(" -MODEL_TYPE = %d \n", ValBGParams->MODEL_TYPE);
	printf(" -LOW_THRESHOLD = %d \n", ValBGParams->LOW_THRESHOLD);
	printf(" -MORFOLOGIA = %d \n", ValBGParams->MORFOLOGIA);
	printf(" -CVCLOSE_ITR = %d \n", ValBGParams->CVCLOSE_ITR);
	printf(" -MAX_CONTOUR_AREA = %d \n", ValBGParams->MAX_CONTOUR_AREA);
	printf(" -MIN_CONTOUR_AREA= %d \n", ValBGParams->MIN_CONTOUR_AREA);
	printf(" -HIGHT_THRESHOLD = %d \n", ValBGParams->HIGHT_THRESHOLD);
	printf(" -K = %0.6f \n", ValBGParams->K);

	printf(" -UmbralCirc = %d \n", valParams->UmbralCirc);
	printf(" -Umbral_H = %0.1f \n", valParams->Umbral_H);
	printf(" -Umbral_L = %0.1f \n", valParams->Umbral_L);
	printf(" -Umbral_MinArea = %d \n", valParams->Umbral_MinArea);
	printf(" -MaxDecLTHIters = %d \n", valParams->MaxDecLTHIters);
	printf(" -MaxIncLTHIters = %d \n", valParams->MaxIncLTHIters);
	printf(" -MaxLowTH = %d \n", valParams->MaxLowTH);
	printf(" -MinLowTH= %d \n", valParams->MinLowTH);

}

void ReleaseDataVal() {
	free(valParams->privateParams);
	free(valParams);
}
