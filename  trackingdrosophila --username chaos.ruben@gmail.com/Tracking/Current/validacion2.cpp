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


// Inicializar los Parametros para la validación

void setValParams( ValParams** Parameters){

	ValParams* Params;
	static int first=1;
	Params = ( ValParams *) malloc( sizeof( ValParams) );
	if( *Parameters == NULL )
	  {
		Params->UmbralCirc = 0;
		Params->Umbral_H=0;
		Params->Umbral_L=0;
		Params->MaxDecLTHIters = 20;
		Params->MaxIncLTHIters= 20;
		Params->MaxLowTH = 20;
		Params->PxiMin = 0;
		Params->MinLowTH = 1;
		Params->UmbralProb=3;
		Params->PxiMax=0;

		if (CREATE_TRACKBARS == 1){
				if(first==1){
				cvCreateTrackbar("UmbralProb","Foreground",&Params->UmbralProb,1);
				cvCreateTrackbar("UmbralCirc","Foreground",&Params->UmbralCirc,1);
				first=0;
				}
		}
	  }
	else
	{
	Params=*Parameters;
	}

	*Parameters=Params;
}

//Inicializar los parametros para el modelado de fondo

void setBGModParams( BGModelParams** Parameters){
	BGModelParams* Params;
	 Params = ( BGModelParams *) malloc( sizeof( BGModelParams) );
	    if( *Parameters == NULL )
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
	    Params=*Parameters;

	    }

	    *Parameters=Params;
}

//Probabilidad de todas las moscas

double CalcProbTotal(tlcde* Lista,SHModel* SH,ValParams* VParams,STFly* FlyData){

	double PX=1;
	double Pxi=0;

	for(int i=0;i < Lista->numeroDeElementos;i++){

		FlyData=(STFly *)obtener(i, Lista);
		CalcProbUmbral( SH, VParams,FlyData);
		Pxi = CalcProbMosca( SH ,FlyData);
		PX=PX*Pxi;
	}

	return PX;
}

// Probabilidad de cada mosca

double CalcProbMosca( SHModel* SH , STFly* FlyData ){
	double Pxi;
	Pxi = exp( -abs((CV_PI*FlyData->a*FlyData->b) - SH->FlyAreaMedia) / SH->FlyAreaDes);
	return Pxi;
}

// Circularidad de cada mosca

double CalcCircul( STFly* FlyData){
	double circularidad;
	circularidad = ( 4*CV_PI*FlyData->a*FlyData->b*CV_PI)/(FlyData->perimetro*FlyData->perimetro);
	return circularidad;
}

// Probabilidades y  Umbrales

int CalcProbUmbral( SHModel* SH,ValParams* VParams,STFly* FlieData ){

	float area_blob;
	float area_H;// area que establce el umbral bajo
	float area_L;// area que estableceel umbral alto.
	double Pth_H;
	double Pth_L;

	area_blob= CV_PI*FlieData->a*FlieData->b;

	area_L = VParams->UmbralProb*SH->FlyAreaDes + SH->FlyAreaMedia;
	area_H= VParams->UmbralProb*SH->FlyAreaDes - SH->FlyAreaMedia;

	Pth_L = exp( -abs(area_H - SH->FlyAreaMedia) / SH->FlyAreaDes);
	Pth_H= exp( -abs(area_L - SH->FlyAreaMedia) / SH->FlyAreaDes);

	VParams->Umbral_L=Pth_H; // Umbral Alto
	VParams->Umbral_H=Pth_L; // Umbral Bajo

	//Si el area del blob es menor el la media, el error es por defecto,
	//si es mayor que la media el error será por exceso.

	if ((area_blob - SH->FlyAreaMedia)<0)  return 0;
	else return 1;
}


// Obtener la maxima distancia de los pixeles al fondo

double* ObtenerMaximo(IplImage* Imagen, STFrame* FrameData,CvRect Roi ){
	// obtener matriz de distancias normalizadas al background
	if (SHOW_VALIDATION_DATA == 1) printf(" \n\n Busqueda del máximo umbral...");
	IplImage* IDif = 0;
	IplImage* peso=0;
	CvSize size = cvSize(Imagen->width,Imagen->height); // get current frame size
	if( !IDif || IDif->width != size.width || IDif->height != size.height ) {
		        cvReleaseImage( &IDif );
		        cvReleaseImage( &peso );
		        IDif=cvCreateImage(cvSize(FrameData->BGModel->width,FrameData->BGModel->height), IPL_DEPTH_8U, 1); // imagen diferencia abs(I(pi)-u(p(i))
		        peso=cvCreateImage(cvSize(FrameData->BGModel->width,FrameData->BGModel->height), IPL_DEPTH_32F, 1);//Imagen resultado wi ( pesos)
		        cvZero( IDif);
		        cvZero(peso);
	}

	// |I(p)-u(p)|/0(p)
	cvAbsDiff(Imagen,FrameData->BGModel,IDif);
	cvDiv(IDif,FrameData->IDesvf,peso);

	// Buscar máximo
	double* Maximo=0;;
	cvMinMaxLoc(peso,Maximo,0,0,0,FrameData->FG);

	return Maximo;
}

					////////////////// VALIDACION /////////////////

