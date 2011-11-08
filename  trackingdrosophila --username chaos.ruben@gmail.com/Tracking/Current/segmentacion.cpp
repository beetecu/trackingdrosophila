/*
 * segmentacion2.cpp
 *
 *  Created on: 13/09/2011
 *      Author: german
 *
 *  Cada blob se ajusta mediante una elipse. Los parámetros de la elipse se obtienen a partir de la
 *  gaussiana 2d del blob mediante la eigendescomposición de su matriz de covarianza. Los parámetros
 *  de la gaussiana guardan proporción con el peso de cada pixel siendo este peso proporcional
 *  a la distancia normalizada de la intensidad del pixel y el valor probable del fondo de ese pixel,
 *   de modo que el centro de la elipse conicidirá con el valor esperado de la gausiana 2d (la región
 *   más oscura) que a su vez coincidirá aproximadamente con el centro del blob.
 *  Una vez hecho el ajuste se rellena una estructura donde se almacenan los parámetros que definen
 *  las características del blob ( identificacion, posición, orientación, semieje a, semieje b y roi
 *  entre otras).
 */


#include "segmentacion.hpp"

	IplImage *FGTemp = 0;
	IplImage *IDif = 0;
	IplImage *IDifm = 0;
	IplImage *pesos = 0;
	IplImage *FGMask = 0;
	CvMat *vector_u; // Matriz de medias
	CvMat *vector_resta; // (pi-u)
	CvMat *matrix_mul;// Matriz (pi-u)(pi-u)T
	CvMat *MATRIX_C;// MATRIZ DE COVARIANZA
	CvMat *evects;// Matriz de EigenVectores
	CvMat *evals;// Matriz de EigenValores

	CvMat *Diagonal; // Matriz Diagonal,donde se extraen los ejes
	CvMat *R;// Matriz EigenVectores.
	CvMat *RT;

	CvMemStorage* storage;
	CvSeq* first_contour;

