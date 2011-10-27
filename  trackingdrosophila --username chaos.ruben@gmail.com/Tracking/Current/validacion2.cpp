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




void setValParams( ValParams** Parameters){

	ValParams* Params;
	static int first=1;
	Params = ( ValParams *) malloc( sizeof( ValParams) );
	if( *Parameters == NULL )
	  {
		Params->UmbralCirc = 0;
		Params->UmbralDes=0;
		Params->MaxDecLTHIters = 20;
		Params->MaxIncLTHIters= 20;
		Params->MaxLowTH = 20;
		Params->PxiMin = 0;
		Params->MinLowTH = 1;
		Params->UmbralProb=3;

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
		Params = *Parameters;
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
	    Params = *Parameters;
	    }

	    *Parameters=Params;
}


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

double CalcProbMosca( SHModel* SH , STFly* FlyData ){
	double Pxi;
	Pxi = exp( -abs((PI*FlyData->a*FlyData->b) - SH->FlyAreaMed) / SH->FlyAreaDes);
	return Pxi;
}

double CalcCircul( STFly* FlyData){
	double circularidad;
	circularidad = ( 4*PI*FlyData->a*FlyData->b*PI)/(FlyData->perimetro*FlyData->perimetro);
	return circularidad;
}

int CalcProbUmbral( SHModel* SH,ValParams* VParams,STFly* FlieData ){

	float area_blob;
	float area;
	double Pth;
	area_blob= PI*FlieData->a*FlieData->b;
	area = VParams->UmbralProb*SH->FlyAreaDes + SH->FlyAreaMedia;
	Pth = exp( -abs(area - SH->FlyAreaMedia) / SH->FlyAreaDes);
	VParams->UmbralDes=Pth;
	if ((area_blob - SH->FlyAreaMedia)<0)  return 0;
	else return 1;
}

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



