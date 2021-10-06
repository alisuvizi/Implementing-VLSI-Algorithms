#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "gui.h"
#include <stdlib.h>
#include <cstdlib>
#include <stdio.h>
#include <curses.h>
#include <qdatetime.h>

#include "iostream"
#include "list"
using namespace std;

#include "atlasLEF.h"
#include "atlasDEF.h"
#include "placer.h"
#include "gr.h"
#include "atlasDB.h"

void runGUI ( int argc, char **argv );
void runAlgorithms ( int argc, char **argv );

extern bool doPaint;

atlasDB *db;
atlasLEF *lef_parser;
atlasDEF *def_parser;
SimplePlacer *sPL;
simpleGlobalRouter *sGR;


int main ( int argc, char **argv )
{
	runAlgorithms ( argc, argv );
	runGUI ( argc, argv );

	return EXIT_SUCCESS;

}

void runAlgorithms ( int argc, char **argv )
{

	if ( argc == 6 )
	{
		db = new atlasDB;
		lef_parser = new atlasLEF();
		def_parser = new atlasDEF();

		system ( "clear" );

		QTime t1, t2;

		t1 = QTime::currentTime();
		string s1 = QString ( "AET started at %1" ).arg ( t1.toString ( "hh:mm:ss" ) );
		printf ( "%s\n", s1.c_str() );

		printf ( "1. LEF parser.\n" );
		lef_parser->parse ( ( char * ) argv[1], ( char * ) "lef_out.txt", db );

		printf ( "2. DEF parser.\n" );
		def_parser->parse ( ( char * ) argv[2], ( char * ) "Def_out.txt", db );
		printf("argv[3] = %s\n", argv[3]);
		if (strcmp(argv[3],"-place") == 0)
		{
		    printf ( "3. Simple placer and dump placed cells.\n" );
		    sPL = new SimplePlacer();

		    //Read fixedPoint Data
		    sPL->readfixedPoints(argv[5]);

		    // Read information from DB
		    sPL->readDB ( db );
		    // Do Placement
		    sPL->simPlacer();
		    // Write placement results to DB
		    sPL->updateDB ( db );
		    sPL->dumpPlacement(argv[4]);

		    printf ( "\n\nTotal Area:%g \n\n", sPL->CostFunction()  );
	            printf ( "\n\nTotal Wire Lenght:%g \n\n", sPL->THPWL()  );
	            //printf("\t\tTotal half perimeter wirelength = %lf um\n", sPL->THPWL());
		    
		    printf ( "4. Simple global router.\n" );
		    sGR = new simpleGlobalRouter ( db );
		    TNetlistData *nData = new TNetlistData;
		    db -> getNetlistPlaceResults ( nData );
		    sGR -> createGrMesh ( 10, 10, nData->Width, nData->Height );
		    sGR -> simpleGrRoute ();
		    sGR -> reportCapacityViolations();
		}
		else if (strcmp(argv[3], "-load") == 0)
		{
		    printf ( "3. Read placed cells from dumped file.\n" );
		    sPL = new SimplePlacer();
		    // Read information from DB
		    sPL->readDB ( db );
		    // Do Placement
		    sPL->readDump(argv[4]);

		    // Write placement results to DB
		    sPL->updateDB ( db );

		    printf ( "4. Simple global router.\n" );
		    sGR = new simpleGlobalRouter ( db );
		    TNetlistData *nData = new TNetlistData;
		    db -> getNetlistPlaceResults ( nData );
		    sGR -> createGrMesh ( 10, 10, nData->Width, nData->Height );
		    sGR -> simpleGrRoute ();
		    sGR -> reportCapacityViolations();
		}
		t2 = QTime::currentTime();
		string s3 = QString ( "Simple global router finished at %1" ).arg ( t2.toString ( "hh:mm:ss" ) );
		printf ( "%s\n", s3.c_str() );

		printf ( "5. Ready to view GUI\n" );
		doPaint = true;
	}
	else
	{
		printf ( "\nAtlas Evaluation Toolkit, Version 4.0,\n" );
		printf ( "Copyright AUT-EDA Group,\n" );
		printf ( "Ali Jahanian, April 2009.\n" );
		printf ( "Please send bug reports to jahanian@sbu.ac.ir; see Readme.txt for details.\n\n" );

		printf ( "Usage:./aet [LEF file] [DEF file]\n\n\n" );
		printf ( "[LEF file]: name and path of LEF file which is physical technology library.\n" );
		printf ( "[DEF file]: name and path of DEF file which is synthesized design.\n\n\n" );
		exit ( -1 );
	}
}

void runGUI ( int argc, char **argv )
{
	QApplication::setColorSpec ( QApplication::CustomColor );
	QApplication a ( argc, argv );
	MyWidget w;
	w.setCaption ( "Atlas Evaluation Toolkit, Version 4.0 (Revised for QIAU-MS students)" );
	w.setGeometry ( QRect ( 0,0,1024,762 ) );
	a.setMainWidget ( &w );
	w.show();
	a.exec();
}