tlcde* segmentacion( IplImage *Brillo, STFrame* FrameData ,CvRect Roi,IplImage* Mask){


	//Iniciar lista para almacenar las moscas
	tlcde* flies = NULL;
	flies = ( tlcde * )malloc( sizeof(tlcde ));
	iniciarLcde( flies );
	//Inicializar estructura para almacenar los datos cada mosca
	STFly *flyData = NULL;

	// crear imagenes y datos si no se ha hecho e inicializar
	CreateDataSegm( Brillo );

	cvCopy(FrameData->FG,FGTemp);

	establecerROIS( FrameData, Brillo, Roi );

	// Distancia normalizada de cada pixel a su modelo de fondo.
//		cvShowImage("Foreground",FGTemp);
//				cvWaitKey(0);
	cvAbsDiff(Brillo,FrameData->BGModel,IDif);// |I(p)-u(p)|/0(p)
	cvConvertScale(IDif ,IDifm,1,0);// A float
	cvDiv( IDifm,FrameData->IDesv,pesos );// Calcular

	//Buscamos los contornos de las moscas en movimiento en el foreground

	int Nc = cvFindContours(FGTemp,
			storage,
			&first_contour,
			sizeof(CvContour),
			CV_RETR_EXTERNAL,
			CV_CHAIN_APPROX_NONE,
			cvPoint(Roi.x,Roi.y));


	int id=0; //id=etiqueta de la moca
	
	if( SHOW_SEGMENTATION_DATA == 1) printf( "\nTotal Contornos Detectados: %d ", Nc );

	int i = 0;
	for( CvSeq *c=first_contour; c!=NULL; c=c->h_next) {

		if( SHOW_SEGMENTATION_DATA == 1) printf("\n BLOB %d\n",id);
		id++; //incrmentar el Id de las moscas

		// Parámetros para calcular el error del ajuste en base a la diferencia de areas entre el Fg y la ellipse
		float err;
		float areaFG;
		float areaElipse;
		areaFG = cvContourArea(c);
		/// Parámetros elipse

		float semiejemenor;
		float semiejemayor;
		CvSize axes;
		CvPoint centro;
		float tita; // orientación

		CvRect rect=cvBoundingRect(c,0); // Hallar los rectangulos para establecer las ROIs
		// realiza el ajuste a una elipse de los pesos del rectangulo que coincidan con la máscara
		ellipseFit( rect, pesos, FrameData->FG, &semiejemenor,&semiejemayor,&axes,&centro,&tita );

		areaElipse = PI*semiejemenor*semiejemayor;
		err = abs(areaElipse - areaFG);

		if (SHOW_SEGMENTATION_DATA == 1){
			printf("\n AreaElipse = %f  Area FG = %f  Error del ajuste = %f", areaElipse,areaFG,err);
			printf("\n\nElipse\nEJE MAYOR : %f EJE MENOR: %f ORIENTACION: %f ",
					2*semiejemayor,
					2*semiejemenor,
					tita);
		}

		flyData = ( STFly *) malloc( sizeof( STFly));
		if ( !flyData ) {error(4);	exit(1);}
		flyData->etiqueta = 0;//id  ;  /// Identificación del blob
		flyData->Color = cvScalar( 0,0,0,0); /// Color para dibujar el blob
		flyData-> posicion = centro; /// Posición del blob
		flyData->a = semiejemayor;
		flyData->b = semiejemenor; /// semiejes de la elipse
		flyData->orientacion = tita; /// Almacena la orientación
		flyData->CountState = 0;
		flyData->direccion = 0;
		flyData->dstTotal = 0;
//		flyData->perimetro = cv::arcLength(contorno,0);
		flyData->Roi = rect;
		flyData->Estado = 1;  /// Flag para indicar que si el blob permanece estático ( 0 ) o en movimiento (1)
		flyData->num_frame = FrameData->num_frame;
		// Añadir a lista
		insertar( flyData, flies );

		//Mostrar los campos de cada mosca o blob
		if(SHOW_SEGMENTACION_STRUCT == 1){
		printf("\n EJE A : %f\t EJE B: %f\t ORIENTACION: %f",flyData->a,flyData->b,flyData->orientacion);
		}

		cvEllipse( FGTemp, centro, axes, tita, 0, 360, cvScalar( 255,0,0,0), 1, 8);

		cvEllipse( FGMask, centro , axes, tita, 0, 360, cvScalar( 255,0,0,0), -1, 8);
		// Dibujar Roi para pruebas
		cvRectangle( FGTemp,
				cvPoint( rect.x, rect.y),
				cvPoint( rect.x + rect.width , rect.y + rect.height ),
				cvScalar(255,0,0,0),
				1);
//		cvShowImage("Foreground", FGMask );
//		cvWaitKey(0);
		cvSetImageROI( FGTemp, Roi );
		cvSetImageROI( FGMask, Roi );

	}// Fin de contornos


// PRUEBAS visualizacion
//	cvShowImage("Foreground", FGTemp);
//			cvWaitKey(0);

	/*En la imagen resultante se ve la elipse rellenada con la imagen real
	 * de las moscas usando como máscara el foreground
	 */
//	cvAdd(FGTemp,Brillo,FGTemp, FGMask);
//
//
//
//	cvResetImageROI( FGTemp);
//	cvShowImage("Foreground", FGTemp);
//	cvSetImageROI( FGTemp,Roi);
////		cvWaitKey(0);
//
//	cvAdd ( FrameData->FG, FGTemp, FGTemp);
//	cvShowImage("Foreground", FGTemp);
// 			cvWaitKey(0);
// FIN PRUEBAS
	cvCopy( FGMask,FGTemp);

	if( Mask != NULL) {
		cvSetImageROI( Mask, Roi);
		cvCopy( FGTemp,Mask);
		cvResetImageROI( Mask);
	}

	resetearROIS( FrameData, Brillo );

	return flies;

}//Fin de la función

