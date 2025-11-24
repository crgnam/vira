#ifndef PRIV_MATERIAL_LOAD_IPP
#define PRIV_MATERIAL_LOAD_IPP

namespace vira::geometry {

    // Local helper for loading materials:
    static inline bool isEmbeddedTexture(const std::string& texturePath) {
        bool isEmbedded = texturePath.length() > 0 && texturePath[0] == '*';
        return isEmbedded;
    }

    static inline int getEmbeddedTextureIndex(const std::string& texturePath) {
        if (!isEmbeddedTexture(texturePath)) {
            return -1;
        }

        try {
            return std::stoi(texturePath.substr(1)); // Remove '*' and convert to int
        }
        catch (const std::exception&) {
            return -1;
        }
    }

    template <IsSpectral TSpectral>
    aiMaterial* createAssimpMaterial(const vira::materials::Material<TSpectral>& material, std::string material_name, std::function<ColorRGB(TSpectral)> spectralToRGB_function)
    {
        // TODO Implement (currently just returns a default material)
        (void)material;
        (void)spectralToRGB_function;

        aiMaterial* ai_material = new aiMaterial();

        // Set material name
        aiString name(material_name);
        ai_material->AddProperty(&name, AI_MATKEY_NAME);

        // === Core PBR Properties (using standard keys) ===

        // Base Color (Albedo) - Default to light gray
        aiColor3D baseColor(0.8f, 0.8f, 0.8f);
        ai_material->AddProperty(&baseColor, 1, AI_MATKEY_BASE_COLOR);

        // Metallic Factor (0.0 = dielectric, 1.0 = metallic)
        float metallicFactor = 0.0f;
        ai_material->AddProperty(&metallicFactor, 1, AI_MATKEY_METALLIC_FACTOR);

        // Roughness Factor (0.0 = mirror, 1.0 = completely rough)
        float roughnessFactor = 0.8f;
        ai_material->AddProperty(&roughnessFactor, 1, AI_MATKEY_ROUGHNESS_FACTOR);

        // === Standard Properties ===

        // Emissive Color (for glowing materials)
        aiColor3D emissive(0.0f, 0.0f, 0.0f);
        ai_material->AddProperty(&emissive, 1, AI_MATKEY_COLOR_EMISSIVE);

        // Emissive Strength (if supported)
        float emissiveStrength = 1.0f;
        ai_material->AddProperty(&emissiveStrength, 1, AI_MATKEY_EMISSIVE_INTENSITY);

        // === Legacy Properties (for compatibility with older formats) ===

        // Diffuse color (fallback)
        ai_material->AddProperty(&baseColor, 1, AI_MATKEY_COLOR_DIFFUSE);

        // Specular color (set to low value for PBR workflow)
        aiColor3D specular(0.04f, 0.04f, 0.04f); // Default dielectric F0
        ai_material->AddProperty(&specular, 1, AI_MATKEY_COLOR_SPECULAR);

        // Shininess (derived from roughness)
        float shininess = (1.0f - roughnessFactor) * 128.0f;
        ai_material->AddProperty(&shininess, 1, AI_MATKEY_SHININESS);

        // === Transparency Properties ===

        // Opacity
        float opacity = 1.0f;
        ai_material->AddProperty(&opacity, 1, AI_MATKEY_OPACITY);

        // === Other Standard Properties ===

        // Two-sided rendering
        bool doubleSided = false;
        ai_material->AddProperty(&doubleSided, 1, AI_MATKEY_TWOSIDED);

        // Shading model (set to PBR if supported)
        int shadingModel = aiShadingMode_PBR_BRDF;
        ai_material->AddProperty(&shadingModel, 1, AI_MATKEY_SHADING_MODEL);

        return ai_material;
    };

    static inline vira::images::Image<ColorRGB> readAssimpTextureRGB(const aiScene* ai_scene, const fs::path& basePath, const std::string& texturePathStr)
    {
        vira::images::Image<ColorRGB> output;

        if (isEmbeddedTexture(texturePathStr)) {
            // Handle embedded texture
            int textureIndex = getEmbeddedTextureIndex(texturePathStr);
            if (textureIndex >= 0 && textureIndex < static_cast<int>(ai_scene->mNumTextures)) {
                const aiTexture* embeddedTexture = ai_scene->mTextures[textureIndex];

                if (embeddedTexture->mHeight == 0) {
                    // Compressed texture (JPEG, PNG, etc.)
                    std::string format = embeddedTexture->achFormatHint[0] ?
                        std::string(embeddedTexture->achFormatHint) : "jpg";

                    output = vira::images::ImageInterface::readImageRGBFromMemory(
                        reinterpret_cast<const unsigned char*>(embeddedTexture->pcData),
                        embeddedTexture->mWidth,  // mWidth contains the data size for compressed textures
                        format
                    );
                }
                else {
                    // Uncompressed texture (raw RGBA data)
                    vira::images::Resolution resolution(embeddedTexture->mWidth, embeddedTexture->mHeight);
                    output = vira::images::Image<ColorRGB>(resolution);

                    for (size_t y = 0; y < embeddedTexture->mHeight; ++y) {
                        for (size_t x = 0; x < embeddedTexture->mWidth; ++x) {
                            const aiTexel& texel = embeddedTexture->pcData[y * embeddedTexture->mWidth + x];
                            ColorRGB rgb(texel.r / 255.0f, texel.g / 255.0f, texel.b / 255.0f);
                            output(x, y) = rgb;
                        }
                    }
                }
            }
            else {
                throw std::runtime_error("Invalid embedded texture index: " + std::to_string(textureIndex));
            }
        }
        else {
            // Handle external texture file
            fs::path fullTexturePath = basePath / texturePathStr;
            output = vira::images::ImageInterface::readImageRGB(fullTexturePath);
        }

        return output;
    }

