#include "wrapDeformer.h"
#include "common.h"

#include <maya/MGlobal.h>
#include <maya/MItGeometry.h>
#include <maya/MTypeId.h>
#include <maya/MDataBlock.h>
#include <maya/MMatrix.h>
#include <maya/MFnCompoundAttribute.h>
#include <maya/MFnMatrixAttribute.h>
#include <maya/MFnNumericAttribute.h>
#include <maya/MFnTypedAttribute.h>
#include <maya/MFnMesh.h>

// Need to get an id from Autodesk, I made this one up.
MTypeId Wrap::id(0x0014456B);

// might conflict because the command has the same kName
const char* Wrap::kName = "awWrap";

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

	MGlobal::executeCommand("makePaintable -attrType multiFloat -sm deformer awWrap weights");

	return MS::kSuccess;
}

MStatus GetBindInfo(MDataBlock& data, unsigned int geomIndex, TaskData& taskData) {
	MStatus status;
	MArrayDataHandle hBindDataArray = data.inputArrayValue(Wrap::aBindData);
	status = hBindDataArray.jumpToElement(geomIndex);
	CHECK_MSTATUS_AND_RETURN_IT(status);
	MDataHandle hBindData = hBindDataArray.inputValue();

	MArrayDataHandle hTriangleVerts = hBindData.child(Wrap::aTriangleVerts);
	MArrayDataHandle hBarycentricWeights = hBindData.child(Wrap::aBarycentricWeights);
	MArrayDataHandle hBindMatrix = hBindData.child(Wrap::aBindMatrix);

	unsigned int numComponents = hTriangleVerts.elementCount();
	if (numComponents == 0) {
		// No binding information yet.
		return MS::kNotImplemented;
	}

	hTriangleVerts.jumpToArrayElement(0);
	hBarycentricWeights.jumpToArrayElement(0);

	for (unsigned int i = 0; i < numComponents; i++) {
		int logicalIndex = hTriangleVerts.elementIndex();
		if (logicalIndex >= taskData.triangleVerts.size()) {
			taskData.triangleVerts.resize(logicalIndex + 1);
			taskData.baryCoords.resize(logicalIndex + 1);
			taskData.bindMatrices.setLength(logicalIndex + 1);
		}
		// Get bind matrix
		taskData.bindMatrices[logicalIndex] = hBindMatrix.inputValue().asMatrix();


		// Get the triangle vertex binding
		int3& verts = hTriangleVerts.inputValue(&status).asInt3();
		CHECK_MSTATUS_AND_RETURN_IT(status);
		MIntArray& triangleVerts = taskData.triangleVerts[logicalIndex];
		triangleVerts.setLength(3);
		triangleVerts[0] = verts[0];
		triangleVerts[1] = verts[1];
		triangleVerts[2] = verts[2];

		// Get barycentric weights 
		float3& baryWeights = hBarycentricWeights.inputValue(&status).asFloat3();
		CHECK_MSTATUS_AND_RETURN_IT(status);
		BaryCoords& coords = taskData.baryCoords[logicalIndex];
		coords[0] = baryWeights[0];
		coords[1] = baryWeights[1];
		coords[2] = baryWeights[2];

		hTriangleVerts.next();
		hBarycentricWeights.next();
		hBindMatrix.next();

	}

	return MS::kSuccess;
}

Wrap::Wrap() {

}

Wrap::~Wrap() {

}

void* Wrap::creator() {
	return new Wrap();
}

MStatus Wrap::deform(MDataBlock& data, MItGeometry& itGeo, const MMatrix& localToWorldMatrix, unsigned int geomIndex) {
	MStatus status;

	// Get driver geometry
	MDataHandle hDriverGeo = data.inputValue(aDriverGeo, &status);
	CHECK_MSTATUS_AND_RETURN_IT(status);
	MObject oDriverGeo = hDriverGeo.asMesh();
	CHECK_MSTATUS_AND_RETURN_IT(status);
	if (oDriverGeo.isNull()) {
		// Without a driver mesh, can't do anything
		return MS::kSuccess;
	}
	// Get the bind information
	TaskData taskData;
	status = GetBindInfo(data, geomIndex, taskData);

	// Get the driver geo information
	MFnMesh fnDriver(oDriverGeo, &status);
	CHECK_MSTATUS_AND_RETURN_IT(status);

	//Get the driver point positions and vertex normals
	status = fnDriver.getPoints(taskData.driverPoints, MSpace::kWorld);
	CHECK_MSTATUS_AND_RETURN_IT(status);
	status = fnDriver.getVertexNormals(false, taskData.driverNormals);
	CHECK_MSTATUS_AND_RETURN_IT(status);

	// Can't get world space because I'm inside a deformer
	// Can only get world space positions if you pass in a DAG path.
	itGeo.allPositions(taskData.points);
	
	MMatrix matrix;
	MMatrix drivenInverseMatrix = localToWorldMatrix.inverse();

	// Create temporary information using data points
	for (unsigned int i = 0; i < taskData.points.length(); ++i) {
		MIntArray& triangleVertices = taskData.triangleVerts[i];
		BaryCoords& baryCoords = taskData.baryCoords[i];
		MMatrix& bindMatrix = taskData.bindMatrices[i];

		// Three things needed to generate transform matrix
		MPoint origin;
		MVector up;
		MVector normal;

		CalculateBasisComponents(baryCoords, triangleVertices,
			taskData.driverPoints, taskData.driverNormals,
			origin, up, normal);

		CreateMatrix(origin, normal, up, matrix);
		
		// deformed point
		// multiplying bindMatrix * matrix gives you an offset from where it was bound, to where it currently is.

		MPoint newPoint = (taskData.points[i] * localToWorldMatrix) * (bindMatrix * matrix) * drivenInverseMatrix;
		taskData.points[i] = newPoint;
	}

	status = itGeo.setAllPositions(taskData.points);
	CHECK_MSTATUS_AND_RETURN_IT(status);


	return MS::kSuccess;
}