#include <vector>
#include <cstdint>
#include <fstream>
#include <memory>
#include <iomanip>
#include <unordered_map>
#include <filesystem>
#include <stdexcept>
#include <iostream>
#include <functional>
#include <set>
#include <algorithm>
#include <cctype>

// ASSIMP includes
#include <assimp/Importer.hpp>
#include <assimp/Exporter.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

// DSK includes (SPICE)
#include "cspice/SpiceUsr.h"

#include "vira/spectral_data.hpp"
#include "vira/images/interfaces/image_interface.hpp"
#include "vira/images/image.hpp"
#include "vira/images/image_utils.hpp"
#include "vira/geometry/vertex.hpp"
#include "vira/geometry/mesh.hpp"
#include "vira/utils/utils.hpp"
#include "vira/utils/hash_utils.hpp"
#include "vira/constraints.hpp"
#include "vira/materials/material.hpp"
#include "vira/materials/lambertian.hpp"
#include "vira/materials/pbr_material.hpp"
#include "vira/scene.hpp"

namespace fs = std::filesystem;

// Local private implementations:
#include "material_load.ipp"

namespace vira::geometry {
    template<IsFloat TFloat>
    mat4<TFloat> assimpToViraMat4(const aiMatrix4x4& assimpMat) {
        mat4<TFloat> viraMat;
        // ASSIMP matrices are row-major, so need to transpose:
        for (int i = 0; i < 4; ++i) {
            for (int j = 0; j < 4; ++j) {
                unsigned int i_assimp = static_cast<unsigned int>(i);
                unsigned int j_assimp = static_cast<unsigned int>(j);
                viraMat[j][i] = static_cast<TFloat>(assimpMat[i_assimp][j_assimp]);
            }
        }
        return viraMat;
    }


    template <IsSpectral TSpectral, IsFloat TFloat, IsFloat TMeshFloat> requires LesserFloat<TFloat, TMeshFloat>
    std::string GeometryInterface<TSpectral, TFloat, TMeshFloat>::detectFormat(const fs::path& filepath, const std::string& requested_format) {
        if (!vira::utils::sameString(requested_format, "auto")) {
            return requested_format;
        }

        std::string extension = filepath.extension().string();

        if (vira::utils::sameString(extension, ".obj")) return "OBJ";
        if (vira::utils::sameString(extension, ".ply")) return "PLY";
        if (vira::utils::sameString(extension, ".gltf")) return "GLTF";
        if (vira::utils::sameString(extension, ".glb")) return "GLB";
        if (vira::utils::sameString(extension, ".fbx")) return "FBX";
        if (vira::utils::sameString(extension, ".dae")) return "DAE";
        if (vira::utils::sameString(extension, ".3ds")) return "MAX3DS";
        if (vira::utils::sameString(extension, ".blend")) return "BLEND";
        if (vira::utils::sameString(extension, ".bds")) return "DSK";

        throw std::runtime_error("Cannot determine valid format from extension: " + extension);
    }

    /**
     * @brief Loads 3D geometry from a file into the scene.
     *
     * @param scene The scene to load the geometry into
     * @param filepath Path to the geometry file to load
     * @param format File format override ("AUTO" for automatic detection, or specific format like "OBJ", "PLY", "DSK", etc.)
     * @return LoadedMeshes<TFloat> Structure containing loaded mesh IDs, transformations, and node hierarchy
     *
     * Automatically detects the file format based on extension unless explicitly specified.
     * Supports various formats through Assimp (OBJ, PLY, GLTF, GLB, FBX, DAE, 3DS, BLEND)
     * and native DSK format for planetary/astronomical data.
     *
     * @throws std::runtime_error If file doesn't exist or format is unsupported
     *
     * @see loadWithAssimp(), loadDSK(), detectFormat()
     */
    template <IsSpectral TSpectral, IsFloat TFloat, IsFloat TMeshFloat> requires LesserFloat<TFloat, TMeshFloat>
    LoadedMeshes<TFloat> GeometryInterface<TSpectral, TFloat, TMeshFloat>::load(vira::Scene<TSpectral, TFloat, TMeshFloat>& scene, const fs::path& filepath, std::string format) {
        if (!fs::exists(filepath)) {
            throw std::runtime_error("File does not exist: " + filepath.string());
        }

        std::string detected_format = detectFormat(filepath, format);

        if (vira::utils::sameString(detected_format, "DSK")) {
            return loadDSK(scene, filepath);
        }
        else {
            return loadWithAssimp(scene, filepath);
        }
    }

