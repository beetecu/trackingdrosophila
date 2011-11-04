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


// Inicializar los Parametros

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
	Pxi = exp( -abs((PI*FlyData->a*FlyData->b) - SH->FlyAreaMed) / SH->FlyAreaDes);
	return Pxi;
}

// Circularidad de cada mosca

double CalcCircul( STFly* FlyData){
	double circularidad;
	circularidad = ( 4*PI*FlyData->a*FlyData->b*PI)/(FlyData->perimetro*FlyData->perimetro);
	return circularidad;
}

// Probabilidades y Umbrales

int CalcProbUmbral( SHModel* SH,ValParams* VParams,STFly* FlieData ){

	float area_blob;
	float area_H;
	float area_L;
	double Pth_H;
	double Pth_L;

	area_blob= PI*FlieData->a*FlieData->b;

	area_L = VParams->UmbralProb*SH->FlyAreaDes + SH->FlyAreaMedia;
	area_H= VParams->UmbralProb*SH->FlyAreaDes - SH->FlyAreaMedia;

	Pth_L = exp( -abs(area_H - SH->FlyAreaMedia) / SH->FlyAreaDes);
	Pth_H= exp( -abs(area_L - SH->FlyAreaMedia) / SH->FlyAreaDes);

	VParams->Umbral_L=Pth_H; // Umbral Alto
	VParams->Umbral_H=Pth_L; // Umbral Bajo

	if ((area_blob - SH->FlyAreaMedia)<0)  return 0;
	else return 1;
}


// Obtener la maxima distancia de los pixeles al fondo

int ObtenerMaximo(IplImage* Imagen, STFrame* FrameData,CvRect Roi ){
	// obtener matriz de distancias normalizadas al background
	if (SHOW_VALIDATION_DATA == 1) printf(" \n\n Busqueda del máximo umbral...");
	IplImage* IDif = 0;
	IplImage* pesos=0;
	CvSize size = cvSize(Imagen->width,Imagen->height); // get current frame size
	if( !IDif || IDif->width != size.width || IDif->height != size.height ) {
		        cvReleaseImage( &IDif );
		        cvReleaseImage( &pesos );
		        IDif=cvCreateImage(cvSize(FrameData->BGModel->width,FrameData->BGModel->height), IPL_DEPTH_8U, 1); // imagen diferencia abs(I(pi)-u(p(i))
		        pesos=cvCreateImage(cvSize(FrameData->BGModel->width,FrameData->BGModel->height), IPL_DEPTH_8U, 1);//Imagen resultado wi ( pesos)
		        cvZero( IDif);
		        cvZero(pesos);
	}
	// |I(p)-u(p)|/0(p)
	cvAbsDiff(Imagen,FrameData->BGModel,IDif);
	cvDiv(IDif,FrameData->IDesv,pesos);
	// Buscar máximo
	int Maximo = 0;
	for (int y = Roi.y; y< Roi.y + Roi.height; y++){
		uchar* ptr1 = (uchar*) ( FrameData->FG->imageData + y*FrameData->FG->widthStep + 1*Roi.x);
		uchar* ptr2 = (uchar*) ( pesos->imageData + y*pesos->widthStep + 1*Roi.x);
		if (SHOW_VALIDATION_DATA == 1) printf(" \n\n");
		for (int x = 0; x<Roi.width; x++){
			if (SHOW_VALIDATION_DATA == 1) {
				if( ( y == Roi.y) && ( x == 0) ){
					printf("\n Origen: ( %d , %d )\n\n",(x + Roi.x),y);
				}
				printf("%d\t", ptr2[x]);
			}
			if ( ptr1[x] == 255 ){
				if (ptr2[x] > Maximo ) Maximo = ptr2[x];
			}
		}
	}
	printf("\n Maximo: %d ", Maximo);
	return Maximo;
}

					/////////////// VALIDACION \\\\\\\\\\\\\\\\\\\\\\\\\

