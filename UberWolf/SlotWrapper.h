/*
 *  File: SlotWrapper.h
 *  Copyright (c) 2023 Sinflower
 *
 *  MIT License
 *
 *  Permission is hereby granted, free of charge, to any person obtaining a copy
 *  of this software and associated documentation files (the "Software"), to deal
 *  in the Software without restriction, including without limitation the rights
 *  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 *  copies of the Software, and to permit persons to whom the Software is
 *  furnished to do so, subject to the following conditions:
 *
 *  The above copyright notice and this permission notice shall be included in all
 *  copies or substantial portions of the Software.
 *
 *  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 *  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 *  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 *  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 *  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 *  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 *  SOFTWARE.
 *
 */

#pragma once

#include <functional>
#include <memory>
#include <thread>

class SlotWrapper
{
public:
	using noParam  = std::function<void()>;
	using oneParam = std::function<void(void*)>;
	using twoParam = std::function<void(void*, void*)>;

public:
	SlotWrapper(const noParam& nP) :
		m_noParam(nP) {}
	SlotWrapper(const oneParam& oP) :
		m_oneParam(oP) {}
	SlotWrapper(const twoParam& tP) :
		m_twoParam(tP) {}

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

	bool HasNoParam() const
	{
		return m_noParam != nullptr;
	}
	bool HasOneParam() const
	{
		return m_oneParam != nullptr;
	}
	bool HasTwoParam() const
	{
		return m_twoParam != nullptr;
	}

private:
private:
	noParam m_noParam   = nullptr;
	oneParam m_oneParam = nullptr;
	twoParam m_twoParam = nullptr;
};

using SlotPtr = std::shared_ptr<SlotWrapper>;