    /**
     * @brief Loads geometry using the Assimp library.
     *
     * @param scene Target scene for loading
     * @param filepath Path to the geometry file
     * @return LoadedMeshes<TFloat> containing mesh data and hierarchy
     *
     * Handles most common 3D formats through Assimp with comprehensive post-processing:
     * - Triangulation, normal generation, UV coordinate generation
     * - Tangent space calculation, vertex deduplication
     * - Cache optimization, redundant material removal
     * - Invalid data detection and validation
     *
     * Preserves complete node hierarchy, applies transformations, loads materials
     * with texture support, and handles multi-mesh nodes by merging geometry.
     *
     * @throws std::runtime_error If Assimp fails to load the file
     */
    template <IsSpectral TSpectral, IsFloat TFloat, IsFloat TMeshFloat> requires LesserFloat<TFloat, TMeshFloat>
    LoadedMeshes<TFloat> GeometryInterface<TSpectral, TFloat, TMeshFloat>::loadWithAssimp(vira::Scene<TSpectral, TFloat, TMeshFloat>& scene, const fs::path& filepath) {
        Assimp::Importer importer;

        // Configure ASSIMP post-processing flags
        unsigned int flags = aiProcess_Triangulate |
            aiProcess_GenNormals |
            aiProcess_GenUVCoords |
            aiProcess_CalcTangentSpace |
            aiProcess_JoinIdenticalVertices |
            aiProcess_ImproveCacheLocality |
            aiProcess_RemoveRedundantMaterials |
            aiProcess_FixInfacingNormals |
            aiProcess_FindDegenerates |
            aiProcess_FindInvalidData |
            aiProcess_ValidateDataStructure;

        // Load the ai_scene
        const aiScene* ai_scene = importer.ReadFile(filepath.string(), flags);

        if (!ai_scene || ai_scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !ai_scene->mRootNode) {
            throw std::runtime_error("ASSIMP Error: " + std::string(importer.GetErrorString()));
        }

        // Get the directory of the file for relative texture paths
        fs::path basePath = filepath.parent_path();

        // First pass: collect ALL nodes to preserve hierarchy structure
        std::vector<const aiNode*> all_nodes;
        std::unordered_map<const aiNode*, size_t> node_to_index;

        std::function<void(const aiNode*)> collect_all_nodes = [&](const aiNode* node) {
            node_to_index[node] = all_nodes.size();
            all_nodes.push_back(node);

            for (unsigned int i = 0; i < node->mNumChildren; ++i) {
                collect_all_nodes(node->mChildren[i]);
            }
            };

        collect_all_nodes(ai_scene->mRootNode);

        // Load materials first
        size_t totalMaterials = ai_scene->mNumMaterials;
        std::vector<vira::MaterialID> material_ids(totalMaterials);
        for (unsigned int i = 0; i < totalMaterials; ++i) {
            std::unique_ptr<vira::materials::Material<TSpectral>> new_material =
                createMaterialFromAssimp<TSpectral>(basePath, ai_scene->mMaterials[i], ai_scene, rgbToSpectral_function);

            aiString name;
            std::string material_name;
            if (ai_scene->mMaterials[i]->Get(AI_MATKEY_NAME, name) == AI_SUCCESS) {
                material_name = std::string(name.C_Str());
            }
            else {
                material_name = "Material_" + std::to_string(i);
            }

            material_ids[i] = scene.addMaterial(std::move(new_material), material_name);
        }

        // Count nodes that actually have meshes
        size_t mesh_count = 0;
        for (const aiNode* node : all_nodes) {
            if (node->mNumMeshes > 0) {
                mesh_count++;
            }
        }

        // Create LoadedMeshes with correct mesh count
        LoadedMeshes<TFloat> loaded_meshes(mesh_count);
        loaded_meshes.nodes.reserve(all_nodes.size());

        // Find root node index
        auto root_it = node_to_index.find(ai_scene->mRootNode);
        loaded_meshes.root_node = (root_it != node_to_index.end()) ? root_it->second : 0;

        // Create a mapping from node index to mesh indices for this node
        std::unordered_map<size_t, std::vector<size_t>> node_to_mesh_indices;
        size_t mesh_idx = 0;

        // Process all nodes to build the hierarchy
        for (size_t node_idx = 0; node_idx < all_nodes.size(); ++node_idx) {
            const aiNode* ai_node = all_nodes[node_idx];

            // Create node structure
            typename LoadedMeshes<TFloat>::Node node;
            node.name = ai_node->mName.length > 0 ? std::string(ai_node->mName.C_Str()) : ("Node_" + std::to_string(node_idx));
            node.local_transform = assimpToViraMat4<TFloat>(ai_node->mTransformation);

            // Set up parent-child relationships
            if (ai_node->mParent && node_to_index.find(ai_node->mParent) != node_to_index.end()) {
                node.parent = node_to_index[ai_node->mParent];
            }

            for (unsigned int i = 0; i < ai_node->mNumChildren; ++i) {
                auto child_it = node_to_index.find(ai_node->mChildren[i]);
                if (child_it != node_to_index.end()) {
                    node.children.push_back(child_it->second);
                }
            }

            // Create mesh if this node has geometry
            if (ai_node->mNumMeshes > 0) {
                // Count total vertices and faces for this node's meshes
                size_t totalVertices = 0;
                size_t totalFaces = 0;
                for (unsigned int meshIdx = 0; meshIdx < ai_node->mNumMeshes; ++meshIdx) {
                    const aiMesh* assimpMesh = ai_scene->mMeshes[ai_node->mMeshes[meshIdx]];
                    totalVertices += assimpMesh->mNumVertices;
                    totalFaces += assimpMesh->mNumFaces;
                }

                // Create unified buffers for this node's meshes
                VertexBuffer<TSpectral, TMeshFloat> vertexBuffer;
                IndexBuffer indexBuffer;
                std::vector<MaterialID::ValueType> faceMaterialIndices;

                vertexBuffer.reserve(totalVertices);
                indexBuffer.reserve(totalFaces * 3);
                faceMaterialIndices.reserve(totalFaces);

                uint32_t vertexOffset = 0;
                bool shade_smooth = false;
                std::set<MaterialID::ValueType> uniqueMaterialIndices;

                // Merge all ASSIMP meshes for this node
                for (unsigned int meshIdx = 0; meshIdx < ai_node->mNumMeshes; ++meshIdx) {
                    const aiMesh* assimpMesh = ai_scene->mMeshes[ai_node->mMeshes[meshIdx]];

                    if (assimpMesh->HasNormals()) {
                        shade_smooth = true;
                    }

                    // Process vertices
                    for (unsigned int v = 0; v < assimpMesh->mNumVertices; ++v) {
                        Vertex<TSpectral, TMeshFloat> vertex;

                        vertex.position = vec3<TMeshFloat>(
                            static_cast<TMeshFloat>(assimpMesh->mVertices[v].x),
                            static_cast<TMeshFloat>(assimpMesh->mVertices[v].y),
                            static_cast<TMeshFloat>(assimpMesh->mVertices[v].z)
                        );

                        if (assimpMesh->HasNormals()) {
                            vertex.normal = Normal(
                                assimpMesh->mNormals[v].x,
                                assimpMesh->mNormals[v].y,
                                assimpMesh->mNormals[v].z
                            );
                        }

                        if (assimpMesh->HasTextureCoords(0)) {
                            vertex.uv = UV(
                                assimpMesh->mTextureCoords[0][v].x,
                                1.f - assimpMesh->mTextureCoords[0][v].y
                            );
                        }

                        if (assimpMesh->HasVertexColors(0)) {
                            ColorRGB rgb(
                                assimpMesh->mColors[0][v].r,
                                assimpMesh->mColors[0][v].g,
                                assimpMesh->mColors[0][v].b
                            );
                            vertex.albedo = rgbToSpectral_function(rgb);
                        }

                        vertexBuffer.push_back(vertex);
                    }

                    // Process faces
                    MaterialID::ValueType materialIndex = static_cast<MaterialID::ValueType>(assimpMesh->mMaterialIndex);
                    uniqueMaterialIndices.insert(materialIndex);

                    for (unsigned int f = 0; f < assimpMesh->mNumFaces; ++f) {
                        const aiFace& face = assimpMesh->mFaces[f];

                        if (face.mNumIndices == 3) {
                            indexBuffer.push_back(face.mIndices[0] + vertexOffset);
                            indexBuffer.push_back(face.mIndices[1] + vertexOffset);
                            indexBuffer.push_back(face.mIndices[2] + vertexOffset);
                            faceMaterialIndices.push_back(materialIndex);
                        }
                    }

                    vertexOffset += assimpMesh->mNumVertices;
                }

                // Create material mapping for this mesh
                std::unordered_map<MaterialID::ValueType, MaterialID::ValueType> globalToLocalMaterialMap;
                std::vector<vira::MaterialID> localMaterialIDs;

                MaterialID::ValueType localMaterialIndex = 0;
                for (MaterialID::ValueType globalMaterialIndex : uniqueMaterialIndices) {
                    globalToLocalMaterialMap[globalMaterialIndex] = localMaterialIndex;
                    localMaterialIDs.push_back(material_ids[globalMaterialIndex]);
                    localMaterialIndex++;
                }

                // Remap face material indices
                for (auto& faceMatIndex : faceMaterialIndices) {
                    faceMatIndex = globalToLocalMaterialMap[faceMatIndex];
                }

                // Create the mesh
                auto new_mesh = std::make_unique<Mesh<TSpectral, TFloat, TMeshFloat>>(
                    std::move(vertexBuffer),
                    std::move(indexBuffer),
                    std::move(faceMaterialIndices)
                );
                new_mesh->setSmoothShading(shade_smooth);

                // Add mesh to scene
                vira::MeshID meshid = scene.addMesh(std::move(new_mesh), node.name);

                // Apply materials
                scene[meshid].material_cache_.resize(localMaterialIDs.size());
                scene[meshid].materialIDs_.resize(localMaterialIDs.size());

                for (size_t localIdx = 0; localIdx < localMaterialIDs.size(); ++localIdx) {
                    vira::MaterialID globalMaterialID = localMaterialIDs[localIdx];
                    scene[meshid].material_cache_[localIdx] = scene.materials_.at(globalMaterialID).data.get();
                    scene[meshid].materialIDs_[localIdx] = scene[meshid].material_cache_[localIdx]->getID();
                }

                // Store mesh reference in node
                node.mesh_indices.push_back(mesh_idx);
                node_to_mesh_indices[node_idx].push_back(mesh_idx);

                // Store in flat arrays
                loaded_meshes.mesh_ids[mesh_idx] = meshid;

                mesh_idx++; // Increment mesh index only when we actually create a mesh
            }

            loaded_meshes.nodes.push_back(std::move(node));
        }

        // Second pass: Calculate accumulated transforms for all meshes
        std::function<mat4<TFloat>(size_t)> calculateAccumulatedTransform =
            [&](size_t node_idx) -> vira::mat4<TFloat> {

            if (node_idx >= loaded_meshes.nodes.size()) {
                return vira::mat4<TFloat>(1); // Identity
            }

            const auto& node = loaded_meshes.nodes[node_idx];

            // If this is the root node (no parent), return its local transform
            if (node.parent == std::numeric_limits<size_t>::max()) {
                return node.local_transform;
            }

            // Recursively calculate parent's accumulated transform
            vira::mat4<TFloat> parent_transform = calculateAccumulatedTransform(node.parent);

            // Combine parent transform with local transform
            return parent_transform * node.local_transform;
            };

        // Fill in the accumulated transforms for all meshes
        for (size_t node_idx = 0; node_idx < loaded_meshes.nodes.size(); ++node_idx) {
            const auto& mesh_indices = node_to_mesh_indices[node_idx];

            if (!mesh_indices.empty()) {
                mat4<TFloat> accumulated_transform = calculateAccumulatedTransform(node_idx);

                for (size_t mesh_idx_ : mesh_indices) {
                    if (mesh_idx_ < loaded_meshes.transformations.size()) {
                        loaded_meshes.transformations[mesh_idx_] = accumulated_transform;
                    }
                }
            }
        }

        return loaded_meshes;
    }

