/*
 * Errores.cpp
 *
 *  Created on: 04/07/2011
 *      Author: chao
 */
#include <stdio.h>


void DeallocateImages( void );
void DestroyWindows( );

void error(int err){
	switch( err ){
		case 1:
			fprintf( stderr, "ERROR: No se puede abrir el video. Esto puede suceder"
					"por:\n - El formato de video no está soportado.\n - Los códecs"
					"no están correctamente instalados.\n - El video está corrupto.\n");
			DestroyWindows( );
			break;
		case 2:
			fprintf( stderr, "ERROR: frame is null...Vídeo corrupto.\n" ); //getchar();
			DestroyWindows( );
			break;
		case 3:

			fprintf( stderr, "ERROR: No se ha encontrado el plato \n" );
			fprintf( stderr, "Puede ser debido a:\n "
					"- Tipo de video cargado incorrecto "
					"- Iluminación deficiente \n"
					"- Iluminación oblicua que genera sombras pronunciadas\n"
					"- Plato  fuera del área de visualización\n" );
			getchar();
			DestroyWindows( );
			break;
		case 4:
			fprintf( stderr, "ERROR: Memoria insuficiente\n" );
			DestroyWindows( );
			break;
		case 5:
			fprintf( stderr, "ERROR: División por 0 \n" );
			DestroyWindows( );
			break;
		case 6:
			fprintf( stderr, "ERROR: Fallo al guardar datos 0 \n" );
			DestroyWindows( );
			break;
		case 7:
			fprintf( stderr, "ERROR: Fallo al liberar buffer 0 \n" );
	}
}
