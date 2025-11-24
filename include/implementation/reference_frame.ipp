#include <stdexcept>
#include <ostream>
#include <iomanip>

#include "vira/vec.hpp"
#include "vira/constraints.hpp"
#include "vira/units/units.hpp"
#include "vira/utils/yaml_tools.hpp"
#include "vira/spice_utils.hpp"
#include "vira/utils/valid_value.hpp"

namespace vira {
    // ================================ //
    // === Local Position Modifiers === //
    // ================================ //
    template <IsFloat TFloat>
    void ReferenceFrame<TFloat>::setLocalPosition(vec3<TFloat> position)
    {
        if (!vira::utils::validVec(position)) {
            throw std::runtime_error("Invalid Position (contains NaN or INF)");
        }
        local_position_ = position;
        updateTransformations();
        onReferenceFrameChanged();
    }

    template <IsFloat TFloat>
    void ReferenceFrame<TFloat>::setLocalPosition(double x, double y, double z)
    {
        vec3<TFloat> new_position{
            static_cast<TFloat>(x),
            static_cast<TFloat>(y),
            static_cast<TFloat>(z)
        };
        setLocalPosition(new_position);
    };

    template <IsFloat TFloat>
    void ReferenceFrame<TFloat>::localTranslateBy(const vec3<TFloat>& translation)
    {
        setLocalPosition(local_position_ + translation);
    };

    template <IsFloat TFloat>
    void ReferenceFrame<TFloat>::localTranslateBy(double x, double y, double z)
    {
        vec3<TFloat> translation{
            static_cast<TFloat>(x),
            static_cast<TFloat>(y),
            static_cast<TFloat>(z)
        };
        localTranslateBy(translation);
    };



    // ================================ //
    // === Local Rotation Modifiers === //
    // ================================ //
    template <IsFloat TFloat>
    void ReferenceFrame<TFloat>::setLocalRotation(Rotation<TFloat> rotation)
    {
        local_rotation_ = rotation;
        updateTransformations();
        onReferenceFrameChanged();
    };

    template <IsFloat TFloat>
    void ReferenceFrame<TFloat>::setLocalRotation(mat3<TFloat> rotation)
    {
        setLocalRotation(Rotation<TFloat>(rotation));
    };

    template <IsFloat TFloat>
    void ReferenceFrame<TFloat>::setLocalEulerAngles(units::Degree r1, units::Degree r2, units::Degree r3, std::string sequence)
    {
        setLocalRotation(Rotation<TFloat>::eulerAngles(r1, r2, r3, sequence));
    };

    template <IsFloat TFloat>
    void ReferenceFrame<TFloat>::setLocalQuaternion(vec4<TFloat> quaternion)
    {
        setLocalRotation(Rotation<TFloat>::quaternion(quaternion));
    };

    template <IsFloat TFloat>
    void ReferenceFrame<TFloat>::setLocalShusterQuaternion(vec4<TFloat> quaternion)
    {
        setLocalRotation(Rotation<TFloat>::shusterQuaternion(quaternion));
    };

    template <IsFloat TFloat>
    void ReferenceFrame<TFloat>::setLocalAxisAngle(vec3<TFloat> axis, units::Degree angle)
    {
        setLocalRotation(Rotation<TFloat>::axisAngle(axis, angle));
    };

    template <IsFloat TFloat>
    void ReferenceFrame<TFloat>::localRotateBy(const Rotation<TFloat>& rotate)
    {
        setLocalRotation(rotate * local_rotation_);
    };



    // ============================= //
    // === Local Scale Modifiers === //
    // ============================= //
    template <IsFloat TFloat>
    void ReferenceFrame<TFloat>::setLocalScale(vec3<TFloat> scale)
    {
        if (!vira::utils::validVec(scale)) {
            throw std::runtime_error("Invalid Scale (contains NaN or INF)");
        }
        local_scale_ = scale;
        updateTransformations();
        onReferenceFrameChanged();
    };

    template <IsFloat TFloat>
    void ReferenceFrame<TFloat>::setLocalScale(double sx, double sy, double sz)
    {
        vec3<TFloat> new_scale{
            static_cast<TFloat>(sx),
            static_cast<TFloat>(sy),
            static_cast<TFloat>(sz)
        };
        setLocalScale(new_scale);
    };

