#include "wrapCmd.h"

#include <maya/MArgDatabase.h>
#include <maya/MSyntax.h>

const char* WrapCmd::kName = "Wrap";

WrapCmd::WrapCmd() {}

MSyntax WrapCmd::newSyntax() {
	MSyntax syntax;
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
	return redoIt();
}

MStatus WrapCmd::redoIt() {
	MStatus status;
	return status;
}

MStatus WrapCmd::undoIt() {
	MStatus status;
	return MS::kSuccess;
}
