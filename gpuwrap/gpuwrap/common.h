/*
 * Contains helper functions
 */

#ifndef WRAP_COMMON_H
#define WRAP_COMMON_H

/** 
 * Helper struct to hold the 3 barycentric coordinates
 *
 */

struct BaryCoords {
	float coords[3];
	float operator[](int index) const { return coords[index]; }
	float& operator[](int index) { return coords[index]; }
};

#endif