void CreateDataSegm( IplImage* Brillo ){
	CvSize size = cvSize(Brillo->width,Brillo->height); // get current frame size
	// crear imagenes si no se ha hecho o redimensionarlas si cambia su tamaño
	if( !IDif || IDif->width != size.width || IDif->height != size.height ) {
		    // CREAR IMAGENES
	        cvReleaseImage( &IDif );
	        cvReleaseImage( &IDifm );
	        cvReleaseImage( &pesos );

	        FGTemp = cvCreateImage(size, IPL_DEPTH_8U, 1);
	        IDif=cvCreateImage(size, IPL_DEPTH_8U, 1); // imagen diferencia abs(I(pi)-u(p(i))
	        IDifm=cvCreateImage(size, IPL_DEPTH_8U, 1);// IDif en punto flotante
	        pesos=cvCreateImage(size, IPL_DEPTH_8U, 1);//Imagen resultado wi ( pesos)
	        FGMask=cvCreateImage(size, IPL_DEPTH_8U, 1);// Mascara de fg con elipses rellenas

			// CREAR MATRICES

			vector_u=cvCreateMat(2,1,CV_32FC1); // Matriz de medias
			vector_resta=cvCreateMat(2,1,CV_32FC1); // (pi-u)
			matrix_mul=cvCreateMat(2,2,CV_32FC1);// Matriz (pi-u)(pi-u)T
			MATRIX_C=cvCreateMat(matrix_mul->rows,matrix_mul->cols,CV_32FC1);// MATRIZ DE COVARIANZA
			evects=cvCreateMat(2,2,CV_32FC1);// Matriz de EigenVectores
			evals=cvCreateMat(2,2,CV_32FC1);// Matriz de EigenValores

			Diagonal=cvCreateMat(2,2,CV_32FC1); // Matriz Diagonal,donde se extraen los ejes
			R=cvCreateMat(2,2,CV_32FC1);// Matriz EigenVectores.
			RT=cvCreateMat(2,2,CV_32FC1);

			//Crear storage y secuencia de los contornos

			storage = cvCreateMemStorage();
	}
	first_contour=NULL;
	cvZero( FGTemp);
    cvZero( IDif);
    cvZero( IDifm);
    cvZero(pesos);
	cvZero( FGMask);
}

