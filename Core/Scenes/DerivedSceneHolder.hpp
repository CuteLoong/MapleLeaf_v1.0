#pragma once

#include "DerivedScene.hpp"
#include "Log.hpp"
#include "NonCopyable.hpp"
#include <memory>

namespace MapleLeaf {
class DerivedSceneHolder : NonCopyable
{
public:
    template<typename T, typename = std::enable_if_t<std::is_convertible_v<T*, DerivedScene*>>>
    bool Has() const
    {
        const auto it = derivedScenes.find(TypeInfo<DerivedScene>::GetTypeId<T>());

        return it != derivedScenes.end() && it->second;
    }

    template<typename T, typename = std::enable_if_t<std::is_convertible_v<T*, DerivedScene*>>>
    T* Get() const
    {
        auto it = derivedScenes.find(TypeInfo<DerivedScene>::GetTypeId<T>());

        if (it == derivedScenes.end() || !it->second) {
            return nullptr;
        }

        return static_cast<T*>(it->second.get());
    }

    template<typename T, typename = std::enable_if_t<std::is_convertible_v<T*, DerivedScene*>>>
    void Add(std::unique_ptr<T>&& derivedScene)
    {
        Remove<T>();

        const auto typeId = TypeInfo<DerivedScene>::GetTypeId<T>();

        // Then, add the Scene
        derivedScenes[typeId] = std::move(derivedScene);
    }

    template<typename T, typename = std::enable_if_t<std::is_convertible_v<T*, DerivedScene*>>>
    void Remove()
    {
        const auto typeId = TypeInfo<DerivedScene>::GetTypeId<T>();

        // Then, remove the Scene.
        derivedScenes.erase(typeId);
    }


    void Clear() { derivedScenes.clear(); }


    template<typename Func>
    void ForEach(Func&& func)
    {
        for (auto& [typeId, derivedScene] : derivedScenes) {
            try {
                func(typeId, derivedScene.get());
            }
            catch (const std::exception& e) {
                Log::Error(e.what(), '\n');
            }
        }
    }

private:
    std::unordered_map<TypeId, std::unique_ptr<DerivedScene>> derivedScenes;
};
}   // namespace MapleLeaf