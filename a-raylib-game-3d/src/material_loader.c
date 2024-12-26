#include "material_loader.h"
#include <stdio.h>
#include <string.h>

Material LoadMaterialFromFile(const char* filename) {
    Material material = LoadMaterialDefault();
    MaterialData data = {0};
    
    FILE* file = fopen(filename, "r");
    if (file == NULL) return material;

    char line[256];
    while (fgets(line, sizeof(line), file)) {
        char key[32] = {0};
        char value[224] = {0};
        
        if (sscanf(line, "%s %s", key, value) == 2) {
            if (strcmp(key, "diffuseMap") == 0) {
                material.maps[MATERIAL_MAP_DIFFUSE].texture = LoadTexture(value);
            }
            else if (strcmp(key, "normalMap") == 0) {
                material.maps[MATERIAL_MAP_NORMAL].texture = LoadTexture(value);
            }
            else if (strcmp(key, "specularMap") == 0) {
                material.maps[MATERIAL_MAP_SPECULAR].texture = LoadTexture(value);
            }
            else if (strcmp(key, "color") == 0) {
                float r, g, b;
                sscanf(value, "%f,%f,%f", &r, &g, &b);
                data.color = (Vector3){r, g, b};
                int colorLoc = GetShaderLocation(material.shader, "matColor");
                SetShaderValue(material.shader, colorLoc, &data.color, SHADER_UNIFORM_VEC3);
            }
            else if (strcmp(key, "metallic") == 0) {
                sscanf(value, "%f", &data.metallic);
                int metallicLoc = GetShaderLocation(material.shader, "metallic");
                SetShaderValue(material.shader, metallicLoc, &data.metallic, SHADER_UNIFORM_FLOAT);
            }
            else if (strcmp(key, "roughness") == 0) {
                sscanf(value, "%f", &data.roughness);
                int roughnessLoc = GetShaderLocation(material.shader, "roughness");
                SetShaderValue(material.shader, roughnessLoc, &data.roughness, SHADER_UNIFORM_FLOAT);
            }
        }
    }
    
    fclose(file);
    return material;
}

void UnloadMaterialData(Material* material) {
    for (int i = 0; i < MATERIAL_MAP_MAX; i++) {
        if (material->maps[i].texture.id > 0) {
            UnloadTexture(material->maps[i].texture);
        }
    }
} 