tlcde* Validacion(IplImage *Imagen,
		STFrame* FrameData,
		SHModel* SH,
		CvRect Segroi,
		BGModelParams* BGParams,
		ValParams* VParams,IplImage* Mask){

	//Iniciar lista para almacenar las moscas
		tlcde* FLIE = NULL;
		FLIE = ( tlcde * )malloc( sizeof(tlcde ));
		iniciarLcde( FLIE);

	//Inicializar estructura para almacenar los datos cada mosca
		STFly *FlyData = NULL;


	double Pxi; // La probabilidad del blob actual
	bool Exceso;//Flag que indica si la pxi minima no se alcanza x exceso o por defecto.
	double Circul; // Para almacenar la circularidad del blob actual
	double N=100; // Permite establecer la probabilidad minima ( Vparams->PxiMin )
	double Px;
	static int first=1;
	double Besthres=0;// el mejor de los umbrales
	double BestPxi=0;

	cvZero(Mask);

	IplImage* mask=cvCreateImage(cvSize(FrameData->BGModel->width,FrameData->BGModel->height), IPL_DEPTH_8U, 1);;
	IplImage* maskTemp=cvCreateImage(cvSize(FrameData->BGModel->width,FrameData->BGModel->height), IPL_DEPTH_8U, 1);

	FLIE=FrameData->Flies;
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


	// Recorremos los blobs uno por uno y los validamos.

	// bucle for desde el primero hasta el ultimo individuo de la estructura mosca del frame actual


//Insertamos los Padres en la lista FLIE

//for(int n =0;n < FrameData->Flies->numeroDeElementos;n++){
//
//	FlyData=(STFly *)obtener(n, FrameData->Flies);
//	insertar(FlyData, FLIE);
//
//}

// Recorremos los blobs uno por uno y los validamos.
// bucle for desde el primero hasta el ultimo individuo de la estructura mosca del frame actual

for(int j=0;j<FLIE_TEMP->numeroDeElementos;j++){


	Temp=FLIE_TEMP;


	FlyData=(STFly *)obtener(j, Temp);


	bool SEG = false;

	Pxi = CalcProbMosca( SH ,FlyData);
	Circul = CalcCircul( FlyData );
	Exceso = CalcProbUmbral( SH, VParams,FlyData); /// calcula VParams->UmbralProb

		// si no alcanza la P(xi) minima o bien tiene mucha circularidad
		if( ( Pxi < VParams->UmbralDes) || ( Circul > VParams->UmbralCirc) ){
			if( Pxi < VParams->UmbralDes ){
				// comprobamos si no alcanza la probabilidad minima devido
				// a un area superior o inferior al rango establecido

				if( Exceso ){
					VParams->MaxLowTH = ObtenerMaximo(Imagen, FrameData ,FlyData->Roi);
					VParams->PxiMin=VParams->UmbralProb/N;


					while( !SEG ){


						/* Incrementar paulatinamente el umbral de resta de fondo hasta el
						 * máximo comprobando si hay o no segmentación, si se supera o no
						 *  el umbral y si la pxi es inferior o superior al PxiMin
						 */
						/* El máximo umbral será el pixel del blob con mayor distancia normalizada
						 * al background. En general dicho punto coincidirá aproximadamente con el
						 * centro del blob habrá un tamaño minimo ( Pxi minima ) a partir del cual
						 * deje de reducirse el umbral y un valor maximo para el umbral ( MaxLowTH )
						 * Se puede ponderar con el valor del pixel del centro.
						 */



						// Incrementar umbral
						BGParams->LOW_THRESHOLD += 1;


						// Resta de fondo
						BackgroundDifference( Imagen, FrameData->BGModel,FrameData->IDesv,FrameData->FG,BGParams, FlyData->Roi);

						// Segmentar


						FrameData->Flies = segmentacion(Imagen, FrameData, FlyData->Roi,NULL);

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
//						cvSetImageROI(FrameData->FG,FlyData->Roi);
//						cvSetImageROI(pesos,FlyData->Roi);
//						// Hallar Z y u={ux,uy}
//						for (int y = FlyData->Roi.y; y< FlyData->Roi.y + FlyData->Roi.height; y++){
//							uchar* ptr1 = (uchar*) ( FrameData->FG->imageData + y*FrameData->FG->widthStep + 1*FlyData->Roi.x);
//							uchar* ptr2 = (uchar*) ( pesos->imageData + y*pesos->widthStep + 1*FlyData->Roi.x);
//							printf(" \n\n");
//
//							for (int x = 0; x<FlyData->Roi.width; x++){
//
//									if( ( y == FlyData->Roi.y) && ( x == 0) ){
//										printf("\n Origen: ( %d , %d )\n\n",(x + FlyData->Roi.x),y);
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
//							cvReleaseImage(&IDif);
//							cvReleaseImage(&pesos);
//
//
//						}//for
//
//						}

						// Comprobar si hay segmentacion

						if ( FrameData->Flies->numeroDeElementos > 1 ){
							SEG = true;
							FlyData->flag_seg=true; // Poner el flag de la segmentación a 1
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

//						if(!Exceso && Pxi < VParams->PxiMin) break;
//						if(!Exceso ) break;
						if(Pxi > VParams->PxiMin) break;

					}// Fin del While

					BGParams=NULL;// Inicializar LOW_THRESHOLD y demas valores
					setBGModParams( &BGParams);

					// Validar las condiciones de salida del bucle
					/* Si se consige segmentar (en 2 o mas )el blob borrar el blob actual de la estructura mosca,
					 * sustituirlo por uno de los segmentados y añadir al final el/los nuevo/s blob/s
					 * hacerlo aqui o modificar la función segmentación para que pueda hacerlo
					 */

					// Si hay segmentacion,añadimos los blos

						if (SEG){

//							Px = CalcProbTotal(FrameData->Flies,SH,VParams,FlyData);

							for(int k=0;k < FrameData->Flies->numeroDeElementos;k++){

							TempSeg=FrameData->Flies;

							FlyData=(STFly *)obtener(k, TempSeg);

							Pxi = CalcProbMosca( SH , FlyData );
							Exceso = CalcProbUmbral( SH, VParams,FlyData);

							anyadirAlFinal(FlyData, FLIE );


							}//fin for

						}//Fin if, fin de la segementación

//						else{
//
//							// Resta de fondo
//							BackgroundDifference( Imagen, FrameData->BGModel,FrameData->IDesv,FrameData->FG,BGParams, FlyData->Roi);
//
//							// Segmentar
//							FrameData->Flies = segmentacion(Imagen, FrameData, FlyData->Roi,NULL);
//						}//Fin else

			} //IF Exceso


		}// FIN IF2

	}// Fin IF 1

}//Fin del FOR

				///// AHORA ES CUANDO REALMENTE VALIDAMOS/////

//	Px=CalcProbTotal(FLIE,SH,VParams,FlyData);
//
//	for(int p=0;p < FLIE->numeroDeElementos;p++){
//
//		FlyData=(STFly *)obtener(p, FLIE);
//
//		if(FlyData->flag_seg){
//
//			borrar(FLIE);
//		}
//
//	}

	return FLIE; // Guardar los blobs validados en la lista para meterlos en el frameBuf

}//Fin de Validación



