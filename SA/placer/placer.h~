#ifndef SIMPLE_H
#define SIMPLE_H

#include <iostream>
#include <list>
#include <vector>

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

struct simpleNet {
	char 	*netName;
	int NetID;
	list <struct simpleInstance *> *instances;
	bool tag;
	simpleInstance *srcTerminal;
};

struct simpleInstance {
	int	InstID;
	char 	*InstName;
   	char 	*CellName;
    simplePoint TL;
    float 	Width, Height;
	simpleNet *net;
	int rowNo;
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
	SimplePlacer(float T0, float alpha, float Tmin, int inner_count);
	~SimplePlacer();

	int SAplacer();
	
	int readDB(atlasDB *);
	void updateDB(atlasDB *);
	void dumpPlacement ( char *dumpFile );
	void readDump (char *dumpFile);

private:
	simpleInstance *findInstance(string instName);
	vector <simpleInstance *> *mList; // List of modules wherein design
	vector <simpleNet *> *nList; // List of nets wherein design
	float rowHeight;
	unsigned maxlayoutW;
	unsigned maxlayoutH;
	int rowNumber;
	double THPWL();
	int simPlacer();
	void Perturb(bool rollback);
	void Legalizer();
	float T0;
	float Tmin;
	float alpha;
	int inner_count;
};

#endif
