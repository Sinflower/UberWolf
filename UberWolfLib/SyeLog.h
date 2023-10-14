#pragma once

#include <functional>

#include "Types.h"

namespace SyeLog
{
	using KeyCallback = std::function<void(Key& key, const bool& useOldDxArc)>;

	bool init();
	void registerKeyCallback(KeyCallback callback);
}