    /**
     * @brief Loads DSK (Digital Shape Kernel) format files using SPICE.
     *
     * @param scene Target scene for loading
     * @param filepath Path to the DSK file
     * @return LoadedMeshes<TFloat> containing the loaded DSK mesh
     *
     * Loads planetary surface data from NASA SPICE DSK files. Applies coordinate
     * scaling (x1000) for unit conversion, extracts frame information for naming,
     * and creates a default Lambertian material with the configured DSK albedo.
     *
     * @throws SPICE error If DSK file is invalid or no segment is found
     * @note Requires SPICE library integration
     */
    template <IsSpectral TSpectral, IsFloat TFloat, IsFloat TMeshFloat> requires LesserFloat<TFloat, TMeshFloat>
    LoadedMeshes<TFloat> GeometryInterface<TSpectral, TFloat, TMeshFloat>::loadDSK(vira::Scene<TSpectral, TFloat, TMeshFloat>& scene, const fs::path& filepath) {
        // DSK implementation (copied from your existing DSK interface)
        SpiceBoolean   found;
        SpiceDLADescr  dladsc;
        SpiceDouble    verts[1][3];
        SpiceInt       handle;
        SpiceInt       n;
        SpiceInt       np;
        SpiceInt       nv;
        SpiceInt       nvtx;
        SpiceInt       plates[1][3];

        dasopr_c(filepath.string().c_str(), &handle);
        dlabfs_c(handle, &dladsc, &found);

        if (!found) {
            setmsg_c("No segment found in file #.");
            errch_c("#", filepath.string().c_str());
            sigerr_c("SPICE(NOSEGMENT)");
        }

        dskz02_c(handle, &dladsc, &nv, &np);

        size_t numPlates = static_cast<size_t>(np);
        size_t numVertices = static_cast<size_t>(nv);

        VertexBuffer<TSpectral, TMeshFloat> vertexBuffer(numVertices);
        IndexBuffer indexBuffer(3 * numPlates);

        size_t v_idx = 0;
        for (auto i = 0; i < nv; i++) {
            dskv02_c(handle, &dladsc, i + 1, 1, &nvtx, verts);
            Vertex<TSpectral, TMeshFloat> vertex;
            vertex.position = vec3<TMeshFloat>(1000 * verts[0][0], 1000 * verts[0][1], 1000 * verts[0][2]);
            vertexBuffer[v_idx] = vertex;
            v_idx++;
        }

        size_t i_idx = 0;
        for (auto i = 0; i < np; i++) {
            dskp02_c(handle, &dladsc, i + 1, 1, &n, plates);
            for (int j = 0; j < 3; j++) {
                indexBuffer[i_idx] = static_cast<uint32_t>(plates[0][static_cast<size_t>(j)] - 1);
                i_idx++;
            }
        }

        std::string meshName;
        SpiceDSKDescr dskdsc;
        SpiceChar frameStr[33] = { 0 };

        dskgd_c(handle, &dladsc, &dskdsc);
        frmnam_c(dskdsc.frmcde, 32, frameStr);

        if (strlen(frameStr) > 0 && strcmp(frameStr, "UNKNOWN") != 0) {
            meshName = std::string(frameStr);
        }
        else {
            meshName = filepath.stem().string();
        }

        dascls_c(handle);

        // Create the DSK material:
        auto new_material = std::make_unique<vira::materials::Lambertian<TSpectral>>();
        new_material->setAlbedo(dsk_albedo);
        vira::MaterialID material_id = scene.addMaterial(std::move(new_material), "DSK_Lambertian");

        // Add the mesh to the Scene:
        auto new_mesh = std::make_unique<Mesh<TSpectral, TFloat, TMeshFloat>>(vertexBuffer, indexBuffer);
        vira::MeshID meshid = scene.addMesh(std::move(new_mesh), meshName);

        // Add the default material:
        scene[meshid].material_cache_.resize(1);
        scene[meshid].materialIDs_.resize(1);
        scene[meshid].material_cache_[0] = scene.materials_.at(material_id).data.get();
        scene[meshid].materialIDs_[0] = scene[meshid].material_cache_[0]->getID();

        // Export the loaded mesh ID and a default transform:
        LoadedMeshes<TFloat> loaded_meshes(1);
        loaded_meshes.mesh_ids[0] = meshid;
        loaded_meshes.transformations[0] = vira::mat4<TFloat>(1);

        return loaded_meshes;
    }

