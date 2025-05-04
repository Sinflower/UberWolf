/*
 *  File: RouteCommand.h
 *  Copyright (c) 2024 Sinflower
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

#include "FileCoder.h"
#include "WolfRPGUtils.h"

class RouteCommand
{
public:
	RouteCommand() = default;

	bool Init(FileCoder& coder)
	{
		m_id              = coder.ReadByte();
		uint32_t argCount = coder.ReadByte();

		for (uint32_t i = 0; i < argCount; i++)
			m_args.push_back(coder.ReadInt());

		VERIFY_MAGIC(coder, TERMINATOR);

		return true;
	}

	void Dump(FileCoder& coder) const
	{
		coder.WriteByte(m_id);
		coder.WriteByte((uint8_t)m_args.size());
		for (uint32_t arg : m_args)
			coder.WriteInt(arg);
		coder.Write(TERMINATOR);
	}

private:
	uint8_t m_id = 0;
	uInts m_args = {};

	inline static const Bytes TERMINATOR{ 0x01, 0x00 };
};

using RouteCommands = std::vector<RouteCommand>;
