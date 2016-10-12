/*
 * Contains helper functions
 */

#ifndef WRAP_COMMON_H
#define WRAP_COMMON_H
#include <maya/MIntArray.h>
#include <maya/MPointArray.h>
#include <maya/MIntArray.h>
#include <maya/MFloatVector.h>
#include <maya/MPoint.h>
#include <maya/MVector.h>
#include <maya/MDoubleArray.h>
#include <maya/MFloatVectorArray.h>
#include <maya/MMatrix.h>


/**
 * Helper struct to hold the 3 barycentric coordinates
 *
 */

struct BaryCoords {
	float coords[3];
	float operator[](int index) const { return coords[index]; }
	float& operator[](int index) { return coords[index]; }
};



/**
 * Calculates the components necessary to create a wrap basis matrix
 * @param[in] coords The barycentric coordinates of the closest point
 * @param[in] triangleVertices The vertex ids forming the triangle of the closest point.
 * @param[in] points The driver point array
 * @param[in] normals the driver per-vertex normal array
 * @param[out] origin The origin of the coordinate system
 * @param[out] up The up vector of the coordinate system
 * @param[out] normal The normal vector of the coordinate system
*/

void CalculateBasisComponents(const BaryCoords& coords,
							  const MIntArray& triangleVertices,
						      const MPointArray& points,
							  const MFloatVectorArray& normals,
							  MPoint& origin, MVector& up, MVector& normal);

/*
 * Creates an orthonormal basis using the given point and two axes.
 * @param[in] origin Position
 * @param[in] normal Normal Vector
 * @param[in] up Up vector
 * @param[out] matrix Generated matrix
 */
void CreateMatrix(const MPoint& origin, const MVector& normal, const MVector& up, MMatrix& matrix);

#endif