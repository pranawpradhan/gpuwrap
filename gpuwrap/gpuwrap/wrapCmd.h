#ifndef WRAPCMD_H
#define WRAPCMD_H

#include <maya/MArgList.h>
#include <maya/MPxCommand.h>

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
};

#endif