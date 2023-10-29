#pragma once

#include "Types.h"

class WolfKeyFinder
{
public:
	WolfKeyFinder(const tString& m_exePath);

	bool Inject();

	const Key& GetKey() const { return m_key; }
	const bool& UseOldDxArc() const { return m_useOldDxArc; }

	void KeyCallback(Key& key, const bool& useOldDxArc)
	{
		m_key = key;
		m_useOldDxArc = useOldDxArc;
	}

	static const tString DLL_NAME;

private:
private:
	tString m_exePath;
	Key m_key;
	bool m_useOldDxArc = false;
};
