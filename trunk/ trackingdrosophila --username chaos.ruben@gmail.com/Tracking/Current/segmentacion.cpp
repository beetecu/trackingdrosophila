/*
 * segmentacion2.cpp
 *
 *  Created on: 13/09/2011
 *      Author: german
 */


#include "segmentacion.h"


	IplImage *IDif = 0;
	IplImage *IDifm = 0;
	IplImage *pesos = 0;
	IplImage *FGMask = 0;


void segmentacion( IplImage *Brillo, STCapas* Capa ,CvRect Roi ){

	// CREAR IMAGENES
	CvSize size = cvSize(Brillo->width,Brillo->height); // get current frame size

	if( !IDif || IDif->width != size.width || IDif->height != size.height ) {

	        cvReleaseImage( &IDif );
	        cvReleaseImage( &IDifm );
	        cvReleaseImage( &pesos );

	        IDif=cvCreateImage(cvSize(Capa->BGModel->width,Capa->BGModel->height), IPL_DEPTH_8U, 1); // imagen diferencia abs(I(pi)-u(p(i))
	        IDifm=cvCreateImage(cvSize(Capa->BGModel->width,Capa->BGModel->height), IPL_DEPTH_8U, 1);// IDif en punto flotante
	        pesos=cvCreateImage(cvSize(Capa->BGModel->width,Capa->BGModel->height), IPL_DEPTH_8U, 1);//Imagen resultado wi ( pesos)
	        FGMask=cvCreateImage(cvSize(Capa->BGModel->width,Capa->BGModel->height), IPL_DEPTH_8U, 1);// Mascara de fg con elipses rellenas
	        cvZero( FGMask);
	}

	cvCopy(Capa->FG,Capa->FGTemp);

	cvSetImageROI( Brillo , Roi);
	cvSetImageROI( Capa->BGModel, Roi );
	cvSetImageROI( Capa->IDesv, Roi );
	cvSetImageROI( Capa->FG, Roi );
	cvSetImageROI( Capa->FGTemp, Roi );

	cvSetImageROI( IDif, Roi );
	cvSetImageROI( IDifm, Roi );
	cvSetImageROI( pesos, Roi );
	cvSetImageROI( FGMask, Roi );

	CvScalar v;
	CvScalar d1,d2; // valores de la matriz diagonal de eigen valores, ejes de la elipse.
	CvScalar r1,r2; // valores de la matriz de los eigen vectores, orientación.

	//Crear storage y secuencia de los contornos

	CvMemStorage* storage = cvCreateMemStorage();
	CvSeq* first_contour=NULL;
	// Distancia normalizada de cada pixel a su modelo de fondo.
	cvAbsDiff(Brillo,Capa->BGModel,IDif);// |I(p)-u(p)|/0(p)
	cvConvertScale(IDif ,IDifm,1,0);// A float
	cvDiv(IDifm,Capa->IDesv,pesos);// Calcular

//	cvShowImage("Foreground", Capa->FG);
//	cvWaitKey(0);

	//Buscamos los contornos de las moscas en movimiento en el foreground

	int Nc = cvFindContours(Capa->FGTemp,
			storage,
			&first_contour,
			sizeof(CvContour),
			CV_RETR_EXTERNAL,
			CV_CHAIN_APPROX_SIMPLE,
			cvPoint(Roi.x,Roi.y));
	if( SHOW_SEGMENTATION_DATA == 1) printf( "\nTotal Contornos Detectados: %d ", Nc );

	for( CvSeq *c=first_contour; c!=NULL; c=c->h_next) {

		float z=0;  // parámetro para el cálculo de la matriz de covarianza

		/// Parámetros elipse
		float semiejemenor;
		float semiejemayor;
		CvSize axes;
		CvPoint centro;
		float tita; // orientación

		CvRect rect=cvBoundingRect(c,0); // Hallar los rectangulos para establecer las ROIs

		// CREAR MATRICES

		CvMat *vector_u=cvCreateMat(2,1,CV_32FC1); // Matriz de medias
		CvMat *vector_resta=cvCreateMat(2,1,CV_32FC1); // (pi-u)
		CvMat *matrix_mul=cvCreateMat(2,2,CV_32FC1);// Matriz (pi-u)(pi-u)T
		CvMat *MATRIX_C=cvCreateMat(matrix_mul->rows,matrix_mul->cols,CV_32FC1);// MATRIZ DE COVARIANZA
		CvMat *evects=cvCreateMat(2,2,CV_32FC1);// Matriz de EigenVectores
		CvMat *evals=cvCreateMat(2,2,CV_32FC1);// Matriz de EigenValores

		CvMat *Diagonal=cvCreateMat(2,2,CV_32FC1); // Matriz Diagonal,donde se extraen los ejes
		CvMat *R=cvCreateMat(2,2,CV_32FC1);// Matriz EigenVectores.
		CvMat *RT=cvCreateMat(2,2,CV_32FC1);

//		cvShowImage("Foreground", Capa->FG);
//		cvWaitKey(0);
		if (SHOW_SEGMENTATION_DATA == 1) {
			printf(" \n\nMatriz de distancia normalizada al background |I(p)-u(p)|/0(p)");
		}
		// Hallar Z y u={ux,uy}
		for (int y = rect.y; y< rect.y + rect.height; y++){
			uchar* ptr1 = (uchar*) ( Capa->FG->imageData + y*Capa->FG->widthStep + 1*rect.x);
			uchar* ptr2 = (uchar*) ( pesos->imageData + y*pesos->widthStep + 1*rect.x);
			if (SHOW_SEGMENTATION_DATA == 1) printf(" \n\n");
			for (int x= 0; x<rect.width; x++){
				if (SHOW_SEGMENTATION_DATA == 1) {
					printf("%d\t", ptr2[x]);
					if( ( y == rect.y) && ( x == 0) ){
						printf("\n Origen: ( %d , %d )",y,(x + rect.x));
					}
				}

				if ( ptr1[x] == 255 ){

					z = z + ptr2[x]; // Sumatorio de los pesos
					*((float*)CV_MAT_ELEM_PTR( *vector_u, 0, 0 )) = CV_MAT_ELEM( *vector_u, float, 0,0 )+ (x + rect.x)*ptr2[x];
					*((float*)CV_MAT_ELEM_PTR( *vector_u, 1, 0 )) = CV_MAT_ELEM( *vector_u, float, 1,0 )+ y*ptr2[x];
	//					vector_u[0][0] = vector_u[0][0] + x*ptr2[x]; // sumatorio del peso por la pos x
	//					vector_u[1][0] = vector_u[1][0] + y*ptr2[x]; // sumatorio del peso por la pos y
				}
			}
		}
		if ( z != 0) cvConvertScale(vector_u, vector_u, 1/z,0); // vector de media {ux, uy}
		if (SHOW_SEGMENTATION_DATA == 1){
			printf("\n\nCentro (vector u):\n %f \n %f\n",CV_MAT_ELEM( *vector_u, float, 0,0 ),CV_MAT_ELEM( *vector_u, float, 1,0 ));
		}



//		cvShowImage("Foreground", Capa->FGTemp);
//		cvWaitKey(0);

		for (int y = rect.y; y< rect.y + rect.height; y++){
			uchar* ptr1 = (uchar*) ( Capa->FG->imageData + y*Capa->FG->widthStep + 1*rect.x);
			uchar* ptr2 = (uchar*) ( pesos->imageData + y*pesos->widthStep + 1*rect.x);
			for (int x= 0; x<rect.width; x++){

				if ( ptr1[x] == 255 ){
					//vector_resta[0][0] = x - vector_u[0][0];
					//vector_resta[1][0] = y - vector_u[1][0];
					*((float*)CV_MAT_ELEM_PTR( *vector_resta, 0, 0 )) = (x + rect.x) - CV_MAT_ELEM( *vector_u, float, 0,0 );
					*((float*)CV_MAT_ELEM_PTR( *vector_resta, 1, 0 )) = y - CV_MAT_ELEM( *vector_u, float, 1,0 );
					cvGEMM(vector_resta, vector_resta, ptr2[x] , NULL, 0, matrix_mul,CV_GEMM_B_T);// Multiplicar Matrices (pi-u)(pi-u)T
					cvAdd( MATRIX_C, matrix_mul	, MATRIX_C ); // sumatorio
				}
			}
		}
		if ( z != 0) cvConvertScale(MATRIX_C, MATRIX_C, 1/z,0); // Matriz de covarianza

		// Mostrar matriz de covarianza
		if (SHOW_SEGMENTATION_DATA == 1) {
				printf("\nMatriz de covarianza");
								for(int i=0;i<2;i++){
									printf("\n\n");
									for(int j=0;j<2;j++){
										v=cvGet2D(MATRIX_C,i,j);
										printf("\t%f",v.val[0]);
									}
						}
		}

		// EXTRAER LOS EIGENVALORES Y EIGENVECTORES

		cvEigenVV(MATRIX_C,evects,evals,2);// Hallar los EigenVectores
		cvSVD(MATRIX_C,Diagonal,R,RT,0); // Hallar los EigenValores, MATRIX_C=R*Diagonal*RT

		//Extraer valores de los EigenVectores y EigenValores

		d1=cvGet2D(Diagonal,0,0);
		d2=cvGet2D(Diagonal,1,1);
		r1=cvGet2D(R,0,0);
		r2=cvGet2D(R,0,1);

		//Hallar los semiejes y la orientación

		semiejemayor=2*(sqrt(d1.val[0]));
		semiejemenor=2*(sqrt(d2.val[0]));
		tita=atan(r2.val[0]/r1.val[0]);

		if (SHOW_SEGMENTATION_DATA == 1){
			printf("\n\nElipse\nEJE MAYOR : %f EJE MENOR: %f ORIENTACION: %f",
					2*semiejemayor,
					2*semiejemenor,
					tita);
		}
		// Dibujar elipse

		//Eliminamos el blob

		for (int y = rect.y; y< rect.y + rect.height; y++){
					uchar* ptr1 = (uchar*) ( Capa->FGTemp->imageData + y*Capa->FGTemp->widthStep + 1*rect.x);
					for (int x= 0; x<rect.width; x++){
						ptr1[x] = 0;
					}
		}
//		cvSetImageROI( Capa->FGTemp, rect );
//		cvZero( Capa->FGTemp );
//		cvResetImageROI( Capa->FGTemp);
		// Obtenemos el centro de la elipse
		cvResetImageROI( Capa->FGTemp ); // para evitar el offset de find contours
		cvResetImageROI( FGMask);

		centro = cvPoint( cvRound( *((float*)CV_MAT_ELEM_PTR( *vector_u, 0, 0 )) ),
				cvRound( *((float*)CV_MAT_ELEM_PTR( *vector_u, 1, 0 )) ) );
		// Obtenemos los ejes y la orientacion en grados
		axes = cvSize( cvRound(semiejemayor) , cvRound(semiejemenor) );
		tita = (tita*180)/PI;
		cvEllipse( Capa->FGTemp, centro, axes, tita, 0, 360, cvScalar( 255,0,0,0), 1, 8);
		cvEllipse( FGMask, centro , axes, tita, 0, 360, cvScalar( 255,0,0,0), -1, 8);
		// Dibujar Roi para pruebas
		cvRectangle( Capa->FGTemp,
				cvPoint( rect.x, rect.y),
				cvPoint( rect.x + rect.width , rect.y + rect.height ),
				cvScalar(255,0,0,0),
				1);

		cvSetImageROI( Capa->FGTemp, Roi );
		cvSetImageROI( FGMask, Roi );

		cvReleaseMat(&vector_resta);
		cvReleaseMat(&matrix_mul);
		cvReleaseMat(&MATRIX_C);
		cvReleaseMat(&evects);
		cvReleaseMat(&evals);
		cvReleaseMat(&Diagonal);
		cvReleaseMat(&R);
		cvReleaseMat(&RT);

	}// Fin de contornos

//	cvShowImage("Foreground", Capa->FGTemp);
//			cvWaitKey(0);
	cvSetImageROI( Capa->ImFMask,Roi);
	invertirBW(  Capa->ImFMask );
	/*En la imagen resultante se ve la elipse rellenada con la imagen real
	 * de las moscas usando como máscara el foreground
	 */
	cvAdd(Capa->FGTemp,Brillo,Capa->FGTemp, FGMask);

	invertirBW(  Capa->ImFMask );
	cvShowImage("Foreground", Capa->FGTemp);
//		cvWaitKey(0);

	cvAdd ( Capa->FG, Capa->FGTemp, Capa->FGTemp);
//	cvShowImage("Foreground", Capa->FGTemp);
// 			cvWaitKey(0);
//
	cvCopy( FGMask, Capa->FGTemp);



	cvResetImageROI( Brillo );
	cvResetImageROI( Capa->BGModel );
	cvResetImageROI( Capa->IDesv );
	cvResetImageROI( Capa->FG );
	cvResetImageROI( Capa->FGTemp );
	cvResetImageROI( Capa->ImFMask );
//	cvResetImageROI( FGMask);

//	cvShowImage("Foreground", Capa->FGTemp);
//	cvWaitKey(0);
	// Liberar memoria
	cvReleaseImage(&IDif);
	cvReleaseImage(&IDifm);
	cvReleaseImage(&pesos);
	cvReleaseImage(&FGMask);
	cvReleaseMemStorage( &storage);

}//Fin de la función


