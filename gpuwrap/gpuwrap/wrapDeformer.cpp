#include "wrapDeformer.h"

#include <maya/MItGeometry.h>
#include <maya/MTypeId.h>
#include <maya/MDataBlock.h>
#include <maya/MMatrix.h>

// Need to get an id from Autodesk, I made this one up.
MTypeId Wrap::id(0x0014456B);

// might conflict because the command has the same kName
const char* Wrap::kName = "Wrap";

const char* Wrap::initialize() {
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