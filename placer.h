#ifndef SIMPLE_H
#define SIMPLE_H

#include <iostream>
#include <list>
using namespace std;
#include "atlasDB.h"

enum PlacerRunMode { none_pl_mode, read_file_mode, read_db_mode };

struct simplePoint  {
	float x;
	float y;
};

struct simpleRect  {
	float t;
	float l;
	float b;
	float r;
};

//Fixed_Point
struct simpleFixedPoint {
	char *pinName;
	int PinID;
	
	simplePoint TL;
	
};

struct simpleNet {
	char 	*netName;
	int NetID;
	list <struct simpleInstance *> *instances;
	bool tag;
	simpleInstance *srcTerminal;
	
	simpleFixedPoint *fixedPoint;
};

struct simpleInstance {
	int	InstID;
	char 	*InstName;
   	char 	*CellName;
    simplePoint TL;
    float 	Width, Height;
	
	int rowNo;

	float x;
	float y;

};

struct simpleInst_cmp {
	bool operator()(simpleInstance const * a, simpleInstance const * b) {
		return a->y < b->y;
	};
	
};

struct PerturbLog {
	simpleInstance *inst1;
	simpleInstance *inst2;
};

struct simpleDumpInst
{
  string instName;
  simplePoint TL;
};


class SimplePlacer {
public:
	SimplePlacer();
	~SimplePlacer();
	int simPlacer();
	int readDB(atlasDB *);
	void updateDB(atlasDB *);
	void dumpPlacement ( char *dumpFile );
	void readDump (char *dumpFile);
	
	double CostFunction();
	void readfixedPoints ( char *fixedPointsFile );
	void fixedSet();
	void createMatrix();
	void LU_Solver();
	double THPWL();
	
private:
	simpleInstance *findInstance(string instName);
	simpleFixedPoint *findfixedPoint(string pinName);
	list <simpleInstance *> *mList; // List of modules wherein design
	list <simpleNet *> *nList; // List of nets wherein design
	list <simpleFixedPoint *> *fList; // List of fixedPoints design
	int rowNumber;
	long int mSize; 	
	float rowHeight;
	int fixedPointCnt;
	void newInst();
	float *mat; 
	float *Ax; 
	float *Ay; 
	unsigned maxlayoutW;
	unsigned maxlayoutH;
	
};
#endif



