#ifndef WRAPDEFORMER_H
#define WRAPDEFORMER_H

#include <vector>
#include <maya/MPxDeformerNode.h>
#include <maya/MPointArray.h>
#include "common.h"

struct TaskData {
	MPointArray driverPoints;
	MPointArray points;
	std::vector<MIntArray> triangleVerts;
	std::vector<BaryCoords> baryCoords;
};

class Wrap : public MPxDeformerNode {
public:
	Wrap();
	virtual ~Wrap();
	virtual MStatus deform(
		MDataBlock& data,
		MItGeometry& iter,
		const MMatrix& mat,
		unsigned int mIndex);
	
	static void* creator();
	static MStatus initialize();

	const static char* kName;
	static MTypeId id;

	static MObject aDriverGeo; // Drives wrap deformer
	static MObject aBindData; // per-input geo
	static MObject aSampleComponents; // Vertex IDs of verts when crawling out from surface
	static MObject aSampleWeights; // For each of sample components
	static MObject aTriangleVerts; // Store the closest point
	static MObject aBarycentricWeights; // For each of the triangle verts
	static MObject aBindMatrix; // Per vertex

};


#endif // !WRAPDEFORMER_H