    template <IsFloat TFloat>
    void ReferenceFrame<TFloat>::setLocalScale(double scale)
    {
        vec3<TFloat> new_scale{
            static_cast<TFloat>(scale),
            static_cast<TFloat>(scale),
            static_cast<TFloat>(scale)
        };
        setLocalScale(new_scale);
    };

    template <IsFloat TFloat>
    void ReferenceFrame<TFloat>::localScaleBy(vec3<TFloat> scaling)
    {
        setLocalScale(scaling * local_scale_);
    };

    template <IsFloat TFloat>
    void ReferenceFrame<TFloat>::localScaleBy(double sx, double sy, double sz)
    {
        vec3<TFloat> scaling{
            static_cast<TFloat>(sx),
            static_cast<TFloat>(sy),
            static_cast<TFloat>(sz)
        };
        localScaleBy(scaling);
    };

    template <IsFloat TFloat>
    void ReferenceFrame<TFloat>::localScaleBy(double scaling)
    {
        vec3<TFloat> scaling3{
            static_cast<TFloat>(scaling),
            static_cast<TFloat>(scaling),
            static_cast<TFloat>(scaling)
        };
        localScaleBy(scaling3);
    };



    // ================================ //
    // === Local Velocity Modifiers === //
    // ================================ //
    template <IsFloat TFloat>
    void ReferenceFrame<TFloat>::setLocalVelocity(vec3<TFloat> velocity)
    {
        if (!vira::utils::validVec(velocity)) {
            throw std::runtime_error("Invalid Velocity (contains NaN or INF)");
        }
        local_velocity_ = velocity;
        updateGlobalVelocities();
        onReferenceFrameChanged();
    };

    template <IsFloat TFloat>
    void ReferenceFrame<TFloat>::setLocalVelocity(double vx, double vy, double vz)
    {
        vec3<TFloat> velocity{
            static_cast<TFloat>(vx),
            static_cast<TFloat>(vy),
            static_cast<TFloat>(vz)
        };
        setLocalVelocity(velocity);
    };

    template <IsFloat TFloat>
    void ReferenceFrame<TFloat>::addLocalVelocity(vec3<TFloat> velocity_addition)
    {
        setLocalVelocity(local_velocity_ + velocity_addition);
    };

    template <IsFloat TFloat>
    void ReferenceFrame<TFloat>::addLocalVelocity(double vx, double vy, double vz)
    {
        vec3<TFloat> velocity_addition{
            static_cast<TFloat>(vx),
            static_cast<TFloat>(vy),
            static_cast<TFloat>(vz)
        };
        addLocalVelocity(velocity_addition);
    };


    // ==================================== //
    // === Local Angular Rate Modifiers === //
    // ==================================== //
    template <IsFloat TFloat>
    void ReferenceFrame<TFloat>::setLocalAngularRate(vec3<TFloat> angular_rate)
    {
        if (!vira::utils::validVec(angular_rate)) {
            throw std::runtime_error("Invalid Angular Rate (contains NaN or INF)");
        }
        local_angular_rate_ = angular_rate;
        updateGlobalVelocities();
        onReferenceFrameChanged();
    };

    template <IsFloat TFloat>
    void ReferenceFrame<TFloat>::setLocalAngularRate(double wx, double wy, double wz)
    {
        setLocalAngularRate(vec3<TFloat>{wx, wy, wz});
    };

    template <IsFloat TFloat>
    void ReferenceFrame<TFloat>::addLocalAngularRate(vec3<TFloat> rate_addition)
    {
        setLocalAngularRate(local_angular_rate_ + rate_addition);
    };

    template <IsFloat TFloat>
    void ReferenceFrame<TFloat>::addLocalAngularRate(double wx, double wy, double wz)
    {
        vec3<TFloat> rate_addition{
            static_cast<TFloat>(wx),
            static_cast<TFloat>(wy),
            static_cast<TFloat>(wz)
        };
        addLocalAngularRate(rate_addition);
    };



    // =========================== //
    // === Combination Setters === //
    // =========================== //
    template <IsFloat TFloat>
    void ReferenceFrame<TFloat>::setLocalPositionRotation(vec3<TFloat> position, Rotation<TFloat> rotation)
    {
        local_position_ = position;
        local_rotation_ = rotation;
        updateTransformations();
        onReferenceFrameChanged();
    };


