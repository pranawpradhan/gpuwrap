#include "wrapCmd.h"
#include "wrapDeformer.h"

#include <maya/MArgDatabase.h>
#include <maya/MSyntax.h>
#include <maya/MFnDagNode.h>
#include <maya/MGlobal.h>
#include <maya/MItSelectionList.h>
#include <maya/MItDependencyGraph.h>
#include <maya/MItGeometry.h>
#include <maya/MPointArray.h>
#include <maya/MFnMesh.h>

const char* WrapCmd::kName = "awWrap";
const char* WrapCmd::kNameFlagShort = "-n";
const char* WrapCmd::kNameFlagLong = "-name";

WrapCmd::WrapCmd() : name_("awWrap#") {}

MSyntax WrapCmd::newSyntax() {
	MSyntax syntax;
	syntax.addFlag(kNameFlagShort, kNameFlagLong, MSyntax::kString);
	// Use the current selection as a selection list, and pass the selection as a default argument
	syntax.setObjectType(MSyntax::kSelectionList, 0, 255);
	syntax.useSelectionAsDefault(true);
	return syntax;
}

void* WrapCmd::creator() {
	return new WrapCmd;
}

bool WrapCmd::isUndoable() const {
	return true;
}

MStatus WrapCmd::doIt(const MArgList& args) {
	MStatus status;

	// Get the geometry from the command arguments
	status = GatherCommandArguments(args);
	CHECK_MSTATUS_AND_RETURN_IT(status);
	status = GetGeometryPaths();
	CHECK_MSTATUS_AND_RETURN_IT(status);
	// Create deformer
	MString command = "deformer -type awWrap -n \"" + name_ + "\"";
	for (unsigned int i = 0; i < pathDriven_.length(); ++i) {
		command += " " + pathDriven_[i].partialPathName();
	}
	status = dgMod_.commandToExecute(command);
	CHECK_MSTATUS_AND_RETURN_IT(status);
	return redoIt();
}

MStatus WrapCmd::GatherCommandArguments(const MArgList& args) {
	MStatus status;
	MArgDatabase argData(syntax(), args);
	argData.getObjects(selectionList_);
	if (argData.isFlagSet(kNameFlagShort)) {
		name_ = argData.flagArgumentString(kNameFlagShort, 0, &status);
		CHECK_MSTATUS_AND_RETURN_IT(status);
	}
	return MS::kSuccess;
}

MStatus WrapCmd::GetGeometryPaths() {
	MStatus status;
	status = selectionList_.getDagPath(selectionList_.length() - 1, pathDriver_);
	CHECK_MSTATUS_AND_RETURN_IT(status);
	status = GetShapeNode(pathDriver_);
	CHECK_MSTATUS_AND_RETURN_IT(status);

	if (!pathDriver_.hasFn(MFn::kMesh)) {
		MGlobal::displayError("Wrap driver must be a mesh");
		return MS::kFailure;
	}
	MItSelectionList iter(selectionList_);
	pathDriven_.clear(); // Should be empty, call it to make sure
	for (unsigned int i = 0; i < selectionList_.length() - 1; ++i, iter.next()) {
		MDagPath path;
		MObject component;
		iter.getDagPath(path, component);
		status = GetShapeNode(path);
		CHECK_MSTATUS_AND_RETURN_IT(status);
		pathDriven_.append(path);
	}

	return MS::kSuccess;
}

bool IsShapeNode(MDagPath& path) {
	return path.node().hasFn(MFn::kMesh) ||
		path.node().hasFn(MFn::kNurbsCurve) ||
		path.node().hasFn(MFn::kNurbsSurface);
}

MStatus WrapCmd::GetShapeNode(MDagPath& path, bool intermediate) {
	MStatus status;
	/* Check to see if it's a shape node first.*/
	if (IsShapeNode(path)) {
		path.pop();
	}

	if (path.hasFn(MFn::kTransform)) {
		// If it's a transform, find out how many shape nodes are directly below it.
		unsigned int shapeCount;
		status = path.numberOfShapesDirectlyBelow(shapeCount);
		CHECK_MSTATUS_AND_RETURN_IT(status);

		for (unsigned int i = 0; i < shapeCount; i++) {
			status = path.extendToShapeDirectlyBelow(i);
			CHECK_MSTATUS_AND_RETURN_IT(status);
			MFnDagNode fnNode(path, &status);
			CHECK_MSTATUS_AND_RETURN_IT(status);
			if ((!fnNode.isIntermediateObject() && !intermediate) || (fnNode.isIntermediateObject() && intermediate)) {
				return MS::kSuccess;
			}
			// Go back to transform node and go to the next shape
			path.pop();
		}
	}
	return MS::kFailure;
}

