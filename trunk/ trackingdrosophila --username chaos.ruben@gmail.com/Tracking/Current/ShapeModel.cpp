/*
 * ShapeModel.cpp
 *
 *  Created on: 19/09/2011
 *      Author: chao
 */

#include "ShapeModel.hpp"

void ShapeModel( CvCapture* g_capture, int* FlyAreaMed, int* FlyAreaDes,IplImage* ImMask, CvRect ROI ){

	int num_frames = 0;

	IplImage* ImGray = cvCreateImage(cvGetSize( ImMask ), 8, 1 );
	IplImage* Imblob = cvCreateImage(cvGetSize( ImMask ), 8, 3 );

	while( num_frames < SM_FRAMES_TRAINING ){
		IplImage* frame = cvQueryFrame( g_capture );
		if ( !frame ) {
			error(2);
			break;
		}
		if ( (cvWaitKey(10) & 255) == 27 ) break;

		PreProcesado( frame, ImGray, ImMask, true, ROI);
		cvSetImageROI( ImGray, ROI);
		cvShowImage( "Drosophila.avi", ImGray );
	//	cvWaitKey(0);
//		cvCanny(ImGray,)
		CBlobResult blobs;
		CBlob *currentBlob;


		//Obtener los Blobs y excluir aquellos que no interesan por su tamaño

		blobs = CBlobResult( ImGray, NULL, 100, true );
		blobs.Filter( blobs, B_EXCLUDE, CBlobGetArea(),B_GREATER,1000);
//		blobs.Filter( blobs, B_EXCLUDE, CBlobGetPerimeter(),B_GREATER,1000);

		int j = blobs.GetNumBlobs();
		printf("%d\n",j);

		// Guardar los Blobs en un archivo txt (OPCIONAL)



		//Recorrer Blob a blob y obtener las caracteristicas de cada uno de ellos

		for (int i = 0; i < blobs.GetNumBlobs(); i++ ){

			currentBlob = blobs.GetBlob(i);

			CBlobGetArea();
			printf("Area blob %d = %f ",i,currentBlob->Area());

			currentBlob->FillBlob( Imblob, CV_RGB(255,0,0));

			cvShowImage("Foreground", Imblob);

		}
//					cvWaitKey(0);
		num_frames += 1;
		cvResetImageROI(ImGray);
	}
}
