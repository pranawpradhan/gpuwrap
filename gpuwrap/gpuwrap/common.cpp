#include "common.h"

#include <maya/MVector.h>

void CalculateBasisComponents(const BaryCoords& coords,
	const MIntArray& triangleVertices,
	const MPointArray& points,
	const MFloatVectorArray& normals,
	MPoint& origin, MVector& up, MVector& normal) {
	
	// Use barycentric coordinates to calculate origin and normal
	// Calculate origin
	// calculate normal
	origin = MPoint::origin;
	normal = MVector::zero;
	for (int i = 0; i < 3; ++i) {
		origin += points[triangleVertices[i]] * coords[i];
		normal += MVector(normals[triangleVertices[i]]) * coords[i];
	}
		
	// calculate up vector
	// The up vector will be the vector to the lowest weighted point on a barycentric system
	// Find the lowest barycentric weight

	float lowestWeight = coords[0];
	int lowestVertexId = triangleVertices[0];
	for (int i = 1; i < 3; i++) {
		if (coords[i] < lowestWeight) {
			lowestWeight = coords[i];
			lowestVertexId = triangleVertices[i];
		}
	}

	up = points[lowestVertexId] - origin;
	normal.normalize();
	up.normalize();
};

void CreateMatrix(const MPoint& origin,
				  const MVector& normal,
				  const MVector& up,
				  MMatrix& matrix)
{
	const MPoint& t = origin;
	const MVector& y = normal;
	MVector x = y ^ up;
	MVector z = y ^ x;
	matrix[0][0] = x.x; matrix[0][1] = x.y; matrix[0][2] = x.z; matrix[0][3] = 0.0;
	matrix[1][0] = y.x; matrix[1][1] = y.y; matrix[1][2] = y.z; matrix[1][3] = 0.0;
	matrix[2][0] = z.x; matrix[2][1] = z.y; matrix[2][2] = z.z; matrix[2][3] = 0.0;
	matrix[3][0] = t.x; matrix[3][1] = t.y; matrix[3][2] = t.z; matrix[3][3] = 1.0;
	
}