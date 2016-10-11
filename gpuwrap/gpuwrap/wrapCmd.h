#ifndef WRAPCMD_H
#define WRAPCMD_H

#include <vector>

#include <maya/MArgList.h>
#include <maya/MPxCommand.h>
#include <maya/MDagPath.h>
#include <maya/MDagPathArray.h>
#include <maya/MSelectionList.h>
#include <maya/MDGModifier.h>
#include <maya/MMeshIntersector.h>
#include <maya/MPointArray.h>

struct BaryCoords {
	float coords[3];
	float operator[](int index) const { return coords[index]; }
	float& operator[](int index) { return coords[index]; }
};

struct BindData {
	MPointArray driverPoints;
	// The first vector will be accessed by the face id
	// The second vector will be accessed by the triangle id on that face
	// MIntArray will be 3 vertex indices on that triangle
	std::vector<std::vector<MIntArray>> perFaceTriangleVertices; /**< The per-face per-triangle vertex id of the driver*/
	MMeshIntersector intersector;
	std::vector<BaryCoords> coords;
	// MIntArray will always be length 3
	// Vector is per-vertex storage on the deforming mesh.
	// For each vertex on the deforming mesh, it's storing the 3 triangle mesh ids
	std::vector<MIntArray> triangleVertices;

};

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

	/**
	 * Get latest wrap node in the history of the deformed shape
	 */
	MStatus GetLatestWrapNode();

	/*
	Ensures that the given DAG path points to a non-intermediate shape node.
	@param[in,out] path Path to a dag node that could be a transform or a shape.
	On return, the path will be to a shape node if one exists.
	@param[in] intermediate true to get the intermediate shape.
	@return MStatus
	*/
	MStatus GetShapeNode(MDagPath& path, bool intermediate = false);

	MStatus CalculateBinding(MDagPath& path);
	
	/* 
	 * Get the barycentric coordinates of point P in the triangle specified by points A,B,C
	 * @param[in] P - The sample point
	 * @param[in] A - Triangle point
	 * @param[in] B - Triangle point
	 * @param[in] C - Triangle point
	 * @param[out] coords Barycentric coordinates storage
	 */
	void GetBarycentricCoordinates(const MPoint& P, const MPoint& A, const MPoint& B, const MPoint& C, BaryCoords& coords);

	MString name_; // Name of Wrap node to create
	MDagPath pathDriver_; // Path to the shape wrapping the other shape
	MDagPathArray pathDriven_; // Path to the shapes being wrapped
	MSelectionList selectionList_; // Selected command input 
	MDGModifier dgMod_;
	MObject oWrapNode_; // MObject to the wrap node in focus.
};

#endif