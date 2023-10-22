#pragma once

#include <functional>
#include <memory>
#include <thread>

class SlotWrapper
{
public:
	using noParam = std::function<void()>;
	using oneParam = std::function<void(void*)>;
	using twoParam = std::function<void(void*, void*)>;

public:
	SlotWrapper(const noParam& nP) : m_noParam(nP) { }
	SlotWrapper(const oneParam& oP) : m_oneParam(oP) { }
	SlotWrapper(const twoParam& tP) : m_twoParam(tP) { }

	std::thread operator()() const
	{
		if (m_noParam)
			return std::thread(m_noParam);

		return std::thread();
	}

	std::thread operator()(void* p) const
	{
		if (m_oneParam)
			return std::thread(m_oneParam, p);

		return std::thread();
	}

	std::thread operator()(void* p1, void* p2) const
	{
		if (m_twoParam) 
			return std::thread(m_twoParam, p1, p2);

		return std::thread();
	}

private:

private:
	noParam m_noParam = nullptr;
	oneParam m_oneParam = nullptr;
	twoParam m_twoParam = nullptr;
};

using SlotPtr = std::shared_ptr<SlotWrapper>;