    /**
     * @brief Saves a scene group to a file in the specified format.
     *
     * @param group The scene group to export
     * @param filepath Output file path (extension determines format if format is "AUTO")
     * @param format Export format ("AUTO" for automatic detection, or specific format like "OBJ")
     *
     * Exports the complete group hierarchy including meshes, materials, transformations,
     * and node structure. Currently supports OBJ format with plans for additional formats.
     *
     * @throws std::runtime_error If format is unsupported or export fails
     * @note Currently limited to OBJ export format
     */
    template <IsSpectral TSpectral, IsFloat TFloat, IsFloat TMeshFloat> requires LesserFloat<TFloat, TMeshFloat>
    void GeometryInterface<TSpectral, TFloat, TMeshFloat>::saveGroup(vira::scene::Group<TSpectral, TFloat, TMeshFloat>& group, const fs::path& filepath, std::string format)
    {
        // Create ASSIMP scene
        aiScene* ai_scene = new aiScene();

        // Detect format if auto
        std::string detected_format = detectFormat(filepath, format);

        // Limit output format based on what current works:
        if (!vira::utils::sameString(detected_format, "obj")) {
            throw std::runtime_error("Currently Vira only supports exporting OBJ files");
        }

        // Collect all unique meshes and materials referenced in the group hierarchy
        std::set<vira::MeshID> uniqueMeshIDs;
        std::set<vira::MaterialID> uniqueMaterialIDs;

        // Lambda to collect mesh and material IDs recursively
        std::function<void(const vira::scene::Group<TSpectral, TFloat, TMeshFloat>&)> collectIDs =
            [&](const vira::scene::Group<TSpectral, TFloat, TMeshFloat>& currentGroup) {
            // Collect mesh IDs from instances
            for (const auto& [instanceID, instanceData] : currentGroup.instances_) {
                vira::MeshID meshID = instanceData.data->getMeshID();
                uniqueMeshIDs.insert(meshID);

                // Collect material IDs from this mesh
                const auto& mesh = group.scene_->operator[](meshID);
                for (const auto& materialID : mesh.materialIDs_) {
                    uniqueMaterialIDs.insert(materialID);
                }
            }

            // Recursively process child groups
            for (const auto& [groupID, groupData] : currentGroup.groups_) {
                collectIDs(*groupData.data);
            }
            };

        collectIDs(group);

        // Create ASSIMP materials
        ai_scene->mNumMaterials = static_cast<unsigned int>(uniqueMaterialIDs.size());
        ai_scene->mMaterials = new aiMaterial * [ai_scene->mNumMaterials];

        std::unordered_map<vira::MaterialID, unsigned int> materialIDToIndex;
        unsigned int materialIndex = 0;

        for (const auto& materialID : uniqueMaterialIDs) {
            const vira::materials::Material<TSpectral>& material = group.scene_->operator[](materialID);

            ai_scene->mMaterials[materialIndex] = createAssimpMaterial<TSpectral>(material, group.scene_->getName(materialID), spectralToRGB_function);

            materialIDToIndex[materialID] = materialIndex;
            materialIndex++;
        }

        // Create ASSIMP meshes
        ai_scene->mNumMeshes = static_cast<unsigned int>(uniqueMeshIDs.size());
        ai_scene->mMeshes = new aiMesh * [ai_scene->mNumMeshes];

        std::unordered_map<vira::MeshID, unsigned int> meshIDToIndex;
        unsigned int meshIndex = 0;

        for (const auto& meshID : uniqueMeshIDs) {
            const auto& mesh = group.scene_->operator[](meshID);

            // Create ASSIMP mesh
            aiMesh* ai_mesh = new aiMesh();
            ai_mesh->mName = aiString(group.scene_->getName(meshID));

            // Set vertices
            const auto& vertexBuffer = mesh.getVertexBuffer();
            ai_mesh->mNumVertices = static_cast<unsigned int>(vertexBuffer.size());
            ai_mesh->mVertices = new aiVector3D[ai_mesh->mNumVertices];
            ai_mesh->mNormals = new aiVector3D[ai_mesh->mNumVertices];
            ai_mesh->mTextureCoords[0] = new aiVector3D[ai_mesh->mNumVertices];
            ai_mesh->mNumUVComponents[0] = 2;

            for (size_t i = 0; i < vertexBuffer.size(); ++i) {
                const auto& vertex = vertexBuffer[i];

                ai_mesh->mVertices[i] = aiVector3D(
                    static_cast<float>(vertex.position.x),
                    static_cast<float>(vertex.position.y),
                    static_cast<float>(vertex.position.z)
                );

                ai_mesh->mNormals[i] = aiVector3D(
                    vertex.normal.x,
                    vertex.normal.y,
                    vertex.normal.z
                );

                ai_mesh->mTextureCoords[0][i] = aiVector3D(
                    vertex.uv.x,
                    vertex.uv.y,
                    0.0f
                );
            }

            // Set faces
            const auto& indexBuffer = mesh.getIndexBuffer();
            const auto& faceMaterialIndices = mesh.getMaterialIndices();

            ai_mesh->mNumFaces = static_cast<unsigned int>(indexBuffer.size() / 3);
            ai_mesh->mFaces = new aiFace[ai_mesh->mNumFaces];

            for (size_t i = 0; i < ai_mesh->mNumFaces; ++i) {
                aiFace& face = ai_mesh->mFaces[i];
                face.mNumIndices = 3;
                face.mIndices = new unsigned int[3];
                face.mIndices[0] = indexBuffer[i * 3 + 0];
                face.mIndices[1] = indexBuffer[i * 3 + 1];
                face.mIndices[2] = indexBuffer[i * 3 + 2];
            }

            // Set material index (use first material for simplicity, or most common)
            if (!faceMaterialIndices.empty() && !mesh.materialIDs_.empty()) {
                vira::MaterialID firstMaterialID = mesh.materialIDs_[0];
                ai_mesh->mMaterialIndex = materialIDToIndex[firstMaterialID];
            }
            else {
                ai_mesh->mMaterialIndex = 0; // Default to first material
            }

            ai_scene->mMeshes[meshIndex] = ai_mesh;
            meshIDToIndex[meshID] = meshIndex;
            meshIndex++;
        }

        // Create ASSIMP node hierarchy
        std::function<aiNode* (const vira::scene::Group<TSpectral, TFloat, TMeshFloat>&, const std::string&)> createNode =
            [&](const vira::scene::Group<TSpectral, TFloat, TMeshFloat>& currentGroup, const std::string& nodeName) -> aiNode* {

            aiNode* ai_node = new aiNode();
            ai_node->mName = aiString(nodeName);

            // Set transform
            auto transform_matrix = currentGroup.getModelMatrix();

            // Convert to ASSIMP matrix (transpose because ASSIMP uses row-major)
            for (unsigned int i = 0; i < 4; ++i) {
                for (unsigned int j = 0; j < 4; ++j) {
                    int ii = static_cast<int>(i);
                    int jj = static_cast<int>(j);
                    ai_node->mTransformation[i][j] = static_cast<float>(transform_matrix[jj][ii]);
                }
            }

            // Collect mesh indices for instances in this group
            std::vector<unsigned int> nodeMeshIndices;
            for (const auto& [instanceID, instanceData] : currentGroup.instances_) {
                vira::MeshID meshID = instanceData.data->getMeshID();
                auto it = meshIDToIndex.find(meshID);
                if (it != meshIDToIndex.end()) {
                    nodeMeshIndices.push_back(it->second);
                }
            }

            // Set meshes for this node
            if (!nodeMeshIndices.empty()) {
                ai_node->mNumMeshes = static_cast<unsigned int>(nodeMeshIndices.size());
                ai_node->mMeshes = new unsigned int[ai_node->mNumMeshes];
                for (size_t i = 0; i < nodeMeshIndices.size(); ++i) {
                    ai_node->mMeshes[i] = nodeMeshIndices[i];
                }
            }

            // Create child nodes
            if (!currentGroup.groups_.empty()) {
                ai_node->mNumChildren = static_cast<unsigned int>(currentGroup.groups_.size());
                ai_node->mChildren = new aiNode * [ai_node->mNumChildren];

                unsigned int childIndex = 0;
                for (const auto& [groupID, groupData] : currentGroup.groups_) {
                    aiNode* childNode = createNode(*groupData.data, groupData.name);
                    childNode->mParent = ai_node;
                    ai_node->mChildren[childIndex] = childNode;
                    childIndex++;
                }
            }

            return ai_node;
            };

        // Create root node
        std::string rootName = "RootNode"; // You might want to get this from the group
        ai_scene->mRootNode = createNode(group, rootName);

        // Export the scene
        Assimp::Exporter exporter;

        // Get appropriate format ID for ASSIMP
        std::string formatID;
        if (detected_format == "OBJ") formatID = "obj";
        else if (detected_format == "PLY") formatID = "ply";
        else if (detected_format == "GLTF") formatID = "gltf2";
        else if (detected_format == "GLB") formatID = "glb2";
        else if (detected_format == "FBX") formatID = "fbx";
        else if (detected_format == "DAE") formatID = "collada";
        else {
            delete ai_scene;
            throw std::runtime_error("Unsupported export format: " + detected_format);
        }

        // Perform export
        aiReturn result = exporter.Export(ai_scene, formatID, filepath.string());

        // Clean up
        delete ai_scene;

        if (result != AI_SUCCESS) {
            std::cout << "ASSIMP Export failed: " + std::string(exporter.GetErrorString()) + "\n";
            throw std::runtime_error("ASSIMP Export failed: " + std::string(exporter.GetErrorString()));
        }
    }