MStatus WrapCmd::GetLatestWrapNode() {
	MStatus status;
	MObject oDriven = pathDriven_[0].node();
	MItDependencyGraph itdg(
		oDriven,
		MFn::kGeometryFilt,
		MItDependencyGraph::kUpstream,
		MItDependencyGraph::kDepthFirst,
		MItDependencyGraph::kNodeLevel,
		&status);
	CHECK_MSTATUS_AND_RETURN_IT(status);

	MObject oDeformerNode;
	for (; !itdg.isDone(); itdg.next()) {
		oDeformerNode = itdg.currentItem();
		MFnDependencyNode fnNode(oDeformerNode, &status);
		CHECK_MSTATUS_AND_RETURN_IT(status);
		if (fnNode.typeId() == Wrap::id)
		{
			oWrapNode_ = oDeformerNode;
			return MS::kSuccess;
		}

	}

	return MS::kFailure;
}

MStatus WrapCmd::redoIt() {
	MStatus status;


	// Calculate binding for wrap deformer
	status = dgMod_.doIt();
	CHECK_MSTATUS_AND_RETURN_IT(status);
	// After calling the doIt method, the wrap deformer should be created on all the passed in geometry.
	// If you're using a reference Pipeline where you're referencing in meshes, and you create a wrap deformer on
	// it, Maya will create a shapeDeformed mesh

	// Reacquire the paths because on reference geo, a new driven path is created (the shapeDeformed).
	status = GetGeometryPaths();
	CHECK_MSTATUS_AND_RETURN_IT(status);

	// Get the created wrap deformer node
	status = GetLatestWrapNode();

	// Calculate the binding
	MDGModifier dgMod;
	status = CalculateBinding(pathDriver_, dgMod);
	CHECK_MSTATUS_AND_RETURN_IT(status);
	status = dgMod.doIt();
	CHECK_MSTATUS_AND_RETURN_IT(status);
	// Connect the driver mesh to the wrap deformer
	MFnDagNode fnDriver(pathDriver_);
	MPlug plugDriverMesh = fnDriver.findPlug("worldMesh", false, &status);
	CHECK_MSTATUS_AND_RETURN_IT(status);

	status = plugDriverMesh.selectAncestorLogicalIndex(0, plugDriverMesh.attribute());
	CHECK_MSTATUS_AND_RETURN_IT(status);
	MPlug plugDriverGeo(oWrapNode_, Wrap::aDriverGeo);
	MDGModifier dgMod2;
	dgMod2.connect(plugDriverMesh, plugDriverGeo);

	status = dgMod2.doIt();
	CHECK_MSTATUS_AND_RETURN_IT(status);

	MFnDependencyNode fnNode(oWrapNode_, &status);
	CHECK_MSTATUS_AND_RETURN_IT(status);
	setResult(fnNode.name());
	// Store all binding information on the deformer
	return status;
}

