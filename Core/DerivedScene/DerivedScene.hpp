#pragma once

#include "NonCopyable.hpp"
#include "TypeInfo.hpp"

namespace MapleLeaf {
class DerivedScene : NonCopyable
{
public:
    virtual ~DerivedScene() = default;

    virtual void Start()  = 0;
    virtual void Update() = 0;

    bool IsEnabled() const { return enabled; }
    bool IsStarted() const { return started; }

    void SetEnabled(bool enabled) { this->enabled = enabled; }
    void SetStarted(bool started) { this->started = started; }

private:
    bool started = false;
    bool enabled = true;
};

template class TypeInfo<DerivedScene>;
}   // namespace MapleLeaf