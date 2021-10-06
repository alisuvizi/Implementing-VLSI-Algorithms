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
SimplePlacer::SimplePlacer()
{
	mList = new list <simpleInstance *>;
	nList = new list <simpleNet *>;
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
	list <simpleInstance *>::iterator iter;
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
	list <simpleInstance *>::iterator iter;
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
	list <simpleInstance *>::iterator iter_cell;
	list <simpleInstance *>::iterator iter_cell_1;
	list <simpleInstance *>::iterator iter_cell_2;
	list <simpleNet *>::iterator iter_net;
	list <simpleNet *> *nets = new list <simpleNet *>;
	simpleInstance *anInst = new simpleInstance;
	simpleInstance *maxInst = new simpleInstance;
	simpleInstance *Ins = new simpleInstance;
	simpleInstance *Ist = new simpleInstance;
	simpleInstance *neighbor = new simpleInstance;
	simpleInstance *neighbor_1 = new simpleInstance;
	simpleNet *aNet = new simpleNet;
	simpleNet *maxNet = new simpleNet;
	int rowNo;
	int num_cell;
	float rowWidth;
	float maxWidth;
	float lLeft, lRight;
	
	rowNo = 0;
	num_cell = 0;
	lLeft = 0;
	lRight = 0;
	
	for ( iter_cell = mList->begin(); iter_cell != mList->end(); iter_cell ++ ) {
		anInst = ( simpleInstance * ) ( * iter_cell );
		anInst->flag = 0;
		anInst->nflag = 0;
		anInst->LR = -1;
	}
	
	while ( num_cell < mList->size() ) {
		list <simpleInstance *>::iterator iter_row;
		list <simpleInstance *> *row = new list <simpleInstance *>;
		
		rowWidth = 0;
		maxWidth = 0;
		
		//finding the biggest cell
		for ( iter_cell = mList->begin(); iter_cell != mList->end(); iter_cell ++ ) {
			anInst = ( simpleInstance * ) ( * iter_cell );
			if ( anInst->Width > maxWidth) {
				if ( anInst->flag == 0 ) {
					maxWidth = anInst->Width;
					maxInst = anInst;
				}
			}
		}
		//put the bigest cell
		maxInst->rowNo = rowNo;
		maxInst->TL.x = maxlayoutW/2;
		maxInst->TL.y = rowNo * rowHeight;
		maxInst->flag = 1;
		rowWidth += maxInst->Width;
		neighbor = maxInst;
		row->push_back(neighbor);
		num_cell ++;
		
		
		//continue until this row will be full
		while ( rowWidth < maxlayoutW ) {
			list <simpleInstance *>::iterator iter_neigh;
			list <simpleInstance *> *neighbors = new list <simpleInstance *>;
			simpleInstance *SI = new simpleInstance;
			
			for ( iter_net = nList->begin(); iter_net != nList->end(); iter_net ++ ) {
				aNet = ( simpleNet * )( * iter_net);
				for ( iter_cell = aNet->instances->begin(); iter_cell != aNet->instances->end(); iter_cell ++ ) {
					Ins = ( simpleInstance * )( * iter_cell);
					if ( Ins->InstID == neighbor->InstID ) {
						for ( iter_neigh = aNet->instances->begin(); iter_neigh != aNet->instances->end(); iter_neigh ++ ) {
							SI = ( simpleInstance * )( * iter_neigh );
							neighbors->push_back ( SI );
						}
						nets->push_back ( aNet );		
					}
				}
			}
			
			//select a random neighbor of this instance
			for ( iter_neigh = neighbors->begin(); iter_neigh != neighbors->end(); iter_neigh ++ ) {
				anInst = ( simpleInstance * ) ( * iter_neigh );
				if ( anInst->flag != 1 ) {
					neighbor_1 = anInst;
					neighbor->nflag = 1;
				}
			}	
			
			if ( neighbor->nflag == 1 ) {
				if ( neighbor == maxInst ) {
					//put left
					neighbor_1->rowNo = rowNo;
					neighbor_1->TL.x = neighbor->TL.x - neighbor_1->Width;
					neighbor_1->TL.y = rowNo * rowHeight;
					lLeft = THPWL();
					//printf("**Final half perimeter wirelength "left" = %lf \n", lLeft );
	
					//put right
					neighbor_1->TL.x = neighbor->TL.x + neighbor->Width;
					lRight = THPWL();
					//printf("**Final half perimeter wirelength "right" = %lf \n", lRight );
	
					if ( lLeft < lRight ) {
						neighbor_1->TL.x = neighbor->TL.x - neighbor_1->Width;
						neighbor_1->LR = 1;
						row->push_front(neighbor_1);
						
					}
					else {
						neighbor_1->LR = 0;
						row->push_back(neighbor_1);
					}
					neighbor_1->flag = 1;
					rowWidth += neighbor_1->Width;
					neighbor = neighbor_1;
					num_cell ++;
				}
				else {	
					if ( neighbor->LR == 1 ) {
						if ( neighbor->TL.x -  neighbor_1->Width > 0 ) {
							neighbor_1->rowNo = rowNo;
							neighbor_1->TL.x = neighbor->TL.x - neighbor_1->Width;
							neighbor_1->TL.y = rowNo * rowHeight;
							neighbor_1->flag = 1;
							neighbor_1->LR = 1;
							row->push_front(neighbor_1);
							rowWidth += neighbor_1->Width;
							num_cell ++;
						}
						else {
							float maxX = 0;
							simpleInstance *maxXIn = new simpleInstance;
							for ( iter_row = row->begin(); iter_row != row->end(); iter_row ++ ) {
								Ist = ( simpleInstance * ) ( * iter_row );
								if ( Ist->TL.x > maxX ) {
									maxX = Ist->TL.x;
									maxXIn = Ist;
								}
							}
							if ( maxXIn->TL.x + maxXIn->Width + neighbor_1->Width < maxlayoutW ) {
								neighbor_1->rowNo = rowNo;
								neighbor_1->TL.x = maxXIn->TL.x + maxXIn->Width;
								neighbor_1->TL.y = rowNo * rowHeight;
								neighbor_1->flag = 1;
								neighbor_1->LR = 0;
								row->push_back(neighbor_1);
								rowWidth += neighbor_1->Width;
								num_cell ++;
							}
						}
					}
					else {
						if ( neighbor->TL.x + neighbor->Width + neighbor_1->Width < maxlayoutW ) {
							neighbor_1->rowNo = rowNo;
							neighbor_1->TL.x = neighbor->TL.x + neighbor->Width;
							neighbor_1->TL.y = rowNo * rowHeight;
							neighbor_1->flag = 1;
							neighbor_1->LR = 0;
							row->push_back(neighbor_1);
							rowWidth += neighbor_1->Width;
							num_cell ++;
						}
						else {
							float minX = 100000000;
							simpleInstance *minXIn = new simpleInstance;
							for ( iter_row = row->begin(); iter_row != row->end(); iter_row ++ ) {
								Ist = ( simpleInstance * ) ( * iter_row );
								if ( Ist->TL.x < minX ) {
									minX = Ist->TL.x;
									minXIn = Ist;
								}
							}
							if ( minXIn->TL.x - neighbor_1->Width > 0 ) {
								neighbor_1->rowNo = rowNo;
								neighbor_1->TL.x = minXIn->TL.x - neighbor_1->Width;
								neighbor_1->TL.y = rowNo * rowHeight;
								neighbor_1->flag = 1;
								neighbor_1->LR = 1;
								row->push_front(neighbor_1);
								rowWidth += neighbor_1->Width;
								num_cell ++;
							}
						}
					}
				}
				if ( neighbor_1->flag == 1 ) {
					neighbor = neighbor_1;
				}
				else {
					rowWidth = maxlayoutW;
				}
			}
			else {
				simpleInstance *begin = new simpleInstance;
				simpleInstance *end = new simpleInstance;
				for ( iter_row = row->begin(); iter_row != row->end(); iter_row ++ ) {
					end = ( simpleInstance * ) ( * iter_row );
				}
				for ( iter_cell = mList->begin(); iter_cell != mList->end(); iter_cell ++ ) {
					anInst = ( simpleInstance * ) ( * iter_cell );
					if ( anInst->flag == 0 ) {
						if ( end->TL.x + end->Width + anInst->Width < maxlayoutW ) {
							anInst->rowNo = rowNo;
							anInst->TL.x = end->TL.x + end->Width;
							anInst->TL.y = rowNo * rowHeight;
							anInst->flag = 1;
							anInst->LR = 0;
							rowWidth += anInst->Width;
							row->push_back(anInst);
							num_cell ++;
							for ( iter_row = row->begin(); iter_row != row->end(); iter_row ++ ) {
								end = ( simpleInstance * ) ( * iter_row );
							}
						}
					}
				}
				iter_row = row->begin();
				begin = ( simpleInstance * ) ( * iter_row );
				for ( iter_cell = mList->begin(); iter_cell != mList->end(); iter_cell ++ ) {
					anInst = ( simpleInstance * ) ( * iter_cell );
					if ( anInst->flag == 0 ) {
						if ( begin->TL.x - anInst->Width > 0 ) {
							anInst->rowNo = rowNo;
							anInst->TL.x = begin->TL.x - anInst->Width;
							anInst->TL.y = rowNo * rowHeight;
							anInst->flag = 1;
							anInst->LR = 1;
							rowWidth += anInst->Width;
							row->push_front(anInst);
							num_cell ++;
							iter_row = row->begin();
							begin = ( simpleInstance * ) ( * iter_row );
						}
					}
				}
				rowWidth = maxlayoutW;
			}
		}
		rowNo ++;
	}
	int num_1 = 0;
	for ( iter_cell = mList->begin(); iter_cell != mList->end(); iter_cell ++ ) {
		anInst = ( simpleInstance * ) ( * iter_cell );
		if (anInst->flag == 1) {
			num_1 ++;
		}
	}
	if ( mList->size() == num_1 ) {
		printf("successful \n");
	}
	else {
		printf("not successful \n");
	}
	rowNumber = rowNo;
	layoutW = maxlayoutW;
	layoutH = maxlayoutH;
	printf("\nFinal Cost:  %f",this->THPWL());
	return 1;
}

void SimplePlacer::updateDB ( atlasDB *db )
{
	list <simpleInstance *>::iterator iter;
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
	list <simpleInstance *>::iterator iter1;
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
	list <simpleNet *>::iterator iter1;
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


