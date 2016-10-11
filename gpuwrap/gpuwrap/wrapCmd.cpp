#include "wrapCmd.h"
#include "wrapDeformer.h"

#include <maya/MArgDatabase.h>
#include <maya/MSyntax.h>
#include <maya/MFnDagNode.h>
#include <maya/MGlobal.h>
#include <maya/MItSelectionList.h>
#include <maya/MItDependencyGraph.h>

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

	// Connect the driver mesh to the wrap deformer
	MFnDependencyNode fnNode(oWrapNode_, &status);
	CHECK_MSTATUS_AND_RETURN_IT(status);
	setResult(fnNode.name());
	// Store all binding information on the deformer
	return status;
}

MStatus WrapCmd::undoIt() {
	MStatus status;
	status = dgMod_.undoIt();
	CHECK_MSTATUS_AND_RETURN_IT(status);
	return MS::kSuccess;
}
