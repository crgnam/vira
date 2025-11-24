#ifndef VIRA_REFERENCE_FRAME_HPP
#define VIRA_REFERENCE_FRAME_HPP

#include <ostream>

#include "vira/vec.hpp"
#include "vira/constraints.hpp"
#include "vira/rotation.hpp"
#include "vira/units/units.hpp"
#include "vira/spectral_data.hpp"

namespace vira {
    template <IsSpectral TSpectral, IsFloat TFloat, IsFloat TMeshFloat> requires LesserFloat<TFloat, TMeshFloat>
    class Scene;

    namespace scene {
        template <IsSpectral TSpectral, IsFloat TFloat, IsFloat TMeshFloat> requires LesserFloat<TFloat, TMeshFloat>
        class Group;
    }

    /**
     * @brief Class representing ReferenceFrame information
     *
     * @tparam TFloat Precision Type (float or double)
     */
    template <IsFloat TFloat>
    class ReferenceFrame {
    public:
        ReferenceFrame() = default;

        // Copy constructors
        ReferenceFrame(const ReferenceFrame& other) = default;
        ReferenceFrame& operator=(const ReferenceFrame& other) = default;

        // Move constructors
        ReferenceFrame(ReferenceFrame&& other) noexcept = default;
        ReferenceFrame& operator=(ReferenceFrame&& other) noexcept = default;

        virtual ~ReferenceFrame() = default;

        // Local position modifiers and getters:
        void setLocalPosition(vec3<TFloat> position);
        void setLocalPosition(double x, double y, double z);

        void localTranslateBy(const vec3<TFloat>& translation);
        void localTranslateBy(double x, double y, double z);

        vec3<TFloat> getLocalPosition() const { return local_position_; }
        vec3<TFloat> getGlobalPosition() const { return global_position_; }


        // Local rotation modifiers and getters:
        void setLocalRotation(Rotation<TFloat> rotation);
        void setLocalRotation(mat3<TFloat> rotation);
        void setLocalEulerAngles(units::Degree r1, units::Degree r2, units::Degree r3, std::string sequence = "XYZ");
        void setLocalQuaternion(vec4<TFloat> quaternion);
        void setLocalShusterQuaternion(vec4<TFloat> quaternion);
        void setLocalAxisAngle(vec3<TFloat> axis, units::Degree angle);

        void localRotateBy(const Rotation<TFloat>& rotate);

        Rotation<TFloat> getLocalRotation() const { return local_rotation_; }
        Rotation<TFloat> getGlobalRotation() const { return global_rotation_; }


        // Local scale modifiers and getters:
        void setLocalScale(vec3<TFloat> scale);
        void setLocalScale(double sx, double sy, double sz);
        void setLocalScale(double scale);

        void localScaleBy(vec3<TFloat> scaling);
        void localScaleBy(double sx, double sy, double sz);
        void localScaleBy(double scaling);

        vec3<TFloat> getLocalScale() const { return local_scale_; }
        vec3<TFloat> getGlobalScale() const { return global_scale_; }


        // Local velocity modifiers and getters:
        void setLocalVelocity(vec3<TFloat> velocity);
        void setLocalVelocity(double vx, double vy, double vz);

        void addLocalVelocity(vec3<TFloat> velocity_addition);
        void addLocalVelocity(double vx, double vy, double vz);

        const vec3<TFloat>& getLocalVelocity() const { return local_velocity_; }
        const vec3<TFloat>& getGlobalVelocity() const { return global_velocity_; }

        // Local angular rate modifiers and getters:
        void setLocalAngularRate(vec3<TFloat> angular_rate);
        void setLocalAngularRate(double wx, double wy, double wz);

        void addLocalAngularRate(vec3<TFloat> rate_addition);
        void addLocalAngularRate(double wx, double wy, double wz);

        const vec3<TFloat>& getLocalAngularRate() const { return local_angular_rate_; }
        const vec3<TFloat>& getGlobalAngularRate() const { return global_angular_rate_; }


        // Combination Setters:
        void setLocalPositionRotation(vec3<TFloat> position, Rotation<TFloat> rotation);
        void setLocalPositionRotationScale(vec3<TFloat> position, Rotation<TFloat> rotation, vec3<TFloat> scale);
        void setLocalTransformation(mat4<TFloat> local_transformation);


        // Transformation matrices:
        mat4<TFloat> getParentTransformationMatrix() const { return parent_->global_transformation_; }
        mat4<TFloat> getLocalTransformationMatrix() const { return local_transformation_; }
        mat4<TFloat> getGlobalTransformationMatrix() const { return global_transformation_; }