    template <IsFloat TFloat>
    void ReferenceFrame<TFloat>::setLocalPositionRotationScale(vec3<TFloat> position, Rotation<TFloat> rotation, vec3<TFloat> scale)
    {
        local_position_ = position;
        local_rotation_ = rotation;
        local_scale_ = scale;
        updateTransformations();
        onReferenceFrameChanged();
    };

    template <IsFloat TFloat>
    void ReferenceFrame<TFloat>::setLocalTransformation(mat4<TFloat> local_transformation)
    {
        local_transformation_ = local_transformation;
        extractLocalComponents();
        updateGlobalTransformation();
        onReferenceFrameChanged();
    };


    /**
    * @brief Get the local zDirection of the current ReferenceFrame
    *
    * @return vec3<TFloat> The local z-direction
    */
    template <IsFloat TFloat>
    vec3<TFloat> ReferenceFrame<TFloat>::getLocalZDir() const
    {
        mat3<TFloat> rotationMatrix = local_rotation_.getMatrix();
        return vec3<TFloat>{ rotationMatrix[2][0], rotationMatrix[2][1], rotationMatrix[2][2] };
    };

    /**
    * @brief Get the global zDirection of the current ReferenceFrame
    *
    * @return vec3<TFloat> The global z-direction
    */
    template <IsFloat TFloat>
    vec3<TFloat> ReferenceFrame<TFloat>::getGlobalZDir() const
    {
        mat3<TFloat> rotationMatrix = global_rotation_.getMatrix();
        return vec3<TFloat>{ rotationMatrix[2][0], rotationMatrix[2][1], rotationMatrix[2][2] };
    };

    /**
    * @brief Get the Model Matrix
    *
    * @return mat4<TFloat> The model matrix
    */
    template <IsFloat TFloat>
    const mat4<TFloat>& ReferenceFrame<TFloat>::getModelMatrix() const
    {
        return global_transformation_;
    };

    /**
    * @brief Get the Normal Matrix
    *
    * @return mat3<TFloat> The normal matrix
    */
    template <IsFloat TFloat>
    mat3<TFloat> ReferenceFrame<TFloat>::getModelNormalMatrix() const
    {
        return global_rotation_.getMatrix();
    };


    // ======================= //
    // === Transformations === //
    // ======================= //
    template <IsFloat TFloat>
    vec3<TFloat> ReferenceFrame<TFloat>::globalToLocal(const vec3<TFloat>& global_point) const
    {
        mat4<TFloat> inverse_transform = inverse(global_transformation_);
        vec4<TFloat> homogeneous(global_point[0], global_point[1], global_point[2], 1.0f);
        vec4<TFloat> result = inverse_transform * homogeneous;
        return vec3<TFloat>(result[0] / result[3], result[1] / result[3], result[2] / result[3]);
    }

    template <IsFloat TFloat>
    vec3<TFloat> ReferenceFrame<TFloat>::localToGlobal(const vec3<TFloat>& local_point) const
    {
        vec4<TFloat> homogeneous(local_point[0], local_point[1], local_point[2], 1.0f);
        vec4<TFloat> result = global_transformation_ * homogeneous;
        return vec3<TFloat>(result[0] / result[3], result[1] / result[3], result[2] / result[3]);
    }

    template <IsFloat TFloat>
    vec3<TFloat> ReferenceFrame<TFloat>::globalDirectionToLocal(const vec3<TFloat>& global_direction) const
    {
        return this->global_rotation_.getInverseMatrix() * global_direction;
    };

    template <IsFloat TFloat>
    vec3<TFloat> ReferenceFrame<TFloat>::localDirectionToGlobal(const vec3<TFloat>& local_direction) const
    {
        return this->global_rotation_ * local_direction;
    };

    template <IsFloat TFloat>
    vec3<TFloat> ReferenceFrame<TFloat>::localPointToGlobalVelocity(const vec3<TFloat>& local_point) const
    {
        vec3<TFloat> global_point = localToGlobal(local_point);
        vec3<TFloat> relative_position = global_point - global_position_;
        vec3<TFloat> angular_contribution = cross(global_angular_rate_, relative_position);
        return global_velocity_ + angular_contribution;
    };

