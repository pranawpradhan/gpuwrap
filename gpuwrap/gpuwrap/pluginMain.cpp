#include "wrapCmd.h"
#include "wrapDeformer.h"

#include <maya/MFnPlugin.h>
#include <maya/MGlobal.h>


MStatus initializePlugin(MObject obj) {
	MStatus status;
	MFnPlugin plugin(obj, "Alex Widener", "1.0", "Any");

	status = plugin.registerNode(
		Wrap::kName,
		Wrap::id,
		Wrap::creator,
		Wrap::initialize,
		MPxNode::kDeformerNode);

	CHECK_MSTATUS_AND_RETURN_IT(status);
	status = plugin.registerCommand(
		WrapCmd::kName,
		WrapCmd::creator,
		WrapCmd::newSyntax);

	CHECK_MSTATUS_AND_RETURN_IT(status);
	return status;
}

MStatus uninitializePlugin(MObject obj) {
	MStatus status;
	MFnPlugin plugin(obj);

	status = plugin.deregisterCommand(WrapCmd::kName);
	CHECK_MSTATUS_AND_RETURN_IT(status);
	status = plugin.deregisterNode(Wrap::id);
	CHECK_MSTATUS_AND_RETURN_IT(status);
	return status;
}

