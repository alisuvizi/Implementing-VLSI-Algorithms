/**
	*********************************************************
	In this file, basic functions of parser are implemented.
	*********************************************************
**/

#include <stdlib.h>
#include <string.h>
#include <placer.h>
#include <stdio.h>
#include <math.h>

#include <iostream>
#include <list>
#include <vector>
#include <map>

using namespace std;
#include <atlasDB.h>

unsigned layoutH, layoutW, cellHeight;

#define SRC 101
#define SNK 102

#define __whiteSpace 1.1

#define __whiteSpace 1.1
#define DBU 2000

//-----------------------------------------------------------
// Constructors
//-----------------------------------------------------------
SimplePlacer::SimplePlacer(float T0, float alpha, float Tmin, int inner_count)
{
	this->T0 = T0;
	this->alpha = alpha;
	this->Tmin = Tmin;
	this->inner_count = inner_count;
	mList = new vector <simpleInstance *>;
	nList = new vector <simpleNet *>;
}

//-----------------------------------------------------------
// Destructors
//-----------------------------------------------------------
SimplePlacer::~SimplePlacer()
{
}

//-----------------------------------------------------------
// Read DB information
//-----------------------------------------------------------
int SimplePlacer::readDB ( atlasDB *db )
{
	TInstData *dbInst = new TInstData;
	vector <simpleInstance *>::iterator iter;
	simpleInstance *anInst;
	TNetData *dbNet = new TNetData;
	TPinData *dbPin = new TPinData;
	TPointData *pinXY = new TPointData;
	float layoutArea = 0, Dim, modDim;
	int cnt;
	printf ( "\tReading %d cells ... \n", db->getInstancesNum() );
	//------------------
	// Reading Instances
	//------------------
	db->resetInstListPos();
	cnt = 0;
	while ( db->getInstData ( dbInst ) == true )
	{
		//--------------------------
		// Creating a new instance
		//--------------------------
		anInst = new simpleInstance;
		anInst->rowNo = 0;
		anInst -> TL.x = 0;
		anInst -> TL.y = 0;
		anInst -> InstID = cnt + 1;
		anInst -> InstName = new char[100];
		strcpy ( anInst -> InstName, dbInst->instName.c_str() );
		anInst -> CellName = new char[100];
		strcpy ( anInst -> CellName, dbInst->macroName.c_str() );
		anInst -> Width = dbInst->width;
		anInst -> Height = dbInst->height;
		rowHeight = anInst->Height;
		mList->push_back ( anInst );
		db->instGoForward();
		cnt ++;
	}
	cellHeight = rowHeight;

	printf ( "\tReading and elaborating %d nets ... ", db->getNetsNum() );
	unsigned nn = 0;
	db->resetNetListPos();
	while ( db->getNetData ( dbNet ) == true )
	{
		simpleNet *aNet;
		aNet = new simpleNet;
		aNet -> netName = new char[100];
		aNet->instances = new list <struct simpleInstance *>;
		aNet->srcTerminal = NULL;
		strcpy ( aNet -> netName, dbNet -> name . c_str() );
/*		if ( ( nn%100 ) == 0 )
			printf ( "\t\t%d / %d\n", nn, db->getNetsNum() );*/
		nn ++;
		if ( dbNet -> name == "POWR" )
		{
			printf ( "\t\tNet Power is ignored.\n" );
			db->NetDataGoForward();
			continue;
		}
		if ( dbNet -> name == "GRND" )
		{
			printf ( "\t\tNet Ground is ignored.\n" );
			db->NetDataGoForward();
			continue;
		}
		db->resetPinOfNetListPos();
		while ( db->getPinOfNetData ( dbPin ) == true )
		{
			db->getPinParentPos ( pinXY );
			db->getPinParent ( dbInst );
			simpleInstance *anInst =  findInstance ( dbInst->instName );
			if ( anInst == NULL )
			{
				printf ( "Netlist annotation error.\n" );
			}
			else
			{
				aNet->instances->push_back ( anInst );
				if ( dbPin->dir == output )
					aNet->srcTerminal = anInst;
			}
			db->PinOfNetGoForward();
		}
		nList->push_back ( aNet );
		db->NetDataGoForward();
	}
	printf ( "OK.\n" );

	//----------------------------
	// Estimate layout dimension
	//----------------------------
	for ( iter = mList->begin(); iter != mList->end(); iter ++ )
	{
		anInst = ( simpleInstance * ) ( *iter );
		layoutArea += anInst->Width * anInst->Height;
	}

	Dim = floor ( sqrt ( layoutArea * __whiteSpace ) );
	modDim = Dim - ( int ) Dim % ( int ) rowHeight;
	maxlayoutW = ( unsigned ) ( modDim );
	maxlayoutH = ( unsigned ) ( modDim );
	printf ( "\tRead DB: %d insts are read.\n", mList->size() );
	printf ( "\tRead DB: %d nets are read.\n" , nList->size() );
	
	return 0;
}

