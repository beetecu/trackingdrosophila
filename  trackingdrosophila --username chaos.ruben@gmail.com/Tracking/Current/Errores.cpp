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
			fprintf( stderr, "ERROR: capture is NULL \n" );	//getchar();
			DestroyWindows( );
		case 2:
			fprintf( stderr, "ERROR: frame is null...\n" ); //getchar();
			DestroyWindows( );
		case 3:

			fprintf( stderr, "ERROR: No se ha encontrado el plato \n" );
			fprintf( stderr, "Puede ser debido a:\n "
					"- Tipo de video cargado incorrecto "
					"- Iluminación deficiente \n"
					"- Iluminación oblicua que genera sombras pronunciadas\n"
					"- Plato  fuera del área de visualización\n" );
			getchar();
			DestroyWindows( );
		case 4:
			fprintf( stderr, "ERROR: Memoria insuficiente\n" );
			DestroyWindows( );

	}
}