tlcde* Validacion(IplImage *Imagen,
		STFrame* FrameData,
		SHModel* SH,
		CvRect Segroi,
		BGModelParams* BGParams,
		ValParams* VParams,IplImage* Mask){

	//Iniciar listas para almacenar las moscas

		tlcde* FLIE_LIST = NULL; // Contiene las moscas validadas para cada frame
		tlcde* PxList=NULL;

		FLIE_LIST = ( tlcde * )malloc( sizeof(tlcde ));
		PxList = ( tlcde * )malloc( sizeof(tlcde ));

		iniciarLcde( FLIE_LIST);
		iniciarLcde(PxList);

	//Inicializar estructura para almacenar los datos cada mosca

	STFly *FlyData = NULL;

	//Inicializar FrameData

	STFrame* FrameDataTemp=NULL;
	FrameDataTemp = ( STFrame *) malloc( sizeof(STFrame));

	double Pxi; // La probabilidad del blob actual
	bool Exceso;//Flag que indica si la Pxi minima no se alcanza por exceso o por defecto.
	double Circul; // Para almacenar la circularidad del blob actual
	double N=4; // Permite establecer la probabilidad minima ( Vparams->PxiMin )
	double Px; // Probabilidad de todas las mosca
	static int first=1;
	double Besthres=0;// el mejor de los umbrales perteneciente a la mejor probabilidad
	double BestPxi=0; // la mejor probabilidad obtenida para cada mosca

	cvZero(Mask);

	IplImage* mask=cvCreateImage(cvSize(FrameData->BGModel->width,FrameData->BGModel->height), IPL_DEPTH_8U, 1);;
	IplImage* maskTemp=cvCreateImage(cvSize(FrameData->BGModel->width,FrameData->BGModel->height), IPL_DEPTH_8U, 1);

	// Copiar la lista procedente de la segmentación en otras lista para su mejor manejo

	tlcde* FLIE=FrameData->Flies;
	tlcde* FLIE_TEMP=FLIE;
	tlcde* Temp=NULL;
	tlcde* TempSeg=NULL;

	// establecemos los parámetros de validación por defecto

	if ( VParams == NULL ){
		setValParams( &VParams);
	}

	// establecemos parámetros para los umbrales en la resta de fondo por defecto

	if ( BGParams == NULL){
		setBGModParams( &BGParams);
	}

	Px = CalcProbTotal(FLIE_TEMP,SH,VParams,FlyData); // Calcular la probabilidad de todas las mosca

	maskTemp=FrameData->FG;


// Recorremos los blobs uno por uno y los validamos.
// bucle for desde el primero hasta el ultimo individuo de la estructura mosca del frame actual

// Los blobs visitados y analizados se almacenan en una "cola" FIFO para una posterior busqueda en anchura
// de los blobs validados

for(int j=0;j<FLIE_TEMP->numeroDeElementos;j++){


	Temp=FLIE_TEMP;
	Besthres=0;
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




				// Comprobamos si no alcanza la probabilidad umbral minima devido
				// a un area superior al rango establecido

				if( Exceso ){

					VParams->MaxLowTH = ObtenerMaximo(Imagen, FrameData ,FlyData->Roi);
					VParams->PxiMin=VParams->Umbral_L;// Establecer el Umbral Bajo
					VParams->PxiMax=VParams->Umbral_H;// Establecer el Umbral Alto


					while( !SEG ){


						/* Incrementar paulatinamente el umbral de resta de fondo hasta
						 * que no haya Execeso comprobando si hay o no segmentación, en este caso
						 * añadir los blobs resultantes al final de la "cola" para ser validados
						 */

						/* El máximo umbral será el pixel del blob con mayor distancia normalizada
						 * al background. En general dicho punto coincidirá aproximadamente con el
						 * centro del blob, habrá un tamaño del blob en el cual a probabilidad
						 *  Pxi será maxima ( cerca de 1) y en para el cual ya no hay Exceso a
						 *  partir del cual deje de aumentar el umbral.
						 */


						// Incrementar umbral
						BGParams->LOW_THRESHOLD += 1;


						// Resta de fondo
						BackgroundDifference( Imagen, FrameData->BGModel,FrameData->IDesv,FrameData->FG,BGParams, FlyDataTemp);


						// Segmentar
						FrameData->Flies = segmentacion(Imagen, FrameData, FlyDataTemp,mask);

						cvCopy(mask,FrameData->FG);


//						if (SHOW_VALIDATION_DATA == 1) {
//							printf(" \n\nMatriz de distancia normalizada al background |I(p)-u(p)|/0(p)");
//
//						IplImage* IDif=cvCreateImage(cvSize(FrameData->BGModel->width,FrameData->BGModel->height), IPL_DEPTH_8U, 1); // imagen diferencia abs(I(pi)-u(p(i))
//						IplImage* pesos=cvCreateImage(cvSize(FrameData->BGModel->width,FrameData->BGModel->height), IPL_DEPTH_8U, 1);//Imagen resultado wi ( pesos)
//
//						cvAbsDiff(Imagen,FrameData->BGModel,IDif);// |I(p)-u(p)|/0(p)
//
//						cvDiv( IDif,FrameData->IDesv,pesos );// Calcular
//
//						cvSetImageROI(FrameData->FG,FlyDataTemp);
//						cvSetImageROI(pesos,FlyDataTemp);
//
//						// Hallar Z y u={ux,uy}
//						for (int y = FlyDataTemp.y; y< FlyDataTemp.y + FlyDataTemp.height; y++){
//							uchar* ptr1 = (uchar*) ( FrameData->FG->imageData + y*FrameData->FG->widthStep + 1*FlyDataTemp.x);
//							uchar* ptr2 = (uchar*) ( pesos->imageData + y*pesos->widthStep + 1*FlyDataTemp.x);
//							printf(" \n\n");
//
//							for (int x = 0; x<FlyDataTemp.width; x++){
//
//									if( ( y == FlyDataTemp.y) && ( x == 0) ){
//										printf("\n Origen: ( %d , %d )\n\n",(x + FlyDataTemp.x),y);
//									}
//									printf("%d\t", ptr2[x]);
//
//
//								if ( ptr1[x] == 255 ) printf("%d\t", ptr2[x]);
//
//								else printf("0\t");
//							}
//
//							cvResetImageROI( FrameData->FG );
//							cvResetImageROI( pesos );
//
//							}//for
//
//
//						cvReleaseImage(&IDif);
//						cvReleaseImage(&pesos);
//
//						}

						// Comprobar si hay segmentacion

						if ( FrameData->Flies->numeroDeElementos > 1 ){
							SEG = true;
							break;
						}

						else SEG=false;

						// Calcular Pxi de los blob resultantes

						FlyData=(STFly *)obtener(0, FrameData->Flies);


						Pxi = CalcProbMosca( SH , FlyData );

						if(Pxi>BestPxi) {

							BestPxi=Pxi;
							Besthres=BGParams->LOW_THRESHOLD;
						}


						Exceso = CalcProbUmbral( SH, VParams,FlyData); /// calcula VParams->UmbralProb

						if(!Exceso ) break;

//						if(!Exceso && Pxi < VParams->PxiMin) break;
//						if(Pxi > VParams->PxiMin) break;

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

							FlyData=(STFly *)obtener(j, FLIE);
							FlyData->flag_seg=true;

							for(int k=0;k < FrameData->Flies->numeroDeElementos;k++){

							TempSeg=FrameData->Flies;

							FlyData=(STFly *)obtener(k, TempSeg);

							Pxi = CalcProbMosca( SH , FlyData );
							Exceso = CalcProbUmbral( SH, VParams,FlyData);

							anyadirAlFinal(FlyData, FLIE_TEMP );// añadir al final dela Lista FLIE

							insertar(FlyData,PxList); // insertar en lista  para calcular la Px final


							}//fin for

						}//Fin if, Fin de la segmentación


						// Si no hay segementación mantener el blob en la misma posición dentro de la cola

						else{

//							FLIE_TEMP->actual->dato=FlyData;
//							insertar(FlyData,PxList); //Insertar en lista para calcular la Px final

							BGParams->LOW_THRESHOLD=Besthres;
							BackgroundDifference( Imagen, FrameData->BGModel,FrameData->IDesv,maskTemp,BGParams, FlyDataTemp);
							FrameData->Flies = segmentacion(Imagen, FrameData, FlyDataTemp,mask);
							FlyData=(STFly*)obtener(0,FrameData->Flies);
							Pxi = CalcProbMosca( SH , FlyData );
							FLIE->actual->dato=FlyData;
							insertar(FlyData,PxList);
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


//					while(!Exceso && BGParams->LOW_THRESHOLD >0 && Pxi > VParams->Umbral_H){
					while(!Exceso && BGParams->LOW_THRESHOLD >0){

					// Incrementar umbral
					BGParams->LOW_THRESHOLD -=1;


					// Resta de fondo
					BackgroundDifference( Imagen, FrameData->BGModel,FrameData->IDesv,FrameData->FG,BGParams, FlyData->Roi);


					// Segmentar
					FrameData->Flies = segmentacion(Imagen, FrameData, FlyData->Roi,mask);

					cvCopy(mask,FrameData->FG);

					FlyData=(STFly *)obtener(0, FrameData->Flies);

					if(!FrameData->Flies->numeroDeElementos) break;

					Pxi = CalcProbMosca( SH , FlyData );
					Exceso = CalcProbUmbral( SH, VParams,FlyData);

					}// while defecto

					BGParams=NULL;// Inicializar LOW_THRESHOLD y demas valores
					setBGModParams( &BGParams);

					if(!FrameData->Flies->numeroDeElementos){

						FlyData=(STFly *)obtener(j,FLIE_TEMP);
						FlyData->flag_def = true;
						FLIE->actual->dato=FlyData;
					}

					else{

						FlyData=(STFly *)obtener(j,FLIE_TEMP);
						FLIE->actual->dato=FlyData;

					}



				} //Fin defecto


}//Fin del FOR

				///// AHORA ES CUANDO REALMENTE VALIDAMOS/////

	Px=CalcProbTotal(PxList,SH,VParams,FlyData); // Calcular la Px para todas las moscas

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

}//Fin de Validación