//-----------------------------------
// Read Placed Netlist from file
//-----------------------------------
void SimplePlacer::readDump(char *dumpFile)
{
	FILE *fp;
	char iName[64];
	float iLeft, iTop;
	unsigned int cnt = 0, iNumber = 0; 
	
	fp = fopen(dumpFile, "r+t");
	if (fp == NULL)
	{
	    printf("Cannot read dump file %s\n", dumpFile);
	    exit(1);
	}

	fscanf(fp,"%d %d %d %d\n", &iNumber, &maxlayoutW, &maxlayoutH, &rowNumber);
// 	printf("#cells = %d, W = %d, H = %d, Row number = %d\n", iNumber, maxlayoutW, maxlayoutH, rowNumber);

	for(unsigned i = 0;i < iNumber;i ++)
	{
	    fscanf(fp,"%s %f %f \n",iName, &iLeft, &iTop);
// 	    printf("i = %d, name = %s , x = %f , y = %f \n",i, iName, iLeft, iTop);
	    simpleInstance *iPtr = findInstance(iName);
	    if (iPtr == NULL)
	    {
		printf("Instance %s is not found in instance list.\n");
		exit(1);
	    }
	    else
	    {
		iPtr -> TL.x = iLeft;
		iPtr -> TL.y = iTop;
		cnt ++;
	    }
	}
	printf("%d in stances are loaded from dump\n", cnt);
}

simpleInstance *SimplePlacer::findInstance ( string instName )
{
	vector <simpleInstance *>::iterator iter;
	simpleInstance *anInst;

	for ( iter = mList->begin(); iter != mList->end(); iter ++ )
	{
		anInst = ( simpleInstance * ) ( * iter );
		if ( strcmp ( anInst-> InstName, instName.c_str() ) == 0 )
			return anInst;
	}
	return NULL;
}

int SimplePlacer::simPlacer()
{
	vector <simpleInstance *>::iterator iter;
	simpleInstance *anInst;
	int rowNo, i;
	float rowWidth;
	
	cout << "[-] Running Simple Placer as random placement..." << endl;
	
	rowNo = 0;
	rowWidth = 0;
	for ( iter = mList->begin(); iter != mList->end(); iter ++ )
	{
		anInst = ( simpleInstance * ) ( * iter );
		if ( rowWidth < maxlayoutW )
		{
			anInst->rowNo = rowNo;
			anInst->TL.x = rowWidth;
			anInst->TL.y = rowNo * rowHeight;
			rowWidth += anInst->Width;
		}
		else
		{
			rowNo ++;
			rowWidth = 0;
			anInst->rowNo = rowNo;
			anInst->TL.x = rowWidth;
			anInst->TL.y = rowNo * rowHeight;
		}
	}
	rowNumber = rowNo;
	layoutW = maxlayoutW;
	layoutH = maxlayoutH;
	
	double cost = THPWL();
	cout << "[-] Number of Cells: " << mList->size() << endl;
	cout << "[-] Number of Rows: " << rowNumber << endl;
	cout << "[-] Max Layout Width: " << maxlayoutW << endl;
	cout << "[-] Max Layout Height: " << maxlayoutH << endl;
	cout << "[-] Initial THPWL is " << cost << " um" << endl;
	return cost;
}

void SimplePlacer::Perturb(bool rollback = false){
	simpleInstance *inst_i, *inst_j;
	int temp_rowNo;
	simplePoint temp_TL;
	static size_t cellNumber = mList->size();
	static int cell_i = 0, cell_j = 0;

	if (!rollback){
		srand(time(NULL));
		do{
			cell_i = rand() % cellNumber;
			cell_j = rand() % cellNumber;
		}while(cell_i == cell_j);
	}
	
	inst_i = (*mList)[cell_i];
	inst_j = (*mList)[cell_j];

	temp_rowNo = inst_i->rowNo;
	temp_TL.x = inst_i->TL.x;
	temp_TL.y = inst_i->TL.y;
	
	inst_i->rowNo = inst_j->rowNo;
	inst_i->TL.x = inst_j->TL.x;
	inst_i->TL.y = inst_j->TL.y;
	
	inst_j->rowNo = temp_rowNo;
	inst_j->TL.x = temp_TL.x;
	inst_j->TL.y = temp_TL.y;
}

void SimplePlacer::Legalizer(){
	vector <simpleInstance *>::iterator iter;
	simpleInstance *anInst;
	for (int i = 0; i < rowNumber; i ++){
		float rowWidth = 0.0f;
		vector <bool> movedCell (mList->size());		
		for(int k = 0; k < movedCell.size(); k++){
			movedCell[k] = false;
		}
		simpleInstance * minRowInst;
		int minCellNo;
		int cellNo;
		do{
			minRowInst = NULL;
			minCellNo = -1;
			cellNo = 0;
			for (iter = mList->begin(); iter != mList->end(); iter ++ )
			{
				anInst = ( simpleInstance * ) ( * iter );
				if ( anInst->rowNo == i 
				&& movedCell[cellNo] == false
				&& (minRowInst == NULL || anInst->TL.x < minRowInst->TL.x)) 
				{
					minRowInst = anInst;
					minCellNo = cellNo;
				}
				cellNo++;
			}
			// Exit condition: if no more cells could be found.
			if (minCellNo < 0){
				break;
			}
			minRowInst->TL.x = rowWidth;
			rowWidth += minRowInst->Width;
			movedCell[minCellNo] = true;
		}
		while(true);
		if (rowWidth > layoutW){
			layoutW = rowWidth;
		}
	}
}