    template <IsFloat TFloat>
    vec3<TFloat> ReferenceFrame<TFloat>::globalPointToLocalVelocity(const vec3<TFloat>& global_point) const
    {
        vec3<TFloat> relative_position = global_point - global_position_;
        vec3<TFloat> angular_contribution = cross(global_angular_rate_, relative_position);
        return globalDirectionToLocal(global_velocity_ + angular_contribution);
    };

    template <IsFloat TFloat>
    vec3<TFloat> ReferenceFrame<TFloat>::globalToLocalVelocity(const vec3<TFloat>& global_velocity) const
    {
        // Convert global velocity to velocity relative to this reference frame
        vec3<TFloat> relative_global_velocity = global_velocity - this->global_velocity_;

        // Then transform the direction to local coordinates
        return globalDirectionToLocal(relative_global_velocity);
    };
    
    template <IsFloat TFloat>
    vec3<TFloat> ReferenceFrame<TFloat>::localToGlobalVelocity(const vec3<TFloat>& local_velocity) const
    {
        // Transform local velocity direction to global coordinates
        vec3<TFloat> global_direction_velocity = localDirectionToGlobal(local_velocity);

        // Add this reference frame's global velocity
        return global_direction_velocity + this->getGlobalVelocity();
    };


    // =========================== //
    // === SPICE Configuration === //
    // =========================== //
    template <IsFloat TFloat>
    void ReferenceFrame<TFloat>::configureSPICE(std::string naif_name, std::string frame_name)
    {
        naif_name_ = naif_name;
        frame_name_ = frame_name;

        if (!naif_name_.empty()) {
            configured_object_ = true;
            if (!frame_name_.empty()) {
                configured_frame_ = true;
            }
            else {
                configured_frame_ = false;
            }
        }
        else {
            configured_object_ = false;
            configured_frame_ = false;
        }

        onSPICEConfiguration();
    }

    template <IsFloat TFloat>
    void ReferenceFrame<TFloat>::setNAIFID(std::string naif_name)
    {
        this->configureSPICE(naif_name, frame_name_);
    };

    template <IsFloat TFloat>
    void ReferenceFrame<TFloat>::setFrameName(std::string frame_name)
    {
        this->configureSPICE(naif_name_, frame_name);
    };


    template <IsFloat TFloat>
    void ReferenceFrame<TFloat>::updateSPICEStates(double et)
    {
        // This function directly sets the local and global transformation data without relying on the other
        // setter methods to ensure the SPICE frames are handled appropriately.
        if (!parent_->configured_frame_) {
            throw std::runtime_error("SPICE NAIFID and FrameName have not both been configured for a Parent (Group) object");
        }

        if (!scene_->configured_frame_) {
            throw std::runtime_error("SPICE NAIFID and FrameName have not both been configured for the Scene object");
        }
        if (!configured_object_) {
            throw std::runtime_error("SPICE NAIFID has not been configured");
        }

        // Compute local transform (relative to parent):
        std::array<vira::vec3<TFloat>, 2> local_state = SpiceUtils<TFloat>::spkezr(naif_name_, et, parent_->frame_name_, "NONE", parent_->naif_name_);
        local_position_ = local_state[0];
        local_velocity_ = local_state[1];
        if (!frame_name_.empty()) {
            local_rotation_ = Rotation<TFloat>(SpiceUtils<TFloat>::pxform(frame_name_, parent_->frame_name_, et));
            local_angular_rate_ = SpiceUtils<TFloat>::computeAngularRate(frame_name_, parent_->frame_name_, et);
        }
        local_transformation_ = makeTransformationMatrix(local_position_, local_rotation_, local_scale_);

        // Compute global transform (relative to scene):
        std::array<vira::vec3<TFloat>, 2> global_state = SpiceUtils<TFloat>::spkezr(naif_name_, et, scene_->frame_name_, "NONE", scene_->naif_name_);
        global_position_ = global_state[0];
        global_velocity_ = global_state[1];
        if (!frame_name_.empty()) {
            global_rotation_ = Rotation<TFloat>(SpiceUtils<TFloat>::pxform(frame_name_, scene_->frame_name_, et));
            global_angular_rate_ = SpiceUtils<TFloat>::computeAngularRate(frame_name_, scene_->frame_name_, et);
        }
        global_transformation_ = makeTransformationMatrix(global_position_, global_rotation_, global_scale_);

        onReferenceFrameChanged();
    };



