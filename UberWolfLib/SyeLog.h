#pragma once

#include <functional>

#include "Types.h"

namespace SyeLog
{
using KeyCallback = std::function<void(Key& key, const bool& useOldDxArc)>;

void init();
void registerKeyCallback(KeyCallback callback);
void clearKeyCallbacks();
} // namespace SyeLog
