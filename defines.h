#pragma once
#include "h2bParser.h"
#include "Gateware/Gateware.h"
#include <vector>;

// Most of this file is copied over from h2bParser.h
//
//struct VECTOR {
//	float x, y, z;
//};
//
//struct VERTEX {
//	VECTOR pos, uvw, nrm;
//};
//
//struct alignas(void*) ATTRIBUTES {
//	VECTOR Kd; float d;
//	VECTOR Ks; float Ns;
//	VECTOR Ka; float sharpness;
//	VECTOR Tf; float Ni;
//	VECTOR Ke; unsigned illum;
//};
//struct BATCH {
//	unsigned indexCount, indexOffset;
//};
//
//struct MATERIAL {
//	ATTRIBUTES attrib;
//	const char* name;
//	const char* map_Kd;
//	const char* map_Ks;
//	const char* map_Ka;
//	const char* map_Ke;
//	const char* map_Ns;
//	const char* map_d;
//	const char* disp;
//	const char* decal;
//	const char* bump;
//	const void* padding[2];
//};
//
//struct MESH {
//	const char* name;
//	BATCH drawInfo;
//	unsigned materialIndex;
//};

struct GAMEOBJECT {
	
	std::string name;

	unsigned vertexCount;
	unsigned indexCount;
	unsigned materialCount;
	unsigned meshCount;

	GW::MATH::GMATRIXF worldMatrix;
	std::vector<H2B::VERTEX> vertices;
	std::vector<unsigned> indices;
	std::vector<H2B::MATERIAL> materials;
	std::vector<H2B::BATCH> batches;
	std::vector<H2B::MESH> meshes;
};