MStatus WrapCmd::CalculateBinding(MDagPath& pathBindMesh, MDGModifier& dgMod) {
	MStatus status;
	BindData bindData;

	MObject oBindMesh = pathBindMesh.node();
	MMatrix driverMatrix = pathBindMesh.inclusiveMatrix();
	status = bindData.intersector.create(oBindMesh, driverMatrix);
	CHECK_MSTATUS_AND_RETURN_IT(status);

	MFnMesh fnBindMesh(pathBindMesh, &status);
	CHECK_MSTATUS_AND_RETURN_IT(status);
	fnBindMesh.getPoints(bindData.driverPoints, MSpace::kWorld);
	// Resize the pftv vector to the polygon count
	bindData.perFaceTriangleVertices.resize(fnBindMesh.numPolygons());

	// Get triangles on the bind mesh, iterate through them to create a table lookup of triangle points
	// Vertex count is per-polygon vertex count
	// Vertex list is the actual indices
	// Can use to iterate over all faces and pull out triangle information
	MIntArray vertexCount, vertexList;
	status = fnBindMesh.getVertices(vertexCount, vertexList);
	CHECK_MSTATUS_AND_RETURN_IT(status);
	MIntArray triangleCounts, triangleVertices;
	status = fnBindMesh.getTriangles(triangleCounts, triangleVertices);
	CHECK_MSTATUS_AND_RETURN_IT(status);

	for (unsigned int faceId = 0, triIter = 0; faceId < vertexCount.length(); ++faceId) {
		bindData.perFaceTriangleVertices[faceId].resize(triangleCounts[faceId]);
		// Iterate through each triangle of each face
		for (int triId = 0; triId < triangleCounts[faceId]; ++triId) {
			bindData.perFaceTriangleVertices[faceId][triId].setLength(3);
			bindData.perFaceTriangleVertices[faceId][triId][0] = triangleVertices[triIter++];
			bindData.perFaceTriangleVertices[faceId][triId][1] = triangleVertices[triIter++];
			bindData.perFaceTriangleVertices[faceId][triId][2] = triangleVertices[triIter++];
		}
	}

	MPlug plugBindData(oWrapNode_, Wrap::aBindData);

	for (unsigned int geomIndex = 0; geomIndex < pathDriven_.length(); ++geomIndex) {
		// Get plugs to binding attributes for this geometry
		MPlug plugBind = plugBindData.elementByLogicalIndex(geomIndex, &status);
		CHECK_MSTATUS_AND_RETURN_IT(status);
		MPlug plugTriangleVerts = plugBind.child(Wrap::aTriangleVerts, &status);
		CHECK_MSTATUS_AND_RETURN_IT(status);
		MPlug plugBarycentricWeights = plugBind.child(Wrap::aBarycentricWeights, &status);

		MItGeometry itGeo(pathDriven_[geomIndex], &status);
		MPointArray inputPoints;
		// Grabbing points straight out of the iterator is usually more efficient than using the iterator
		// then just calculate and put them back
		status = itGeo.allPositions(inputPoints, MSpace::kWorld);
		CHECK_MSTATUS_AND_RETURN_IT(status);

		MPointOnMesh pointOnMesh;
		// Resizing vectors
		bindData.coords.resize(itGeo.count());
		bindData.triangleVertices.resize(itGeo.count());

		// By the end of the loop, bind data will hold the per-vertex coords & verts for each triangle.
		for (unsigned int i = 0; i < inputPoints.length(); i++) {
			bindData.intersector.getClosestPoint(inputPoints[i], pointOnMesh);
			// Convert into world space
			// calling getPoint is going to be in the local space of the driver mesh

			int faceId = pointOnMesh.faceIndex();
			int triangleId = pointOnMesh.triangleIndex();

			MPoint closestPoint = MPoint(pointOnMesh.getPoint()) * driverMatrix;

			bindData.triangleVertices[i] = bindData.perFaceTriangleVertices[faceId][triangleId];
			
			// Since there's now a look-up table, can access the three vertices that make up the triangle
			GetBarycentricCoordinates(closestPoint, 
									  bindData.driverPoints[bindData.triangleVertices[i][0]],
									  bindData.driverPoints[bindData.triangleVertices[i][1]],
									  bindData.driverPoints[bindData.triangleVertices[i][2]],
									  bindData.coords[i]);
																		  

		}

		// Store the data in the wrap node data block.
		// Iterate through the geometry one by one because we need to grab the logical index 
		// out of geometry iterator because as we're iterating, indices aren't going to be continuous
		for (int i = 0; !itGeo.isDone(); itGeo.next(), ++i) {
			int logicalIndex = itGeo.index();
			// Storing triangle vertices
			MFnNumericData fnNumericData;
			MObject oNumericData = fnNumericData.create(MFnNumericData::k3Int, &status);
			CHECK_MSTATUS_AND_RETURN_IT(status);
			status = fnNumericData.setData3Int(
				bindData.triangleVertices[i][0],
				bindData.triangleVertices[i][1],
				bindData.triangleVertices[i][2]);

			MPlug plugTriangleVertsElement = plugTriangleVerts.elementByLogicalIndex(logicalIndex, &status);
			CHECK_MSTATUS_AND_RETURN_IT(status);
			status = dgMod.newPlugValue(plugTriangleVertsElement, oNumericData);
			CHECK_MSTATUS_AND_RETURN_IT(status);

			// store barycentric weights
			oNumericData = fnNumericData.create(MFnNumericData::k3Float, &status);
			CHECK_MSTATUS_AND_RETURN_IT(status);
			status = fnNumericData.setData3Float(
				bindData.coords[i][0],
				bindData.coords[i][1],
				bindData.coords[i][2]);

			MPlug plugBarycentricWeightsElement = plugBarycentricWeights.elementByLogicalIndex(logicalIndex, &status);
			CHECK_MSTATUS_AND_RETURN_IT(status);
			status = dgMod.newPlugValue(plugBarycentricWeightsElement, oNumericData);
			CHECK_MSTATUS_AND_RETURN_IT(status);

		}
		
	}
	return MS::kSuccess;
}

void WrapCmd::GetBarycentricCoordinates(const MPoint& P, const MPoint& A, const MPoint& B, const MPoint& C, BaryCoords& coords) {
	// Compute the normal of the triangle
	MVector N = (B - A) ^ (C - A);
	MVector unitN = N.normal();
	// Compute twice area of triangle ABC
	double areaABC = unitN * N;
	// If the triangle is degenerate (point on top of each other), just use one of the points
	if (areaABC == 0.0) {
		coords[0] = 1.0f;
		coords[1] = 0.0f;
		coords[2] = 0.0f;
		return;
	}
	// Compute A
	double areaPBC = unitN * ((B - P) ^ (C - P));
	coords[0] = (float)(areaPBC / areaABC);
	// Compute B
	double areaPCA = unitN * ((C - P) ^ (A - P));
	coords[1] = (float)(areaPCA / areaABC);
	// Compute C
	coords[2] = 1.0f - coords[0] - coords[1];
}


MStatus WrapCmd::undoIt() {
	MStatus status;
	status = dgMod_.undoIt();
	CHECK_MSTATUS_AND_RETURN_IT(status);
	return MS::kSuccess;
}
