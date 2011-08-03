



#include <BlobResult.h>
#include <Blob.h>
#include "Interfaz_Lista.h"

//Estructura de la mosca

typedef struct Moscas{

		int etiqueta;
		float velocidad;
		float area;
		float VV,VH;
		CvPoint moment[8000];
		CvPoint2D32f punto1,punto2;

}STMoscas;

//FUNCION PRINCIPAL//

CBlobResult blobs;
CBlob *currentBlob;

int CreateBlobs(IplImage* ROI,IplImage* blobs_img, STMoscas **Mosca, Lista puntero ) {

	blobs = CBlobResult( ROI, NULL, 100, true );

	//Obtener los Blobs y excluir aquellos que no interesan por su tamaño

	blobs = CBlobResult( ROI, NULL, 100, true );
	blobs.Filter( blobs, B_EXCLUDE, CBlobGetArea(),B_GREATER,100);
	blobs.Filter( blobs, B_EXCLUDE, CBlobGetPerimeter(),B_GREATER,1000);

	int j = blobs.GetNumBlobs();
	printf("%d\n",j);

	// Guardar los Blobs en un archivo txt (OPCIONAL)

	blobs.PrintBlobs( " blobs.txt" );

	//Recorrer Blob a blob y obtener las caracteristicas de cada uno de ellos

	for (int i = 0; i < blobs.GetNumBlobs(); i++ ){

		currentBlob = blobs.GetBlob(i);

		// Extraer las esquinas de los rectangulos que encierran a cada Blob

		CvPoint2D32f p1,p2,p3,p4;

		p1.x=currentBlob->minx;
		p1.y=currentBlob->miny;

		p2.x=currentBlob->maxx;
		p2.y=currentBlob->maxy;

		p3.x=currentBlob->miny;
		p3.y=currentBlob->maxy;

		p4.x=currentBlob->maxx;
		p4.y=currentBlob->maxy;

		//Calcular los size para los ROIS y crear los ROIS

		float valor_vertical,valor_horizontal;

		valor_vertical=abs(p3.y-p1.y);
		valor_horizontal=abs(p3.x-p2.x);

		//Calcular el momento del blob

		CvPoint2D32f m;

		m.x=blobs.GetNumber(i,CBlobGetMoment(1,0));
		m.y=blobs.GetNumber(i,CBlobGetMoment(0,1));

	// METER LOS VALORES EN LA LISTA //

		Mosca = (STMoscas **)malloc(sizeof(STMoscas*)); // Reservar memoria para la lista
		if(!Mosca) {
			error(4);
			return 0;
		}

		// Leer los datos y añadirlos a la lista

		Mosca[i]->punto1.x=p1.x;
		Mosca[i]->punto1.y=p1.y;
		Mosca[i]->punto2.x=p2.x;
		Mosca[i]->punto2.y=p2.y;
		Mosca[i]->VV=valor_vertical;
		Mosca[i]->VH=valor_horizontal;

		// Añadir los parametros de la Mosca ( Blob) a la lista

		int ok = anyadirAlFinal(Mosca, &puntero);
		if (!ok) return 0;
	}

	//Mostrar los valores almcenados en la lista

	printf("\nLista:\n");
//	mostrarLista(puntero);

return 1;
}
