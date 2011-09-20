/*
 * segmentacion2.cpp
 *
 *  Created on: 13/09/2011
 *      Author: german
 */


#include "segmentacion.h"
#include "math.h"


void segmentacion(IplImage *Brillo,IplImage *mediana,IplImage *desviacion,IplImage *Foreg,CvRect Segroi){



	// CREAR IMAGENES
	CvSize size = cvSize(Brillo->width,Brillo->height); // get current frame size

	if( !IDif || IDif->width != size.width || IDif->height != size.height ) {

	        cvReleaseImage( &IDif );
	        cvReleaseImage( &IDifm );
	        cvReleaseImage( &pesos );
	        cvReleaseImage( &FGTemp );

	        IDif=cvCreateImage(cvSize(mediana->width,mediana->height), IPL_DEPTH_8U, 1); // imagen diferencia abs(I(pi)-u(p(i))
	        IDifm=cvCreateImage(cvSize(mediana->width,mediana->height), IPL_DEPTH_8U, 1);// IDif en punto flotante
	        pesos=cvCreateImage(cvSize(mediana->width,mediana->height), IPL_DEPTH_8U, 1);//Imagen resultado wi ( pesos)
	        FGTemp=cvCreateImage(cvSize(mediana->width,mediana->height), IPL_DEPTH_8U, 1);// Imagen copia del foreground

	}

	cvCopy(Foreg,FGTemp);

	CvScalar w; // valor del brillo pixel w
	CvScalar v;
	CvScalar d1,d2; // valores de la matriz diagonal de eigen valores, ejes de la elipse.
	CvScalar r1,r2; // valores de la matriz de los eigen vectores, orientación.

	//Crear storage y secuencia de los contornos

	CvMemStorage* storage = cvCreateMemStorage();
	CvSeq* first_contour=NULL;

	cvAbsDiff(Brillo,mediana,IDif);// |I(p)-u(p)|/0(p)
	cvConvertScale(IDif ,IDifm,1,0);// A float
	cvDiv(IDifm,desviacion,pesos);// Calcular wi

	//Buscamos los contornos de las moscas en movimiento en el foreground

	int Nc = cvFindContours(FGTemp,storage,&first_contour,sizeof(CvContour),CV_RETR_EXTERNAL);
	printf( "\n Total Contornos Detectados: %d\n", Nc );

	for( CvSeq *c=first_contour; c!=NULL; c=c->h_next) {

		float z=0;
		float zx=0;
		float zy=0;

		float ejemenor; // eje menor
		float ejemayor; // eje mayor
		float tita; // orientación


		CvScalar p; // valor del peso del pixel p

		CvRect rect=cvBoundingRect(c,0); // Hallar los rectangulos para establecer las ROIs
		Segroi=cvRect(rect.x,rect.y,rect.width,rect.height);

		cvSetImageROI( FGTemp, Segroi );
		cvSetImageROI( pesos,Segroi);

		// CREAR MATRICES

		float mult[2][1];
		int posiciones[2][1];
		float vector_wp[2][1];

		float matrix_covar11=0;
		float matrix_covar12=0;
		float matrix_covar21=0;
		float matrix_covar22=0;

		CvMat *vector_p=cvCreateMat(2,1,CV_32FC1); // Matriz de posiciones
		CvMat *vector_u=cvCreateMat(2,1,CV_32FC1); // Matriz de medias
		CvMat *vector_resta=cvCreateMat(2,1,CV_32FC1); //Matriz (pi-u)
		CvMat *vector_tras=cvCreateMat(1,2,CV_32FC1);// Matriz  de la traspuesta (pi-u)T
		CvMat *matrix_mul=cvCreateMat(vector_resta->rows,vector_tras->cols,CV_32FC1);// Matriz (pi-u)(pi-u)T
		CvMat *MATRIX_C=cvCreateMat(matrix_mul->rows,matrix_mul->cols,CV_32FC1);// MATRIZ DE COVARIANZA
		CvMat *evects=cvCreateMat(2,2,CV_32FC1);// Matriz de EigenVectores
		CvMat *evals=cvCreateMat(2,2,CV_32FC1);// Matriz de EigenValores

		CvMat *Diagonal=cvCreateMat(2,2,CV_32FC1); // Matriz Diagonal,donde se extraen los ejes
		CvMat *R=cvCreateMat(2,2,CV_32FC1);// Matriz EigenVectores.
		CvMat *RT=cvCreateMat(2,2,CV_32FC1);


		CvScalar elemento11;
		CvScalar elemento12;
		CvScalar elemento21;
		CvScalar elemento22;


		// Hallar Z

		for (int x= 0; x<pesos->roi->height; x++){ // Recorrer la Roi
			for (int y= 0; y<pesos->roi->width; y++){

				w=cvGet2D(pesos,x,y); // Sacar el valor del brillo de cada pixel
				z=z + w.val[0]; // Sumatorio Z, Parametro Z

				posiciones[0][0]=x;
				posiciones[1][0]=y;

				mult[0][0]=posiciones[0][0]*w.val[0];
				mult[1][0]=posiciones[1][0]*w.val[0];

				zx=zx+mult[0][0];
				zy=zy+mult[1][0];

				vector_wp[0][0]=zx;
				vector_wp[1][0]=zy;

			}
		}

		for(int i=0;i<2;i++){
			for(int j=0;j<1;j++){


				float elemento_p=posiciones[i][j];
				*((float*)CV_MAT_ELEM_PTR( *vector_p, i, j ))=elemento_p; // vector p

				float elemento=(vector_wp[i][j]/z);
				*((float*)CV_MAT_ELEM_PTR( *vector_u, i, j ))=elemento; // vector u = (u1,u2)T
			}
		}

		cvSub(vector_p,vector_u,vector_resta);//Resta de matrices, (pi-u)

		cvGEMM(vector_resta, vector_resta,1, NULL, 0, matrix_mul,CV_GEMM_B_T);// Multiplicar Matrices (pi-u)(pi-u)T

		elemento11=cvGet2D(matrix_mul,0,0);
		elemento12=cvGet2D(matrix_mul,0,1);
		elemento21=cvGet2D(matrix_mul,1,0);
		elemento22=cvGet2D(matrix_mul,1,1);

		for (int X= 0; X<pesos->roi->height; X++){ // sumatorio wi(pi-u)(pi-u)T
			for (int Y= 0; Y<pesos->roi->width; Y++){

			p=cvGet2D(pesos,X,Y);

			matrix_covar11=matrix_covar11+(elemento11.val[0]*p.val[0]);//elemento 11 de la matriz de covarianza.
			matrix_covar12=matrix_covar12+(elemento12.val[0]*p.val[0]);//idem
			matrix_covar21=matrix_covar21+(elemento21.val[0]*p.val[0]);//idem
			matrix_covar22=matrix_covar22+(elemento22.val[0]*p.val[0]);//idem

		}
			}

		//Establecer la matriz de covarianza como estructura CVMat

		*( (float*)CV_MAT_ELEM_PTR( *MATRIX_C, 0, 0 ) ) = matrix_covar11/z;
		*( (float*)CV_MAT_ELEM_PTR( *MATRIX_C, 0, 1 ) ) = matrix_covar12/z;
		*( (float*)CV_MAT_ELEM_PTR( *MATRIX_C, 1, 0 ) ) = matrix_covar21/z;
		*( (float*)CV_MAT_ELEM_PTR( *MATRIX_C, 1, 1 ) ) = matrix_covar22/z;


		// EXTRAER LOS EIGEN VALORES Y EIGEN VECTORES

		cvEigenVV(MATRIX_C,evects,evals,2);// Hallar los EigenVectores
		cvSVD(MATRIX_C,Diagonal,R,RT,0); // Hallar lo EigenValores, MATRIX_C=R*Diagonal*RT


		// Mostrar matriz de covarianza

		printf("\n*****************************************");
				for(int i=0;i<2;i++){
					printf("\n");
					for(int j=0;j<2;j++){
						v=cvGet2D(MATRIX_C,i,j);
						printf("\t%f",v.val[0]);
					}
				}

		//Extraer valores de los EigenVectores y EigenValores

		d1=cvGet2D(Diagonal,0,0);
		d2=cvGet2D(Diagonal,1,1);
		r1=cvGet2D(R,0,0);
		r2=cvGet2D(R,0,1);

		//Hallar los ejes y la orientación

		ejemayor=2*(sqrt(d1.val[0]));
		ejemenor=2*(sqrt(d2.val[0]));
		tita=atan(r2.val[0]/r1.val[0]);

		printf("\n EJE MAYOR : %f EJE MENOR: %f ORIENTACION: %f",ejemayor,ejemenor,tita);

		cvResetImageROI(FGTemp);
		cvResetImageROI(pesos);

		cvReleaseMat(&vector_p);
		cvReleaseMat(&vector_resta);
		cvReleaseMat(&vector_tras);
		cvReleaseMat(&matrix_mul);
		cvReleaseMat(&MATRIX_C);
		cvReleaseMat(&evects);
		cvReleaseMat(&evals);
		cvReleaseMat(&Diagonal);
		cvReleaseMat(&R);
		cvReleaseMat(&RT);





	}// Fin de contornos

	cvReleaseImage(&IDif);
	cvReleaseImage(&IDifm);
	cvReleaseImage(&pesos);

}//Fin de la función