    // ============================= //
    // === Static Helper Methods === //
    // ============================= //
    template <IsFloat TFloat>
    mat4<TFloat> ReferenceFrame<TFloat>::makeTransformationMatrix(vec3<TFloat> position, Rotation<TFloat> rotation, vec3<TFloat> scale)
    {
        mat4<TFloat> result;
        mat3<TFloat> rotMatrix = rotation.getMatrix();

        // Recall column-major ordering:
        result[0][0] = rotMatrix[0][0] * scale[0]; result[1][0] = rotMatrix[1][0] * scale[1]; result[2][0] = rotMatrix[2][0] * scale[2]; result[3][0] = position[0];
        result[0][1] = rotMatrix[0][1] * scale[0]; result[1][1] = rotMatrix[1][1] * scale[1]; result[2][1] = rotMatrix[2][1] * scale[2]; result[3][1] = position[1];
        result[0][2] = rotMatrix[0][2] * scale[0]; result[1][2] = rotMatrix[1][2] * scale[1]; result[2][2] = rotMatrix[2][2] * scale[2]; result[3][2] = position[2];
        result[0][3] = TFloat(0);                  result[1][3] = TFloat(0);                  result[2][3] = TFloat(0);                  result[3][3] = TFloat(1);

        return result;
    };

    template <IsFloat TFloat>
    vec3<TFloat> ReferenceFrame<TFloat>::getPositionFromTransformation(mat4<TFloat> transformation)
    {
        // Recall column-major ordering:
        return vec3<TFloat>(transformation[3][0], transformation[3][1], transformation[3][2]);
    };

    template <IsFloat TFloat>
    vec3<TFloat> ReferenceFrame<TFloat>::getScaleFromTransformation(mat4<TFloat> transformation)
    {
        // Extract scale from the length of the first three columns:
        vec3<TFloat> scale_x(
            transformation[0][0],
            transformation[0][1],
            transformation[0][2]);

        vec3<TFloat> scale_y(
            transformation[1][0],
            transformation[1][1],
            transformation[1][2]);

        vec3<TFloat> scale_z(
            transformation[2][0],
            transformation[2][1],
            transformation[2][2]);

        TFloat sx = length(scale_x);
        TFloat sy = length(scale_y);
        TFloat sz = length(scale_z);

        return vec3<TFloat>(sx, sy, sz);
    };

    template <IsFloat TFloat>
    Rotation<TFloat> ReferenceFrame<TFloat>::getRotationFromTransformation(mat4<TFloat> transformation)
    {
        vec3<TFloat> scale = getScaleFromTransformation(transformation);
        return getRotationFromTransformation(transformation, scale);
    };

    template <IsFloat TFloat>
    Rotation<TFloat> ReferenceFrame<TFloat>::getRotationFromTransformation(mat4<TFloat> transformation, vec3<TFloat> scale)
    {
        const TFloat epsilon = TFloat(1e-6);

        // Count negative scale components
        int negativeCount = 0;
        vec3<TFloat> absScale = scale;
        if (scale[0] < 0) { negativeCount++; absScale[0] = -scale[0]; }
        if (scale[1] < 0) { negativeCount++; absScale[1] = -scale[1]; }
        if (scale[2] < 0) { negativeCount++; absScale[2] = -scale[2]; }

        // Extract rotation matrix by normalizing the scale out
        mat3<TFloat> rotationMatrix;
        if (absScale[0] > epsilon) {
            rotationMatrix[0][0] = transformation[0][0] / absScale[0];
            rotationMatrix[0][1] = transformation[0][1] / absScale[0];
            rotationMatrix[0][2] = transformation[0][2] / absScale[0];
        }
        else {
            rotationMatrix[0][0] = TFloat(1);
            rotationMatrix[0][1] = TFloat(0);
            rotationMatrix[0][2] = TFloat(0);
        }

        if (absScale[1] > epsilon) {
            rotationMatrix[1][0] = transformation[1][0] / absScale[1];
            rotationMatrix[1][1] = transformation[1][1] / absScale[1];
            rotationMatrix[1][2] = transformation[1][2] / absScale[1];
        }
        else {
            rotationMatrix[1][0] = TFloat(0);
            rotationMatrix[1][1] = TFloat(1);
            rotationMatrix[1][2] = TFloat(0);
        }

        if (absScale[2] > epsilon) {
            rotationMatrix[2][0] = transformation[2][0] / absScale[2];
            rotationMatrix[2][1] = transformation[2][1] / absScale[2];
            rotationMatrix[2][2] = transformation[2][2] / absScale[2];
        }
        else {
            rotationMatrix[2][0] = TFloat(0);
            rotationMatrix[2][1] = TFloat(0);
            rotationMatrix[2][2] = TFloat(1);
        }

        // If odd number of negative scales, we need to flip the matrix:
        if (negativeCount % 2 == 1) {
            // Flip one column (conventionally the first)
            rotationMatrix[0][0] *= -1;
            rotationMatrix[0][1] *= -1;
            rotationMatrix[0][2] *= -1;
        }

        // Create rotation from matrix
        return Rotation<TFloat>(rotationMatrix); // This validate determinant
    };


