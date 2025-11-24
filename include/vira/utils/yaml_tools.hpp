#ifndef VIRA_UTILS_YAML_TOOLS_HPP
#define VIRA_UTILS_YAML_TOOLS_HPP

#include <stdexcept>
#include <string>

// TODO REMOVE THIRD PARTY HEADERS:
#ifdef _WIN32
#define YAML_CPP_API
#endif
#include "yaml-cpp/yaml.h"

#include "vira/vec.hpp"
#include "vira/rotation.hpp"
#include "vira/images/resolution.hpp"
#include "vira/constraints.hpp"

namespace YAML {
    template<int N, typename T, glm::qualifier Q>
    struct convert<glm::vec<N, T, Q>> {
        static Node encode(const glm::vec<N, T, Q>& rhs) {
            Node node;
            for (int i = 0; i < N; ++i) {
                node.push_back(rhs[i]);
            }
            return node;
        }

        static bool decode(const Node& node, glm::vec<N, T, Q>& rhs) {
            if (node.IsScalar()) {
                rhs = glm::vec<N, T, Q>{ node.as<T>() };
                return true;
            }
            else if (node.IsSequence() && (node.size() == N)) {
                for (int i = 0; i < N; ++i) {
                    rhs[i] = node[i].as<T>();
                }
                return true;
            }
            else {
                return false;
            }
        }
    };

    template<int C, int R, typename T, glm::qualifier Q>
    struct convert<glm::mat<C, R, T, Q>> {
        static Node encode(const glm::mat<C, R, T, Q>& rhs) {
            Node node;
            for (int i = 0; i < R; ++i) {

                Node row;
                for (int j = 0; j < C; ++j) {
                    row.push_back(rhs[j][i]);
                }
                node.push_back(row);
            }
            return node;
        }

        static bool decode(const Node& node, glm::mat<C, R, T, Q>& rhs) {
            if (!node.IsSequence() || node.size() != R) {
                return false;
            }

            for (int i = 0; i < R; ++i) {
                const Node& row = node[i];
                if (!row.IsSequence() || row.size() != C) {
                    return false;
                }

                for (int j = 0; j < C; ++j) {
                    rhs[j][i] = row[j].as<T>();
                }
            }
            return true;
        }
    };

    template<>
    struct convert<vira::images::Resolution> {
        static Node encode(const vira::images::Resolution& rhs) {
            Node node;
            node.push_back(rhs.x);
            node.push_back(rhs.y);
            return node;
        }

        static bool decode(const Node& node, vira::images::Resolution& rhs) {
            if (!node.IsSequence() || node.size() != 2) {
                throw YAML::Exception(node.Mark(), "YAML Parsing: Invalid Resolution");
            }
            rhs.x = node[0].as<int>();
            rhs.y = node[1].as<int>();
            return true;
        }
    };
};


namespace vira::utils {
    static inline bool readBoolean(const YAML::Node& node, const std::string& fieldName, bool defaultResult = false)
    {
        if (node[fieldName]) {
            return node[fieldName].as<bool>();
        }
        return defaultResult;
    };

    template<int N, typename T, glm::qualifier Q>
    glm::vec<N, T, Q> readVec(const YAML::Node& node, const std::string& fieldName, glm::vec<N, T, Q> defaultResult = glm::vec<N, T, Q>{ 0 })
    {
        if (node[fieldName]) {
            return node[fieldName].as<glm::vec<N, T, Q>>();
        }
        return defaultResult;
    };

    template<int C, int R, typename T, glm::qualifier Q>
    glm::mat<C, R, T, Q> readMat(const YAML::Node& node, const std::string& fieldName, glm::mat<C, R, T, Q> defaultResult = glm::mat<C, R, T, Q>{ 1 })
    {
        if (node[fieldName]) {
            return node[fieldName].as<glm::mat<C, R, T, Q>>();
        }
        return defaultResult;
    };

    template <vira::IsFloat TFloat>
    Rotation<TFloat> readRotation(const YAML::Node& rotation_node, Rotation<TFloat> defaultResult = Rotation<TFloat>{})
    {
        if (rotation_node["rotation"]) {
            YAML::Node formatNode = rotation_node["rotation"];
            if (formatNode["quaternion"]) {
                vec4<TFloat> quaternion = readVec(formatNode, "quaternion", vec4<TFloat>{0, 0, 0, 1});
                return Rotation<TFloat>::quaternion(quaternion);
            }
            else if (formatNode["euler"]) {
                std::string sequence = "321";
                TFloat xAngle;
                TFloat yAngle;
                TFloat zAngle;
                if (formatNode["euler"].size() == 4) {
                    sequence = formatNode["euler"][0].as<std::string>();
                    xAngle = formatNode["euler"][1].as<TFloat>();
                    yAngle = formatNode["euler"][2].as<TFloat>();
                    zAngle = formatNode["euler"][3].as<TFloat>();
                }
                else if (formatNode["euler"].size() == 3) {
                    xAngle = formatNode["euler"][0].as<TFloat>();
                    yAngle = formatNode["euler"][1].as<TFloat>();
                    zAngle = formatNode["euler"][2].as<TFloat>();
                }
                else {
                    throw YAML::Exception(formatNode.Mark(), "YAML Parsing: Invalid euler angle definition");
                }
                
                return Rotation<TFloat>::eulerAngles(xAngle, yAngle, zAngle, sequence);
            }
            else if (formatNode["matrix"]) {
                mat3<TFloat> rotmat = readMat(formatNode, "matrix", mat3<TFloat>{1});
                return Rotation<TFloat>(rotmat);
            }
            else {
                throw YAML::Exception(formatNode.Mark(), "YAML Parsing: Invalid rotation definition");
            }
        }
        return defaultResult;
    };
}


#endif