#ifndef VIRA_SCENE_IDS_HPP
#define VIRA_SCENE_IDS_HPP

#include <functional>
#include <iostream>
#include <cstddef>
#include <limits>
#include <string_view>

namespace vira {
    // Specific Types (with allowable range)
    class MeshTag {
    public:
        static constexpr const char* name() { return "MeshID"; }
    };

    class UnresolvedTag {
    public:
        static constexpr const char* name() { return "UnresolvedID"; }
    };

    class LightTag {
    public:
        static constexpr const char* name() { return "LightID"; }
    };

    class GroupTag {
    public:
        static constexpr const char* name() { return "GroupID"; }
    };

    class InstanceTag {
    public:
        static constexpr const char* name() { return "InstanceID"; }
    };

    class MaterialTag {
    public:
        static constexpr const char* name() { return "MaterialID"; }
    };

    class CameraTag {
    public:
        static constexpr const char* name() { return "CameraID"; }
    };

    // Forward Declare:
    template <IsSpectral TSpectral, IsFloat TFloat, IsFloat TMeshFloat> requires LesserFloat<TFloat, TMeshFloat>
    class Scene;

    // ================ //
    // === Type IDs === //
    // ================ //
    template<typename T, typename UnderlyingType, UnderlyingType MaxCount>
    class TypedID {
    public:
        using ValueType = UnderlyingType;

        TypedID() : id_(0) {}

        UnderlyingType id() const { return id_; }
        explicit operator UnderlyingType() const { return id_; }

        bool operator==(const TypedID& other) const { return id_ == other.id_; }
        bool operator<(const TypedID& other) const { return id_ < other.id_; }

        friend std::ostream& operator<<(std::ostream& os, const TypedID& id) { return os << id.format_id(); }
        std::string name() const { return format_id(); }

        static constexpr const char* type_name() { return T::name(); }
        static constexpr UnderlyingType max_size() { return std::numeric_limits<UnderlyingType>::max(); }
        static constexpr UnderlyingType max_count() { return MaxCount; }
        static constexpr bool is_valid_count(UnderlyingType count) { return count <= MaxCount; }

    private:
        explicit TypedID(UnderlyingType id) : id_(id) {}

        UnderlyingType id_;

        std::string format_id() const {
            std::string retVal = std::string(type_name()) + ":" + std::to_string(id_);
            if (id_ == 0) {
                retVal += "(INVALID)";
            }

            if constexpr (std::same_as<T, GroupTag>) {
                if (id_ == 1) {
                    retVal += "(ROOT SCENE)";
                }
            }
            else if constexpr (std::same_as<T, MaterialTag>) {
                if (id_ == 1) {
                    retVal += " (DEFAULT MATERIAL)";
                }
            }
            
            return retVal;
        }

        template<typename> friend struct IDManager;

        template <IsSpectral TSpectral, IsFloat TFloat, IsFloat TMeshFloat> requires LesserFloat<TFloat, TMeshFloat>
        friend class Scene;
    };


    template<typename T>
    struct is_typed_id : std::false_type {};

    template<typename Tag, typename UnderlyingType, UnderlyingType MaxCount>
    struct is_typed_id<TypedID<Tag, UnderlyingType, MaxCount>> : std::true_type {};

    template<typename T>
    concept IsTypedID = is_typed_id<T>::value;


    // ==================== //
    // === ID Allocator === //
    // ==================== //
    template<typename IDType>
    struct IDManager {
        std::atomic<typename IDType::ValueType> next_id_{ 1 };
        std::atomic<typename IDType::ValueType> active_count_{ 0 };

        // Explicitly delete copy/move operations since std::atomic is not copyable
        IDManager() = default;
        IDManager(const IDManager&) = delete;
        IDManager(IDManager&&) = delete;
        IDManager& operator=(const IDManager&) = delete;
        IDManager& operator=(IDManager&&) = delete;
        ~IDManager() = default;

        IDType allocate() {
            if (active_count_.load() >= IDType::max_count()) {
                throw std::runtime_error("Maximum " + std::string(IDType::type_name()) +
                    " count (" + std::to_string(IDType::max_count()) + ") exceeded");
            }
            active_count_++;
            return IDType(next_id_++);
        }

        void deallocate() {
            active_count_--;
        }
    };

    using MeshID = TypedID<MeshTag, uint32_t, 100000>;
    using UnresolvedID = TypedID<UnresolvedTag, uint32_t, 10000000>;
    using LightID = TypedID<LightTag, uint8_t, 100>;
    using GroupID = TypedID<GroupTag, uint16_t, 1000>;
    using InstanceID = TypedID<InstanceTag, uint32_t, 1000000>;
    using MaterialID = TypedID<MaterialTag, uint8_t, 100>;
    using CameraID = TypedID<CameraTag, uint8_t, 100>;

    template <typename T>
    struct DataEntry {
        T data;
        std::string name;

        DataEntry(T&& newData, std::string newName)
            : data(std::move(newData)), name(newName) {
        }

        // Delete copying
        DataEntry(const DataEntry&) = delete;
        DataEntry& operator=(const DataEntry&) = delete;

        // Allow moving
        DataEntry(DataEntry&&) = default;
        DataEntry& operator=(DataEntry&&) = default;
    };

    template <typename T>
    using ManagedData = DataEntry<std::unique_ptr<T>>;
}

namespace std {
    template<typename Tag, typename UnderlyingType, UnderlyingType MaxCount>
    struct hash<vira::TypedID<Tag, UnderlyingType, MaxCount>> {
        size_t operator()(const vira::TypedID<Tag, UnderlyingType, MaxCount>& id) const {
            return hash<UnderlyingType>()(id.id());
        }
    };
}

#endif