    static inline vira::images::Image<float> readAssimpTexture(const aiScene* ai_scene, const fs::path& basePath, const std::string& texturePathStr)
    {
        vira::images::Image<float> output;

        if (isEmbeddedTexture(texturePathStr)) {
            // Handle embedded texture
            int textureIndex = getEmbeddedTextureIndex(texturePathStr);
            if (textureIndex >= 0 && textureIndex < static_cast<int>(ai_scene->mNumTextures)) {
                const aiTexture* embeddedTexture = ai_scene->mTextures[textureIndex];

                if (embeddedTexture->mHeight == 0) {
                    // Compressed texture (JPEG, PNG, etc.)
                    std::string format = embeddedTexture->achFormatHint[0] ?
                        std::string(embeddedTexture->achFormatHint) : "jpg";

                    output = vira::images::ImageInterface::readImageFromMemory(
                        reinterpret_cast<const unsigned char*>(embeddedTexture->pcData),
                        embeddedTexture->mWidth,  // mWidth contains the data size for compressed textures
                        format
                    );
                }
                else {
                    // Uncompressed texture (raw RGBA data)
                    vira::images::Resolution resolution(embeddedTexture->mWidth, embeddedTexture->mHeight);
                    output = vira::images::Image<float>(resolution);

                    for (size_t y = 0; y < embeddedTexture->mHeight; ++y) {
                        for (size_t x = 0; x < embeddedTexture->mWidth; ++x) {
                            const aiTexel& texel = embeddedTexture->pcData[y * embeddedTexture->mWidth + x];
                            // For single-channel data, typically use the red channel
                            // Convert from [0,255] to [0.0,1.0] float range
                            float value = texel.r / 255.0f;
                            output(x, y) = value;
                        }
                    }
                }
            }
            else {
                throw std::runtime_error("Invalid embedded texture index: " + std::to_string(textureIndex));
            }
        }
        else {
            // Handle external texture file
            fs::path fullTexturePath = basePath / texturePathStr;
            output = vira::images::ImageInterface::readImage(fullTexturePath);
        }

        return output;
    };