    /**
     * @brief Sets the RGB to spectral conversion function.
     *
     * @param func Function that converts RGB colors to spectral representation
     *
     * This function is used when loading materials with RGB color data to convert
     * them to the spectral domain. Must be a valid, non-null function.
     *
     * @throws std::runtime_error If the provided function is null
     */
    template <IsSpectral TSpectral, IsFloat TFloat, IsFloat TMeshFloat> requires LesserFloat<TFloat, TMeshFloat>
    void GeometryInterface<TSpectral, TFloat, TMeshFloat>::setRgbToSpectralFunction(std::function<TSpectral(ColorRGB)> func) {
        rgbToSpectral_function = func;
        if (rgbToSpectral_function == nullptr) {
            throw std::runtime_error("rgbToSpectral_function must be valid and non-null!");
        }
    }

    /**
     * @brief Sets the default albedo for DSK format meshes.
     *
     * @param albedo The spectral albedo value to use for DSK meshes
     *
     * DSK files typically don't contain material information, so this sets
     * the default Lambertian albedo applied to all DSK geometry.
     */
    template <IsSpectral TSpectral, IsFloat TFloat, IsFloat TMeshFloat> requires LesserFloat<TFloat, TMeshFloat>
    void GeometryInterface<TSpectral, TFloat, TMeshFloat>::setDSKAlbedo(const TSpectral& albedo) {
        dsk_albedo = albedo;
    }
}