tlcde* Validacion(IplImage *Imagen,
		STFrame* FrameData,
		SHModel* SH,
		CvRect Segroi,
		BGModelParams* BGParams,
		ValParams* VParams,IplImage* Mask){

	//Iniciar listas para almacenar los blobs

		tlcde* FLIE_LIST = NULL; // Contiene las moscas validadas para cada frame.
		tlcde* PxList=NULL; // Contiene las moscas para el caculo de la probabilidad total.

		FLIE_LIST = ( tlcde * )malloc( sizeof(tlcde ));
		PxList = ( tlcde * )malloc( sizeof(tlcde ));

		iniciarLcde( FLIE_LIST);
		iniciarLcde(PxList);

	//Inicializar estructura para almacenar los datos cada mosca

	STFly *FlyData = NULL;

	//Inicializar FrameDataTemp

	STFrame* FrameDataTemp=NULL;
	FrameDataTemp = ( STFrame *) malloc( sizeof(STFrame));


	double Pxi; // La probabilidad del blob actual
	bool Exceso;//Flag que indica si la Pxi minima no se alcanza por exceso o por defecto.
	double Circul; // Para almacenar la circularidad del blob actual
	double Px; // Probabilidad de todas las mosca
	double Besthres=0;// el mejor de los umbrales perteneciente a la mejor probabilidad
	double BestPxi=0; // la mejor probabilidad obtenida para cada mosca

	cvZero(Mask);

	// Copiar la lista procedente de la segmentación en otras lista para su mejor manejo

	tlcde* FLIE=FrameData->Flies;
	tlcde* FLIE_TEMP=FLIE;
	tlcde* Temp=NULL;
	tlcde* TempSeg=NULL;

	// Establecemos los parámetros de validación por defecto

	if ( VParams == NULL ){
		setValParams( &VParams);
	}

	// Establecemos parámetros para los umbrales en la resta de fondo por defecto

	if ( BGParams == NULL){
		setBGModParams( &BGParams);
	}


// Recorremos los blobs uno por uno y los validamos.
// Bucle for desde el primero hasta el ultimo individuo de la Lista FrameData->Flie del frame actual.
// Los blobs visitados y analizados se almacenan en una "cola" FIFO para una posterior busqueda en anchura
// de los blobs validados

for(int j=0;j<FLIE_TEMP->numeroDeElementos;j++){


	Temp=FLIE_TEMP;
	Besthres=BGParams->LOW_THRESHOLD;
	BestPxi=0;

	// Almacenar la Roi del blob visitado antes de ser validado

	FlyData=(STFly *)obtener(j, Temp);
	CvRect FlyDataTemp=FlyData->Roi;


	bool SEG = false; // Indica si el blob fue segmentado
	FlyData->flag_seg = false; // Flag que indica si el blob fue validado
	FlyData->flag_def = false; // Flag que indica si el blob deparece al validar en caso de Defecto

	Pxi = CalcProbMosca( SH ,FlyData);
	BestPxi=Pxi;

	Circul = CalcCircul( FlyData );
	Exceso = CalcProbUmbral( SH, VParams,FlyData); // Calcular los umbrales de la validación

//	VParams->MaxLowTH = *ObtenerMaximo(Imagen, FrameData ,FlyData->Roi);
	VParams->PxiMax=VParams->Umbral_H;// Establecer el Umbral Alto

	// Comprobar si existe Exceso o Defecto de area, en caso de Exceso la probabilidad del blob es muy pequeña
	// y el blob puede corresponder a varios blobs que forman parte de una misma moscas o puede corresponder a
	// un espurio.


				// En caso de EXCESO de Area

				if( Exceso){

					/* Incrementar paulatinamente el umbral de resta de fondo hasta
					* que no haya Exceso comprobando si hay o no segmentación, en este caso
					* añadir los blobs resultantes al final de la "cola" para ser validados.
					*
					* En cada iteración se comprueba:
					*
					* 1.- Si el blob desaprace al incrementar el umbral, el blob es un espurio.
					* 2.- Si se detecta mas de un blob, existe segmentación.
					* 3.- Si el blob no desaparece y no queda segmentado, icrementar el umbral hasta que el
					* 	  blob no tenga exceso.En este caso nos quedamos con el umbral que corresponde a la probabilidad
					*     mas alta, qu será el blob valido.
					*
					*/


					while( Exceso ){

						// Incrementar umbral
						BGParams->LOW_THRESHOLD += 1;


						// Resta de fondo
						BackgroundDifference( Imagen, FrameData->BGModel,FrameData->IDesvf,FrameData->FG,BGParams, FlyDataTemp);


						// Segmentar
						FrameData->Flies = segmentacion(Imagen, FrameData, FlyDataTemp,Mask);

						// Si blob desaparece al aumentar el umbral, poner el flag_def como NO mosca

						if(FrameData->Flies->numeroDeElementos < 1){

							FlyData=(STFly *)obtener(j,FLIE_TEMP);
							FlyData->flag_def = true;
							FLIE->actual->dato=FlyData;

							break;
						}


						// Comprobar si hay segmentacion

						if ( FrameData->Flies->numeroDeElementos > 1 ){
							SEG = true;
							FrameDataTemp->Flies=FrameData->Flies;
							break;
						}

						else SEG=false;

						// Calcular Pxi del blob resultante

						FlyData=(STFly *)obtener(0, FrameData->Flies);

						Pxi = CalcProbMosca( SH , FlyData );


						//Almacenar la probabilidad mas alta, así como el umbral correspondiente a esa probabilidad

						if(Pxi>BestPxi) {

							BestPxi=Pxi;
							Besthres=BGParams->LOW_THRESHOLD;
						}

						//Si no hay execeso,salir del bucle y validar blob
						Exceso = CalcProbUmbral( SH, VParams,FlyData); // calcula VParams->UmbralProb
						if(!Exceso ) break;


					}// Fin del While

					BGParams=NULL;// Inicializar LOW_THRESHOLD y demas valores
					setBGModParams( &BGParams);

					// Validar las condiciones de salida del bucle

					/* Si se consige segmentar (en 2 o mas )añadir los blobs resultantes al final de la cola,
					 * y si no se consigue segmentar dejar el blob visitado en la misma posición dentro de
					 * la cola pero con los parametros pertenecientes a la probabilidad maxima para esa mosca
					 */

					// Si hay segmentacion,añadimos los blos(hijos) al final de la cola

						if (SEG){

							if(Besthres == BGParams->LOW_THRESHOLD) FlyData=(STFly*)obtener(j,FLIE_TEMP);

							else{

							BGParams->LOW_THRESHOLD=Besthres;


							// Resta de fondo
							BackgroundDifference( Imagen, FrameData->BGModel,FrameData->IDesvf,FrameData->FG,BGParams, FlyDataTemp);

							// Segmentar
							FrameData->Flies = segmentacion(Imagen, FrameData, FlyDataTemp,Mask);

							FlyData=(STFly*)obtener(0,FrameData->Flies);

							}


							Pxi = CalcProbMosca( SH , FlyData );

							// Si la probabilidad de la mosca es menor que el umbral (Probabilidad para 3 veces la
							// desviación típica), marcar la mosca como segmentada.Si la probabilidad de la mosca
							// es mayor que el umbral, la mosca no se maraca como segmentada. Esto lo realizamos
							// para descartar falsas segmentaciones. Si la probabilidad de oscaes muy baja = segementación true,
							// y si la probabilidad de mosca es alta = segmentacion false.

							if(Pxi < VParams->Umbral_H){

							FlyData=(STFly *)obtener(j, FLIE);
							FlyData->flag_seg=true;

							}

							else insertar(FlyData,PxList);

							FLIE->actual->dato=FlyData;

							// Añadir los nuevos blobs resultantes de la segementación al final dela lista FIFO

							for(int k=0;k < FrameDataTemp->Flies->numeroDeElementos;k++){

							TempSeg=FrameDataTemp->Flies;

							FlyData=(STFly *)obtener(k, TempSeg);

//							Pxi = CalcProbMosca( SH , FlyData );
//							Exceso = CalcProbUmbral( SH, VParams,FlyData);

							anyadirAlFinal(FlyData, FLIE_TEMP );// añadir al final de la Lista FLIE


							}//fin for

						}//Fin if, Fin de la segmentación


						// Si no hay segementación mantener el blob en la misma posición dentro de la cola

						else{

							if(FrameData->Flies->numeroDeElementos > 0){

							BGParams->LOW_THRESHOLD=Besthres;

							// Resta de fondo
							BackgroundDifference( Imagen, FrameData->BGModel,FrameData->IDesvf,FrameData->FG,BGParams, FlyDataTemp);

							// Segmentar
							FrameData->Flies = segmentacion(Imagen, FrameData, FlyDataTemp,Mask);

							FlyData=(STFly*)obtener(0,FrameData->Flies);
//							Pxi = CalcProbMosca( SH , FlyData );

							FLIE->actual->dato=FlyData;
							insertar(FlyData,PxList);

							}

						}

						BGParams=NULL;// Inicializar LOW_THRESHOLD y demas valores
						setBGModParams( &BGParams);



			} //Fin Exceso

			/* En el caso de DEFECTO disminuir el umbral, hasta que la probabilidad del blob Pxi sea inferior
			 * al Umbral maximo y si este no ha desaparecido al disminuir el umbral, validar el blob como
			 * mosca y mantener su posición dentro de la cola, en el caso de que el blob desaparezca durante
			 * la disminución del umbral o antes de que el umbral alcance el valor de cero validar como NO
			 * mosca y poner el flag_def a true para eliminar el blob de la cola.
			 */

				else{

					BGParams=NULL;// Inicializar LOW_THRESHOLD y demas valores
					setBGModParams( &BGParams);

					while(!Exceso && BGParams->LOW_THRESHOLD >0){

					// Incrementar umbral
					BGParams->LOW_THRESHOLD -=1;


					// Resta de fondo
					BackgroundDifference( Imagen, FrameData->BGModel,FrameData->IDesvf,FrameData->FG,BGParams, FlyData->Roi);

					// Segmentar
					FrameData->Flies = segmentacion(Imagen, FrameData, FlyData->Roi,Mask);


					FlyData=(STFly *)obtener(0, FrameData->Flies);

					if(!FrameData->Flies->numeroDeElementos) break;

//					Pxi = CalcProbMosca( SH , FlyData );
//					Exceso = CalcProbUmbral( SH, VParams,FlyData);

					}// while defecto

					BGParams=NULL;// Inicializar LOW_THRESHOLD y demas valores
					setBGModParams( &BGParams);

					//Si el blob ha desaparecido al disminuir el umbral poner el flag a 1

					if(!FrameData->Flies->numeroDeElementos){

						FlyData=(STFly *)obtener(j,FLIE_TEMP);
						FlyData->flag_def = true;
						FLIE->actual->dato=FlyData;
					}

					//Si el blob no ha desaparecido dejarlo como esta

					else{

						FlyData=(STFly *)obtener(j,FLIE_TEMP);
						FLIE->actual->dato=FlyData;
						insertar(FlyData,PxList);

					}



				} //Fin defecto


}//Fin del FOR

				///// AHORA ES CUANDO REALMENTE VALIDAMOS/////

	// Calcular la Px para todas las moscas

	Px=CalcProbTotal(PxList,SH,VParams,FlyData);

	/* Recorrer los blob introducidos en la cola para realizar una "busqueda" en anchura y validarlos.
	 * Si el blob ha sido segmentado ( flag_seg ) o ha desaparecido al disminuir el umbral durante
	 * el Defecto ( flag_def ), no insertar el la lista ( FLIE_LIST).
	 */

	for(int p=0;p < FLIE->numeroDeElementos;p++){

		FlyData=(STFly *)obtener(p, FLIE);

		Pxi = CalcProbMosca( SH , FlyData );

		if(!FlyData->flag_seg && !FlyData->flag_def && Pxi > Px){ // Si el blob NO fue segmentado y cumple con las condiciones se añade a FLIE_LIST

			insertar(FlyData,FLIE_LIST);

		} // Si el blob SI fue segmentado no se añade a FLIE_LIST

	} // Fin del for

	return FLIE_LIST; // Guardar los blobs validados en la lista para meterlos en el frameBuf

	cvReleaseImage(&Imagen);
	cvReleaseImage(&Mask);
	cvReleaseImage(&FrameData->BGModel);
	cvReleaseImage(&FrameData->FG);
	cvReleaseImage(&FrameData->Frame);
	cvReleaseImage(&Mask);


}//Fin de Validación



