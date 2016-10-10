#include "wrapCmd.h"

#include <maya/MArgDatabase.h>
#include <maya/MSyntax.h>
#include <maya/MFnDagNode.h>
#include <maya/MGlobal.h>
#include <maya/MItSelectionList.h>

const char* WrapCmd::kName = "Wrap";
const char* WrapCmd::kNameFlagShort = "-n";
const char* WrapCmd::kNameFlagLong = "-name";

WrapCmd::WrapCmd() : name_("Wrap#") {}

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
	return false;
}

MStatus WrapCmd::doIt(const MArgList& args) {
	MStatus status;

	// Get the geometry from the command arguments
	status = GatherCommandArguments(args);
	CHECK_MSTATUS_AND_RETURN_IT(status);
	status = GetGeometryPaths();
	CHECK_MSTATUS_AND_RETURN_IT(status);
	// Create deformer



	// Calculate binding for wrap deformer
	// Connect the driver mesh to the wrap deformer
	// Store all binding information on the deformer
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

MStatus WrapCmd::redoIt() {
	MStatus status;
	return status;
}

MStatus WrapCmd::undoIt() {
	MStatus status;
	return MS::kSuccess;
}
