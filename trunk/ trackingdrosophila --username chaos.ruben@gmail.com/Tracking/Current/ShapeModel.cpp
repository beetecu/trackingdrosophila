/*
 * ShapeModel.cpp
 *
 *  Created on: 19/09/2011
 *      Author: chao
 */

#include "ShapeModel.hpp"


void ShapeModel( CvCapture* g_capture, SHModel* SH,IplImage* ImMask, CvRect ROI ){

	int num_frames = 0;
	int total_blobs=0;
	int k=0;
	float Dif=0;

	float areas[SM_FRAMES_TRAINING*15]; // almacena las areas de todos los blobs encontrados

	CBlobResult blobs;
	CBlob *currentBlob;


	IplImage* ImGray = cvCreateImage(cvGetSize( ImMask ), 8, 1 );
	IplImage* Imblob = cvCreateImage(cvGetSize( ImMask ), 8, 3 );

	//Pone a 0 los valores del vector areas

	for(int i=0;i<SM_FRAMES_TRAINING*15;i++){
		areas[i]=0;
	}

	//EXTRACCION DE LOS BLOBS Y CALCULO DE MEDIANA/MEDIA Y DESVIACION TIPICA PARA TODOS LOS FRAMES

	while( num_frames < SM_FRAMES_TRAINING ){
		IplImage* frame = cvQueryFrame( g_capture );
		if ( !frame ) {
			error(2);
			break;
		}
		if ( (cvWaitKey(10) & 255) == 27 ) break;

		ImPreProcess( frame, ImGray, ImMask, true, ROI);
		cvSetImageROI( ImGray, ROI);
		cvShowImage( "Drosophila.avi", ImGray );

		//Obtener los Blobs y excluir aquellos que no interesan por su tamaño

		blobs = CBlobResult( ImGray, NULL, 100, true );
		blobs.Filter( blobs, B_EXCLUDE, CBlobGetArea(),B_GREATER,100);
		blobs.Filter( blobs, B_EXCLUDE, CBlobGetPerimeter(),B_GREATER,1000);

		int j = blobs.GetNumBlobs();//numero de blos encontrados en el frame

		total_blobs=total_blobs+j; // Contabiliza los blobs encontrados para todos los frames

		// Guardar los Blobs en un archivo txt (OPCIONAL)

		//Recorrer Blob a blob y obtener las caracteristicas del AREA de cada uno de ellos

		for (int i = 0; i < blobs.GetNumBlobs(); i++ ){ //for 1

			currentBlob = blobs.GetBlob(i);

			CBlobGetArea();

			areas[k]=currentBlob->area;// almacenar el valor del area del blob en el vector

			// Mediana de las Areas

			if(currentBlob->area < SH->FlyAreaMed) SH->FlyAreaMed=SH->FlyAreaMed-1;
			if(currentBlob->area > SH->FlyAreaMed) SH->FlyAreaMed=SH->FlyAreaMed+1;

			//Media de las Areas

			SH->FlyAreaMedia=SH->FlyAreaMedia+currentBlob->area;//sumatorio de los valores de las areas

			if(SHOW_SHAPE_MODEL_DATA_AREAS) printf("Area blob %d = %f ",i,currentBlob->area);

			currentBlob->FillBlob( Imblob, CV_RGB(255,0,0));
			cvShowImage("Foreground", Imblob);

			k++;//incrementar indice del vector que contiene las areas

		}//Fin del For 1

		num_frames += 1;
		cvResetImageROI(ImGray);

	}//Fin del while

	SH->FlyAreaMedia=SH->FlyAreaMedia/total_blobs;// Media de las Areas para cada frame

	//Calcular la desvición típica

	for(int l=0;l<SM_FRAMES_TRAINING*15;l++){ // For 2

		Dif=abs(areas[l]-SH->FlyAreaMed);// valor del area - mediana

		int valor=areas[l];

		if(valor == 0) break;

		if(Dif < SH->FlyAreaDes) SH->FlyAreaDes=SH->FlyAreaDes-1;
		if(Dif > SH->FlyAreaDes) SH->FlyAreaDes=SH->FlyAreaDes+1;

	} // Fin del For 2

	//Mostrar mediana y media para todos los frames

	if(SHOW_SHAPE_MODEL_DATA_MEDIANA==1) printf("\n MEDIANA AREAS: %f \ t MEDIA AREAS: %f \t DESVIACION AREAS: %f",SH->FlyAreaMed,SH->FlyAreaMedia,SH->FlyAreaDes);

}//Fin de la función ShapeModel