void ellipseFit( CvRect rect,IplImage* pesos, IplImage* mask,
		float *semiejemenor,float* semiejemayor,CvSize* axes,CvPoint* centro,float* tita ){

	CvScalar v;
	CvScalar d1,d2; // valores de la matriz diagonal de eigen valores,semiejes de la elipse.
	CvScalar r1,r2; // valores de la matriz de los eigen vectores, orientación.

	float z=0;  // parámetro para el cálculo de la matriz de covarianza

	// Inicializar matrices a 0
	cvZero( vector_u);
	cvZero( vector_resta);
	cvZero( matrix_mul);
	cvZero( MATRIX_C);
	cvZero( evects);
	cvZero( evals);
	cvZero( Diagonal);
	cvZero( R);
	cvZero( RT);


//		cvShowImage("Foreground", mask);
//		cvWaitKey(0);
	if (SHOW_SEGMENTATION_DATA == 1) {
		printf(" \n\nMatriz de distancia normalizada al background |I(p)-u(p)|/0(p)");
	}
	// Hallar Z y u={ux,uy}
	for (int y = rect.y; y< rect.y + rect.height; y++){
		uchar* ptr1 = (uchar*) ( mask->imageData + y*mask->widthStep + 1*rect.x);
		uchar* ptr2 = (uchar*) ( pesos->imageData + y*pesos->widthStep + 1*rect.x);
		if (SHOW_SEGMENTATION_DATA == 1) printf(" \n\n");

		for (int x = 0; x<rect.width; x++){
			if (SHOW_SEGMENTATION_DATA == 1) {
				if( ( y == rect.y) && ( x == 0) ){
					printf("\n Origen: ( %d , %d )\n\n",(x + rect.x),y);
				}
				if ( ptr1[x] == 255 ){
				printf("%d\t", ptr2[x]);
				}
				else printf("0\t");
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
	if (SHOW_SEGMENTATION_DATA == 1){
				printf( "\n\nSumatorio de pesos:\n Z = %f",z);
				printf( "\n Sumatorio (wi*pi) = [ %f , %f ]",
						CV_MAT_ELEM( *vector_u, float, 0,0 ),
						CV_MAT_ELEM( *vector_u, float, 1,0 ));
	}
	if ( z != 0) {
		cvConvertScale(vector_u, vector_u, 1/z,0); // vector de media {ux, uy}
	}
	else {
		error(5);
	}
	if (SHOW_SEGMENTATION_DATA == 1){
		printf("\n\nCentro (vector u)\n u = [ %f , %f ]\n",CV_MAT_ELEM( *vector_u, float, 0,0 ),CV_MAT_ELEM( *vector_u, float, 1,0 ));
	}

	for (int y = rect.y; y< rect.y + rect.height; y++){
		uchar* ptr1 = (uchar*) ( mask->imageData + y*mask->widthStep + 1*rect.x);
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

	*semiejemayor=2*(sqrt(d1.val[0]));
	*semiejemenor=2*(sqrt(d2.val[0]));
	*tita=atan(r2.val[0]/r1.val[0]);

	// Dibujar elipse

	//Eliminamos el blob

	for (int y = rect.y; y< rect.y + rect.height; y++){
				uchar* ptr1 = (uchar*) ( FGTemp->imageData + y*FGTemp->widthStep + 1*rect.x);
				for (int x= 0; x<rect.width; x++){
					ptr1[x] = 0;
				}
	}

	// Obtenemos el centro de la elipse

	cvResetImageROI( FGTemp ); // para evitar el offset de find contours
	cvResetImageROI( FGMask);


	*centro = cvPoint( cvRound( *((float*)CV_MAT_ELEM_PTR( *vector_u, 0, 0 )) ),
			cvRound( *((float*)CV_MAT_ELEM_PTR( *vector_u, 1, 0 )) ) );

	// Obtenemos los ejes y la orientacion en grados

	*axes = cvSize( cvRound(*semiejemayor) , cvRound(*semiejemenor) );
	*tita = ( (*tita) *180)/PI;

}





void ReleaseDataSegm( ){
	cvReleaseImage( &FGTemp);
	cvReleaseImage(&IDif);
	cvReleaseImage(&IDifm);
	cvReleaseImage(&pesos);
	cvReleaseImage(&FGMask);
	cvReleaseMemStorage( &storage);

	cvReleaseMat(&vector_resta);
	cvReleaseMat(&matrix_mul);
	cvReleaseMat(&MATRIX_C);
	cvReleaseMat(&evects);
	cvReleaseMat(&evals);
	cvReleaseMat(&Diagonal);
	cvReleaseMat(&R);
	cvReleaseMat(&RT);

}

void establecerROIS( STFrame* FrameData, IplImage* Brillo,CvRect Roi ){
	cvSetImageROI( Brillo , Roi);
	cvSetImageROI( FrameData->BGModel, Roi );
	cvSetImageROI( FrameData->IDesv, Roi );
	cvSetImageROI( FrameData->FG, Roi );
	cvSetImageROI( FGTemp, Roi );

	cvSetImageROI( IDif, Roi );
	cvSetImageROI( IDifm, Roi );
	cvSetImageROI( pesos, Roi );
	cvSetImageROI( FGMask, Roi );

}

void resetearROIS( STFrame* FrameData, IplImage* Brillo ){
	cvResetImageROI( Brillo );
	cvResetImageROI( FrameData->BGModel );
	cvResetImageROI( FrameData->IDesv );
	cvResetImageROI( FrameData->FG );
	cvResetImageROI( FGTemp );

	cvResetImageROI( IDif );
	cvResetImageROI( IDifm);
	cvResetImageROI( pesos );
	cvResetImageROI( FGMask);
}
