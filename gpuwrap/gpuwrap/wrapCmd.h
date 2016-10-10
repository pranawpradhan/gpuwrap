#ifndef WRAPCMD_H
#define WRAPCMD_H

#include <maya/MArgList.h>
#include <maya/MPxCommand.h>
#include <maya/MDagPath.h>
#include <maya/MDagPathArray.h>
#include <maya/MSelectionList.h>
#include <maya/MDGModifier.h>

/*
	The Wrap Command is used to create new Wrap Deformers
*/

class WrapCmd : public MPxCommand {
public:
	WrapCmd();
	virtual MStatus		doIt(const MArgList&);
	virtual MStatus		undoIt();
	virtual MStatus		redoIt();
	virtual bool		isUndoable() const;
	static void*		creator();
	static MSyntax		newSyntax();
	const static char*	kName;

	const static char*	kNameFlagShort;
	const static char*	kNameFlagLong;
private:
	/**
		Gathers all the command arguments and sets necessary command slates
		@param[in] args Maya MArgList
	*/
	MStatus GatherCommandArguments(const MArgList& args);
	/**
		Acquires the driver and driven dag paths from the input selection list
	*/
	MStatus GetGeometryPaths();

	/*
	Ensures that the given DAG path points to a non-intermediate shape node.
	@param[in,out] path Path to a dag node that could be a transform or a shape.
	On return, the path will be to a shape node if one exists.
	@param[in] intermediate true to get the intermediate shape.
	@return MStatus
	*/
	MStatus GetShapeNode(MDagPath& path, bool intermediate = false);

	MString name_; // Name of Wrap node to create
	MDagPath pathDriver_; // Path to the shape wrapping the other shape
	MDagPathArray pathDriven_; // Path to the shapes being wrapped
	MSelectionList selectionList_; // Selected command input nodes
	MDGModifier dgMod_;
	MObject oWrapNode_; // MObject to the wrap node in focus.
};

#endif