int SimplePlacer::SAplacer()
{
	// Constant values
	const double T0 = this->T0;
	const double Tmin = this->Tmin;
	const double a = this->alpha;
	//static size_t cellNumber = mList->size();
	// Seed to psedu-random number generator.
	srand(time(NULL));
	// Initial Placement (is legal)
	simPlacer();
	double cost = THPWL();
	// Initial Value of T
	double T = T0;
	while ( T > Tmin ){
		int count = this->inner_count;//1;//cellNumber;
		while (count --> 0){
			Perturb();
			double newCost = THPWL();
			double deltaCost = newCost - cost;
			if (deltaCost < 0){
			// Accept
			cost = newCost;
			}else{
				// A random number [0, 1) 
				double r = double(rand() % RAND_MAX) / RAND_MAX;
				if (r < exp(-deltaCost/T)){
					// Accept
					cost = newCost;
				}else{
					// Reject (= Rollback)
					Perturb(true);
				}
			}
		}
		T *= a;
	}
	Legalizer();
	cout << "[-] Final THPWL is " << THPWL() << " um" << endl;
	/**/
	return 1;
}

void SimplePlacer::updateDB ( atlasDB *db )
{
	vector <simpleInstance *>::iterator iter;
	simpleInstance *iPtr;
	unsigned maxX = 0, maxY = 0;;
	int i;
	int totalCellNo = 0;

	for ( iter = mList->begin(); iter != mList->end(); iter ++ )
	{
		iPtr = ( simpleInstance * ) ( *iter );

		if ( db->setInstTopLeft ( iPtr->InstName, ( long int ) ( iPtr->TL.y ), ( long int ) ( iPtr->TL.x ) ) == false )
		{
			printf ( "Update DB violation!\n" );
			exit ( 1 );
		}
		totalCellNo ++;
		if ( ( iPtr->TL.x + iPtr->Width ) > maxX )
			maxX = iPtr->TL.x + iPtr->Width;

		if ( ( iPtr->TL.y + iPtr->Height ) > maxY )
			maxY = iPtr->TL.y + iPtr->Height;

		maxlayoutW = maxX;
		maxlayoutH = maxY;
	}

	printf ( "\tUpdate: %d instances are updated in DB\n", totalCellNo );
	printf ( "\tUpdate: layout area: w = %d , h = %d\n", maxlayoutW, maxlayoutH );

	//	set layout dimensions
	db->setNetlistPlaceResults ( ( long int ) ( maxX ), ( long int ) ( maxY ), rowNumber, ( long int ) ( rowHeight ) );
}

void SimplePlacer::dumpPlacement ( char *dumpFile )
{
	vector <simpleInstance *>::iterator iter1;
	simpleInstance *anInst;
	FILE *fp;
    

	fp = fopen(dumpFile, "w+t");
	if (fp == NULL)
	{
	    printf("Cannot write dump file %s\n", dumpFile);
	    exit(1);
	}
	fprintf(fp,"%d %d %d %d\n",mList->size(), (unsigned int) (maxlayoutW), (unsigned int)(maxlayoutH), rowNumber);

	for ( iter1 = mList->begin();iter1 != mList->end();iter1 ++ )
	{
	    anInst = ( simpleInstance * ) ( * iter1 );
	    fprintf(fp,"%s %f %f\n", anInst->InstName, (float)(anInst->TL.x), (float)(anInst->TL.y) );
	}
	fclose(fp);
}


double SimplePlacer::THPWL()
{
	vector <simpleNet *>::iterator iter1;
	list <simpleInstance *>::iterator iter2;
	simpleNet *aNet;
	simpleInstance *anInst;
	long int wl, minx, miny, maxx, maxy;
	double THPWL = 0;

	for ( iter1 = nList->begin();iter1 != nList->end();iter1 ++ )
	{
		aNet = ( simpleNet * ) ( * iter1 );
		wl = 0;
		minx = 0x7FFFFFFF;
		miny = 0x7FFFFFFF;
		maxx = -1;
		maxy = -1;
		for ( iter2 = aNet->instances->begin();iter2 != aNet->instances->end();iter2 ++ )
		{
			anInst = ( simpleInstance * ) ( * iter2 );
			if ( anInst->TL.x < minx )
				minx = anInst->TL.x;
			if ( anInst->TL.x > maxx )
				maxx = anInst->TL.x;
			if ( anInst->TL.y < miny )
				miny = anInst->TL.y;
			if ( anInst->TL.y > maxy )
				maxy = anInst->TL.y;
			wl += ( maxy - miny ) + ( maxx - minx );
		}
		THPWL += wl;
	}
	THPWL /= DBU;
// 	printf("\t\tTotal half perimeter wirelength = %lf um\n", THPWL);
	return THPWL;
}