    // ========================= //
    // === Private Functions === //
    // ========================= //
    template <IsFloat TFloat>
    void ReferenceFrame<TFloat>::extractLocalComponents()
    {
        local_position_ = getPositionFromTransformation(local_transformation_);
        local_scale_ = getScaleFromTransformation(local_transformation_);
        local_rotation_ = getRotationFromTransformation(local_transformation_, local_scale_);
    };

    template <IsFloat TFloat>
    void ReferenceFrame<TFloat>::updateTransformations()
    {
        local_transformation_ = makeTransformationMatrix(local_position_, local_rotation_, local_scale_);

        updateGlobalTransformation();
    };

    template <IsFloat TFloat>
    void ReferenceFrame<TFloat>::updateGlobalTransformation()
    {
        if (parent_ == nullptr) {
            global_transformation_ = local_transformation_;
        }
        else {
            global_transformation_ = parent_->global_transformation_ * local_transformation_;
        }
        

        global_position_ = getPositionFromTransformation(global_transformation_);
        global_scale_ = getScaleFromTransformation(global_transformation_);
        global_rotation_ = getRotationFromTransformation(global_transformation_, global_scale_);

        updateGlobalVelocities();
    };

    template <IsFloat TFloat>
    void ReferenceFrame<TFloat>::updateGlobalVelocities()
    {
        vec3<TFloat> parent_pos{ 0,0,0 };
        vec3<TFloat> parent_vel{ 0,0,0 };
        vec3<TFloat> parent_ang_rate{ 0,0,0 };
        if (parent_ != nullptr) {
            parent_pos = getPositionFromTransformation(parent_->global_transformation_);
            parent_vel = parent_->global_velocity_;
            parent_ang_rate = parent_->global_angular_rate_;
        }

        // Transform local velocities to global space
        vec3<TFloat> local_vel_global = global_rotation_ * local_velocity_;
        vec3<TFloat> local_ang_global = global_rotation_ * local_angular_rate_;

        // Get position relative to parent
        vec3<TFloat> relative_position = global_position_ - parent_pos;

        // Velocity due to parent's rotation around parent's center
        vec3<TFloat> velocity_from_parent_rotation = cross(parent_ang_rate, relative_position);

        // Combine all velocity components
        global_velocity_ = parent_vel + velocity_from_parent_rotation + local_vel_global;

        // Angular rate is additive when transformed to global space
        global_angular_rate_ = parent_ang_rate + local_ang_global;
    }

    template <IsFloat TFloat>
    void ReferenceFrame<TFloat>::setParent(ReferenceFrame<TFloat>* parent)
    {
        // The primary assumption here is that the Scene graph is a proper tree, not a DAG,
        // and that child nodes maintain their parents forever.  Because of this, this function
        // needs only be called at object creation, and the set parent pointer remains valid
        // for the entire lifetime of the object.
        if (parent != nullptr) {
            // Update the parent pointer:
            parent_ = parent;
            scene_ = parent_->scene_;
        }
        else {
            // This shouldn't happen but we need to catch it just in case:
            throw std::runtime_error("Parent ReferenceFrame was NULL");
        }

        // Update the current global states:
        updateGlobalTransformation();
    };
};