#include "wrapDeformer.h"

#include <maya/MGlobal.h>
#include <maya/MItGeometry.h>
#include <maya/MTypeId.h>
#include <maya/MDataBlock.h>
#include <maya/MMatrix.h>
#include <maya/MFnCompoundAttribute.h>
#include <maya/MFnMatrixAttribute.h>
#include <maya/MFnNumericAttribute.h>
#include <maya/MFnTypedAttribute.h>

// Need to get an id from Autodesk, I made this one up.
MTypeId Wrap::id(0x0014456B);

// might conflict because the command has the same kName
const char* Wrap::kName = "Wrap";

MObject Wrap::aDriverGeo;
MObject Wrap::aBindData;
MObject Wrap::aSampleComponents;
MObject Wrap::aSampleWeights;
MObject Wrap::aTriangleVerts;
MObject Wrap::aBarycentricWeights;
MObject Wrap::aBindMatrix;

MStatus Wrap::initialize() {
	MFnCompoundAttribute cAttr;
	MFnMatrixAttribute mAttr;
	MFnTypedAttribute tAttr;
	MFnNumericAttribute nAttr;

	MStatus status;
	aDriverGeo = tAttr.create("driver", "driver", MFnData::kMesh);
	addAttribute(aDriverGeo);
	attributeAffects(aDriverGeo, outputGeom);

	/* Each output geometry needs:
	-- bindData: per geometry.
	   | -- sampleComponents
	   | -- sampleWeights
	   | -- triangleVerts
	   | -- barycentric weights
	   | -- bindMatrix
	*/
	// Per-vertex Attributes
	aSampleComponents = tAttr.create("sampleComponents", "sampleComponents", MFnData::kIntArray);
	tAttr.setArray(true);

	aSampleWeights = tAttr.create("sampleWeights", "sampleWeights", MFnData::kDoubleArray);
	tAttr.setArray(true);
	
	// Three triangle vertices when you calculate the closest point 
	aTriangleVerts = nAttr.create("triangleVerts", "triangleVerts", MFnNumericData::k3Int);
	nAttr.setArray(true);

	aBarycentricWeights = nAttr.create("baryCentricWeights", "baryCentricWeights", MFnNumericData::k3Float);
	nAttr.setArray(true);

	aBindMatrix = mAttr.create("bindMatrix", "bindMatrix");
	mAttr.setDefault(MMatrix::identity);
	mAttr.setArray(true);

	// Per-geometry attribute
	aBindData = cAttr.create("bindData", "bindData");
	cAttr.setArray(true);
	cAttr.addChild(aSampleComponents);
	cAttr.addChild(aSampleWeights);
	cAttr.addChild(aTriangleVerts);
	cAttr.addChild(aBarycentricWeights);
	cAttr.addChild(aBindMatrix);
	addAttribute(aBindData);
	// trigger dirty calculations to recalculate deformer
	attributeAffects(aSampleComponents, outputGeom);
	attributeAffects(aSampleWeights, outputGeom);
	attributeAffects(aTriangleVerts, outputGeom);
	attributeAffects(aBarycentricWeights, outputGeom);
	attributeAffects(aBindMatrix, outputGeom);

	MGlobal::executeCommand("makePaintable -attrType multiFloat -sm deformer cvWrap weights");

	return MS::kSuccess;
}

Wrap::Wrap() {

}

Wrap::~Wrap() {

}

void* Wrap::creator() {
	return new Wrap();
}

MStatus Wrap::deform(MDataBlock& MDataHandle, MItGeometry& itGeo, const MMatrix& localToWorldMatrix, unsigned int geomIndex) {
	MStatus status;
	return MS::kSuccess;
}