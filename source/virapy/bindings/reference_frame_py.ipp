#ifndef VIRAPY_REFERENCE_FRAME_PY
#define VIRAPY_REFERENCE_FRAME_PY

#include <string>

#include "pybind11/pybind11.h"
#include "pybind11/stl.h"
#include "pybind11/operators.h"

#include "vira/constraints.hpp"
#include "vira/reference_frame.hpp"

namespace py = pybind11;

namespace vira {
    template <IsFloat TFloat>
    void bind_reference_frame(py::module& m, const std::string& suffix) {
        py::class_<ReferenceFrame<TFloat>>(m, ("ReferenceFrame_" + suffix).c_str())
            .def(py::init<>(), "Default constructor")

            // Local position methods
            .def(("setLocalPosition_" + suffix).c_str(),
                py::overload_cast<vec3<TFloat>>(&ReferenceFrame<TFloat>::setLocalPosition),
                "Set local position using vec3",
                py::arg("position"))
            .def(("setLocalPosition_" + suffix).c_str(),
                py::overload_cast<double, double, double>(&ReferenceFrame<TFloat>::setLocalPosition),
                "Set local position using x, y, z coordinates",
                py::arg("x"), py::arg("y"), py::arg("z"))
            .def(("localTranslateBy_" + suffix).c_str(),
                py::overload_cast<const vec3<TFloat>&>(&ReferenceFrame<TFloat>::localTranslateBy),
                "Translate by a vector",
                py::arg("translation"))
            .def(("localTranslateBy_" + suffix).c_str(),
                py::overload_cast<double, double, double>(&ReferenceFrame<TFloat>::localTranslateBy),
                "Translate by x, y, z values",
                py::arg("x"), py::arg("y"), py::arg("z"))
            .def(("getLocalPosition_" + suffix).c_str(), &ReferenceFrame<TFloat>::getLocalPosition,
                "Get local position")
            .def(("getGlobalPosition_" + suffix).c_str(), &ReferenceFrame<TFloat>::getGlobalPosition,
                "Get global position")

            // Local rotation methods
            .def(("setLocalRotation_" + suffix).c_str(),
                py::overload_cast<Rotation<TFloat>>(&ReferenceFrame<TFloat>::setLocalRotation),
                "Set local rotation using Rotation object",
                py::arg("rotation"))
            .def(("setLocalRotation_" + suffix).c_str(),
                py::overload_cast<mat3<TFloat>>(&ReferenceFrame<TFloat>::setLocalRotation),
                "Set local rotation using rotation matrix",
                py::arg("rotation"))
            .def(("setLocalEulerAngles_" + suffix).c_str(),
                &ReferenceFrame<TFloat>::setLocalEulerAngles,
                "Set local rotation using Euler angles",
                py::arg("r1"), py::arg("r2"), py::arg("r3"), py::arg("sequence") = "XYZ")
            .def(("setLocalQuaternion_" + suffix).c_str(),
                &ReferenceFrame<TFloat>::setLocalQuaternion,
                "Set local rotation using quaternion",
                py::arg("quaternion"))
            .def(("setLocalShusterQuaternion_" + suffix).c_str(),
                &ReferenceFrame<TFloat>::setLocalShusterQuaternion,
                "Set local rotation using Shuster quaternion",
                py::arg("quaternion"))
            .def(("setLocalAxisAngle_" + suffix).c_str(),
                &ReferenceFrame<TFloat>::setLocalAxisAngle,
                "Set local rotation using axis-angle representation",
                py::arg("axis"), py::arg("angle"))
            .def(("localRotateBy_" + suffix).c_str(),
                &ReferenceFrame<TFloat>::localRotateBy,
                "Rotate by a given rotation",
                py::arg("rotate"))
            .def(("getLocalRotation_" + suffix).c_str(), &ReferenceFrame<TFloat>::getLocalRotation,
                "Get local rotation")
            .def(("getGlobalRotation_" + suffix).c_str(), &ReferenceFrame<TFloat>::getGlobalRotation,
                "Get global rotation")

            // Local scale methods
            .def(("setLocalScale_" + suffix).c_str(),
                py::overload_cast<vec3<TFloat>>(&ReferenceFrame<TFloat>::setLocalScale),
                "Set local scale using vec3",
                py::arg("scale"))
            .def(("setLocalScale_" + suffix).c_str(),
                py::overload_cast<double, double, double>(&ReferenceFrame<TFloat>::setLocalScale),
                "Set local scale using sx, sy, sz values",
                py::arg("sx"), py::arg("sy"), py::arg("sz"))
            .def(("setLocalScale_" + suffix).c_str(),
                py::overload_cast<double>(&ReferenceFrame<TFloat>::setLocalScale),
                "Set uniform local scale",
                py::arg("scale"))
            .def(("localScaleBy_" + suffix).c_str(),
                py::overload_cast<vec3<TFloat>>(&ReferenceFrame<TFloat>::localScaleBy),
                "Scale by a vector",
                py::arg("scaling"))
            .def(("localScaleBy_" + suffix).c_str(),
                py::overload_cast<double, double, double>(&ReferenceFrame<TFloat>::localScaleBy),
                "Scale by sx, sy, sz values",
                py::arg("sx"), py::arg("sy"), py::arg("sz"))
            .def(("localScaleBy_" + suffix).c_str(),
                py::overload_cast<double>(&ReferenceFrame<TFloat>::localScaleBy),
                "Scale by uniform factor",
                py::arg("scaling"))
            .def(("getLocalScale_" + suffix).c_str(), &ReferenceFrame<TFloat>::getLocalScale,
                "Get local scale")
            .def(("getGlobalScale_" + suffix).c_str(), &ReferenceFrame<TFloat>::getGlobalScale,
                "Get global scale")

            // Local velocity methods
            .def(("setLocalVelocity_" + suffix).c_str(),
                py::overload_cast<vec3<TFloat>>(&ReferenceFrame<TFloat>::setLocalVelocity),
                "Set local velocity using vec3",
                py::arg("velocity"))
            .def(("setLocalVelocity_" + suffix).c_str(),
                py::overload_cast<double, double, double>(&ReferenceFrame<TFloat>::setLocalVelocity),
                "Set local velocity using vx, vy, vz values",
                py::arg("vx"), py::arg("vy"), py::arg("vz"))
            .def(("addLocalVelocity_" + suffix).c_str(),
                py::overload_cast<vec3<TFloat>>(&ReferenceFrame<TFloat>::addLocalVelocity),
                "Add to local velocity using vec3",
                py::arg("velocity_addition"))
            .def(("addLocalVelocity_" + suffix).c_str(),
                py::overload_cast<double, double, double>(&ReferenceFrame<TFloat>::addLocalVelocity),
                "Add to local velocity using vx, vy, vz values",
                py::arg("vx"), py::arg("vy"), py::arg("vz"))
            .def(("getLocalVelocity_" + suffix).c_str(), &ReferenceFrame<TFloat>::getLocalVelocity,
                "Get local velocity", py::return_value_policy::reference_internal)
            .def(("getGlobalVelocity_" + suffix).c_str(), &ReferenceFrame<TFloat>::getGlobalVelocity,
                "Get global velocity", py::return_value_policy::reference_internal)

            // Local angular rate methods
            .def(("setLocalAngularRate_" + suffix).c_str(),
                py::overload_cast<vec3<TFloat>>(&ReferenceFrame<TFloat>::setLocalAngularRate),
                "Set local angular rate using vec3",
                py::arg("angular_rate"))
            .def(("setLocalAngularRate_" + suffix).c_str(),
                py::overload_cast<double, double, double>(&ReferenceFrame<TFloat>::setLocalAngularRate),
                "Set local angular rate using wx, wy, wz values",
                py::arg("wx"), py::arg("wy"), py::arg("wz"))
            .def(("addLocalAngularRate_" + suffix).c_str(),
                py::overload_cast<vec3<TFloat>>(&ReferenceFrame<TFloat>::addLocalAngularRate),
                "Add to local angular rate using vec3",
                py::arg("rate_addition"))
            .def(("addLocalAngularRate_" + suffix).c_str(),
                py::overload_cast<double, double, double>(&ReferenceFrame<TFloat>::addLocalAngularRate),
                "Add to local angular rate using wx, wy, wz values",
                py::arg("wx"), py::arg("wy"), py::arg("wz"))
            .def(("getLocalAngularRate_" + suffix).c_str(), &ReferenceFrame<TFloat>::getLocalAngularRate,
                "Get local angular rate", py::return_value_policy::reference_internal)
            .def(("getGlobalAngularRate_" + suffix).c_str(), &ReferenceFrame<TFloat>::getGlobalAngularRate,
                "Get global angular rate", py::return_value_policy::reference_internal)

            // Combination setters
            .def(("setLocalPositionRotation_" + suffix).c_str(),
                &ReferenceFrame<TFloat>::setLocalPositionRotation,
                "Set local position and rotation",
                py::arg("position"), py::arg("rotation"))
            .def(("setLocalPositionRotationScale_" + suffix).c_str(),
                &ReferenceFrame<TFloat>::setLocalPositionRotationScale,
                "Set local position, rotation, and scale",
                py::arg("position"), py::arg("rotation"), py::arg("scale"))
            .def(("setLocalTransformation_" + suffix).c_str(),
                &ReferenceFrame<TFloat>::setLocalTransformation,
                "Set local transformation matrix",
                py::arg("local_transformation"))

            // Transformation matrices
            .def(("getParentTransformationMatrix_" + suffix).c_str(),
                &ReferenceFrame<TFloat>::getParentTransformationMatrix,
                "Get parent transformation matrix")
            .def(("getLocalTransformationMatrix_" + suffix).c_str(),
                &ReferenceFrame<TFloat>::getLocalTransformationMatrix,
                "Get local transformation matrix")
            .def(("getGlobalTransformationMatrix_" + suffix).c_str(),
                &ReferenceFrame<TFloat>::getGlobalTransformationMatrix,
                "Get global transformation matrix")
            .def(("getModelMatrix_" + suffix).c_str(),
                &ReferenceFrame<TFloat>::getModelMatrix,
                "Get model matrix", py::return_value_policy::reference_internal)
            .def(("getModelNormalMatrix_" + suffix).c_str(),
                &ReferenceFrame<TFloat>::getModelNormalMatrix,
                "Get model normal matrix")
            .def(("getLocalZDir_" + suffix).c_str(),
                &ReferenceFrame<TFloat>::getLocalZDir,
                "Get local Z direction")
            .def(("getGlobalZDir_" + suffix).c_str(),
                &ReferenceFrame<TFloat>::getGlobalZDir,
                "Get global Z direction")

            // Transformations
            .def(("globalToLocal_" + suffix).c_str(),
                &ReferenceFrame<TFloat>::globalToLocal,
                "Transform point from global to local coordinates",
                py::arg("global_point"))
            .def(("localToGlobal_" + suffix).c_str(),
                &ReferenceFrame<TFloat>::localToGlobal,
                "Transform point from local to global coordinates",
                py::arg("local_point"))
            .def(("globalDirectionToLocal_" + suffix).c_str(),
                &ReferenceFrame<TFloat>::globalDirectionToLocal,
                "Transform direction from global to local",
                py::arg("global_direction"))
            .def(("localDirectionToGlobal_" + suffix).c_str(),
                &ReferenceFrame<TFloat>::localDirectionToGlobal,
                "Transform direction from local to global",
                py::arg("local_direction"))
            .def(("localPointToGlobalVelocity_" + suffix).c_str(),
                &ReferenceFrame<TFloat>::localPointToGlobalVelocity,
                "Convert local point to global velocity",
                py::arg("local_point"))
            .def(("globalPointToLocalVelocity_" + suffix).c_str(),
                &ReferenceFrame<TFloat>::globalPointToLocalVelocity,
                "Convert global point to local velocity",
                py::arg("global_point"))
            .def(("globalToLocalVelocity_" + suffix).c_str(),
                &ReferenceFrame<TFloat>::globalToLocalVelocity,
                "Transform velocity from global to local",
                py::arg("global_velocity"))
            .def(("localToGlobalVelocity_" + suffix).c_str(),
                &ReferenceFrame<TFloat>::localToGlobalVelocity,
                "Transform velocity from local to global",
                py::arg("local_velocity"))

            // SPICE configuration
            .def(("configureSPICE_" + suffix).c_str(),
                &ReferenceFrame<TFloat>::configureSPICE,
                "Configure SPICE frame",
                py::arg("naif_name"), py::arg("frame_name"))
            .def(("setNAIFID_" + suffix).c_str(),
                &ReferenceFrame<TFloat>::setNAIFID,
                "Set NAIF ID",
                py::arg("naif_name"))
            .def(("setFrameName_" + suffix).c_str(),
                &ReferenceFrame<TFloat>::setFrameName,
                "Set frame name",
                py::arg("frame_name"))
            .def(("isConfiguredSpiceObject_" + suffix).c_str(),
                &ReferenceFrame<TFloat>::isConfiguredSpiceObject,
                "Check if configured as SPICE object")
            .def(("isConfiguredSpiceFrame_" + suffix).c_str(),
                &ReferenceFrame<TFloat>::isConfiguredSpiceFrame,
                "Check if configured as SPICE frame")
            .def(("getNAIFName_" + suffix).c_str(),
                &ReferenceFrame<TFloat>::getNAIFName,
                "Get NAIF name", py::return_value_policy::reference_internal)
            .def(("getFrameName_" + suffix).c_str(),
                &ReferenceFrame<TFloat>::getFrameName,
                "Get frame name", py::return_value_policy::reference_internal)

            // Static helper methods
            .def_static(("makeTransformationMatrix_" + suffix).c_str(),
                &ReferenceFrame<TFloat>::makeTransformationMatrix,
                "Create transformation matrix from position, rotation, and scale",
                py::arg("position"), py::arg("rotation"), py::arg("scale"))
            .def_static(("getPositionFromTransformation_" + suffix).c_str(),
                &ReferenceFrame<TFloat>::getPositionFromTransformation,
                "Extract position from transformation matrix",
                py::arg("transformation"))
            .def_static(("getRotationFromTransformation_" + suffix).c_str(),
                py::overload_cast<mat4<TFloat>>(&ReferenceFrame<TFloat>::getRotationFromTransformation),
                "Extract rotation from transformation matrix",
                py::arg("transformation"))
            .def_static(("getRotationFromTransformation_" + suffix).c_str(),
                py::overload_cast<mat4<TFloat>, vec3<TFloat>>(&ReferenceFrame<TFloat>::getRotationFromTransformation),
                "Extract rotation from transformation matrix with scale",
                py::arg("transformation"), py::arg("scale"))
            .def_static(("getScaleFromTransformation_" + suffix).c_str(),
                &ReferenceFrame<TFloat>::getScaleFromTransformation,
                "Extract scale from transformation matrix",
                py::arg("transformation"));
    }
}

#endif