    template <IsSpectral TSpectral>
    std::unique_ptr<vira::materials::Material<TSpectral>> createMaterialFromAssimp(const fs::path& basePath, const aiMaterial* assimpMaterial, const aiScene* ai_scene, std::function<TSpectral(ColorRGB)> rgbToSpectral_function) {
        std::unique_ptr<vira::materials::Material<TSpectral>> material;

        float metallicFactor = 0.0f;
        float roughnessFactor = 0.5f;

        // Try to get metallic/roughness values
        assimpMaterial->Get(AI_MATKEY_METALLIC_FACTOR, metallicFactor);
        assimpMaterial->Get(AI_MATKEY_ROUGHNESS_FACTOR, roughnessFactor);

        // TODO Use a PBR Material once those are supported:
        material = std::make_unique<vira::materials::Lambertian<TSpectral>>();
        //material = std::make_unique<vira::materials::PBRMaterial<TSpectral>>();

        material->setMetalness(metallicFactor);
        material->setRoughness(roughnessFactor);

        // Load albedo/diffuse color
        aiColor3D diffuseColor(1.0f, 1.0f, 1.0f);
        if (assimpMaterial->Get(AI_MATKEY_COLOR_DIFFUSE, diffuseColor) == AI_SUCCESS) {
            ColorRGB rgb(diffuseColor.r, diffuseColor.g, diffuseColor.b);

            // Convert RGB to spectral:
            material->setAlbedo(rgbToSpectral_function(rgb));
        }

        // Load diffuse texture if available
        aiString texturePath;
        if (assimpMaterial->GetTexture(aiTextureType_DIFFUSE, 0, &texturePath) == AI_SUCCESS) {
            try {
                std::string texturePathStr = std::string(texturePath.C_Str());

                // Read the ASSIMP Texture:
                vira::images::Image<ColorRGB> albedoImageRGB = readAssimpTextureRGB(ai_scene, basePath, texturePathStr);

                // Convert from sRGB to Linear colorspace:
                vira::images::Image<TSpectral> albedoImage(albedoImageRGB.resolution());
                for (size_t i = 0; i < albedoImageRGB.size(); ++i) {
                    ColorRGB linearValue = vira::images::sRGBToLinear_val(albedoImageRGB[i]);
                    albedoImage[i] = rgbToSpectral_function(linearValue);
                }

                material->setAlbedo(albedoImage);
            }
            catch (const std::exception& e) {
                std::cerr << "Warning: Failed to load diffuse texture " << texturePath.C_Str() << ": " << e.what() << std::endl;
            }
        }

        // Load normal map if available
        if (assimpMaterial->GetTexture(aiTextureType_NORMALS, 0, &texturePath) == AI_SUCCESS ||
            assimpMaterial->GetTexture(aiTextureType_HEIGHT, 0, &texturePath) == AI_SUCCESS) {
            try {
                std::string texturePathStr = std::string(texturePath.C_Str());

                // Read the ASSIMP Texture:
                vira::images::Image<ColorRGB> normalImageRGB = readAssimpTextureRGB(ai_scene, basePath, texturePathStr);

                // Convert from RGB to Normal vectors:
                vira::images::Image<vira::vec3<float>> normalMap = vira::images::Image<vec3<float>>(normalImageRGB.resolution());
                for (size_t i = 0; i < normalImageRGB.size(); ++i) {
                    ColorRGB pixel = normalImageRGB[i];
                    float x = pixel[0] * 2.f - 1.f;
                    float y = pixel[1] * 2.f - 1.f;
                    float z = pixel[2] * 2.f - 1.f;
                    normalMap[i] = vec3<float>(x, y, z);
                }

                material->setNormalMap(normalMap);
            }
            catch (const std::exception& e) {
                std::cerr << "Warning: Failed to load normal map texture " << texturePath.C_Str() << ": " << e.what() << std::endl;
            }
        }

        // Load roughness map if available
        if (assimpMaterial->GetTexture(aiTextureType_DIFFUSE_ROUGHNESS, 0, &texturePath) == AI_SUCCESS) {
            try {
                std::string texturePathStr = std::string(texturePath.C_Str());

                // Read the ASSIMP Texture:
                vira::images::Image<float> roughnessImage = readAssimpTexture(ai_scene, basePath, texturePathStr);

                material->setRoughness(roughnessImage);
            }
            catch (const std::exception& e) {
                std::cerr << "Warning: Failed to load roughness texture " << texturePath.C_Str() << ": " << e.what() << std::endl;
            }
        }

        // Load metalness map if available
        if (assimpMaterial->GetTexture(aiTextureType_METALNESS, 0, &texturePath) == AI_SUCCESS) {
            try {
                std::string texturePathStr = std::string(texturePath.C_Str());

                // Read the ASSIMP Texture:
                vira::images::Image<float> metalnessImage = readAssimpTexture(ai_scene, basePath, texturePathStr);

                material->setMetalness(metalnessImage);
            }
            catch (const std::exception& e) {
                std::cerr << "Warning: Failed to load metalness texture " << texturePath.C_Str() << ": " << e.what() << std::endl;
            }
        }

        // Load emission value if available:
        aiColor3D emissionColor(0.0f, 0.0f, 0.0f);
        if (assimpMaterial->Get(AI_MATKEY_COLOR_EMISSIVE, emissionColor) == AI_SUCCESS) {
            if (emissionColor.r > 0.0f || emissionColor.g > 0.0f || emissionColor.b > 0.0f) {
                ColorRGB rgb(emissionColor.r, emissionColor.g, emissionColor.b);

                // Convert RGB to spectral:
                material->setEmission(rgbToSpectral_function(vira::images::sRGBToLinear_val(rgb)));
            }
        }

        // Load emission texture if available
        if (assimpMaterial->GetTexture(aiTextureType_EMISSIVE, 0, &texturePath) == AI_SUCCESS) {
            try {
                std::string texturePathStr = std::string(texturePath.C_Str());

                // Read the ASSIMP Texture:
                vira::images::Image<ColorRGB> emissionImageRGB = readAssimpTextureRGB(ai_scene, basePath, texturePathStr);

                // Convert from sRGB to Spectral:
                vira::images::Image<TSpectral> emissionImage(emissionImageRGB.resolution());
                for (size_t i = 0; i < emissionImageRGB.size(); ++i) {
                    ColorRGB linearValue = vira::images::sRGBToLinear_val(emissionImageRGB[i]);
                    emissionImage[i] = rgbToSpectral_function(linearValue);
                }
                material->setEmission(emissionImage);
            }
            catch (const std::exception& e) {
                std::cerr << "Warning: Failed to load emission texture " << texturePath.C_Str() << ": " << e.what() << std::endl;
            }
        }

        return material;
    }

}

#endif