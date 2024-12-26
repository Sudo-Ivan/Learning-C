#ifndef MATERIAL_LOADER_H
#define MATERIAL_LOADER_H

#include "raylib.h"

typedef struct MaterialData {
    char diffuseMap[256];
    char normalMap[256];
    char specularMap[256];
    Vector3 color;
    float shininess;
    float metallic;
    float roughness;
} MaterialData;

Material LoadMaterialFromFile(const char* filename);
void UnloadMaterialData(Material* material);

#endif 