        const mat4<TFloat>& getModelMatrix() const;
        mat3<TFloat> getModelNormalMatrix() const;

        vec3<TFloat> getLocalZDir() const;
        vec3<TFloat> getGlobalZDir() const;


        // Transformations:
        vec3<TFloat> globalToLocal(const vec3<TFloat>& global_point) const;
        vec3<TFloat> localToGlobal(const vec3<TFloat>& local_point) const;

        vec3<TFloat> globalDirectionToLocal(const vec3<TFloat>& global_direction) const;
        vec3<TFloat> localDirectionToGlobal(const vec3<TFloat>& local_direction) const;

        vec3<TFloat> localPointToGlobalVelocity(const vec3<TFloat>& local_point) const;
        vec3<TFloat> globalPointToLocalVelocity(const vec3<TFloat>& global_point) const;

        vec3<TFloat> globalToLocalVelocity(const vec3<TFloat>& global_velocity) const;
        vec3<TFloat> localToGlobalVelocity(const vec3<TFloat>& local_velocity) const;

        // SPICE Configuration:
        void configureSPICE(std::string naif_name, std::string frame_name);
        void setNAIFID(std::string naif_name);
        void setFrameName(std::string frame_name);

        bool isConfiguredSpiceObject() const { return configured_object_; }
        bool isConfiguredSpiceFrame() const { return configured_frame_; }

        const std::string& getNAIFName() const { return naif_name_; }
        const std::string& getFrameName() const { return frame_name_; }

        // Static helper methods:
        static mat4<TFloat> makeTransformationMatrix(vec3<TFloat> position, Rotation<TFloat> rotation, vec3<TFloat> scale);
        static vec3<TFloat> getPositionFromTransformation(mat4<TFloat> transformation);
        static Rotation<TFloat> getRotationFromTransformation(mat4<TFloat> transformation);
        static Rotation<TFloat> getRotationFromTransformation(mat4<TFloat> transformation, vec3<TFloat> scale);
        static vec3<TFloat> getScaleFromTransformation(mat4<TFloat> transformation);

    protected:
        template <IsSpectral TSpectral, IsFloat TMeshFloat>
        Scene<TSpectral, TFloat, TMeshFloat>* getScenePtr() const
        {
            return static_cast<Scene<TSpectral, TFloat, TMeshFloat>*>(scene_);
        }

    private:
        void updateTransformations();
        virtual void onReferenceFrameChanged() {}

        void onSPICEConfiguration()
        {
            if (parent_ != nullptr) {
                if (!parent_->isConfiguredSpiceFrame()) {
                    throw std::runtime_error("Attempted to confiugre SPICE Frame/Object when the Parent is not a configured SPICE frame");
                }
            }

            if (!scene_->isConfiguredSpiceFrame()) {
                throw std::runtime_error("Attempted to confiugre SPICE Frame/Object when the root Scene is not a configured SPICE frame");
            }
        }
        void updateSPICEStates(double et);

        // Local states:
        mat4<TFloat> local_transformation_{ 1 };
        void extractLocalComponents();

        vec3<TFloat> local_position_{ 0,0,0 };
        Rotation<TFloat> local_rotation_{ 1 };
        vec3<TFloat> local_scale_{ 1,1,1 };

        vec3<TFloat> local_velocity_{ 0,0,0 };
        vec3<TFloat> local_angular_rate_{ 0,0,0 };


        // Parent states:
        ReferenceFrame<TFloat>* parent_ = nullptr;
        ReferenceFrame<TFloat>* scene_ = nullptr;

        // Global states:
        mat4<TFloat> global_transformation_{ 1 };
        void updateGlobalTransformation();
        void updateGlobalVelocities();

        vec3<TFloat> global_position_{ 0,0,0 };
        Rotation<TFloat> global_rotation_{ 1 };
        vec3<TFloat> global_scale_{ 1,1,1 };

        vec3<TFloat> global_velocity_{ 0,0,0 };
        vec3<TFloat> global_angular_rate_{ 0,0,0 };

        void setParent(ReferenceFrame<TFloat>* parent);

        template <IsSpectral TSpectral, IsFloat TFloat2, IsFloat TMeshFloat> requires LesserFloat<TFloat2, TMeshFloat>
        friend class scene::Group;

        template <IsSpectral TSpectral, IsFloat TFloat2, IsFloat TMeshFloat> requires LesserFloat<TFloat2, TMeshFloat>
        friend class Scene;

        // SPICE Information:
        std::string naif_name_ = "";
        std::string frame_name_ = "";

        bool configured_object_ = false;
        bool configured_frame_ = false;
    };
};

#include "implementation/reference_frame.ipp"

#endif