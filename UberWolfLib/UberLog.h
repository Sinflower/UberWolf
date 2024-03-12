#pragma once

#include <mutex>
#include <format>

#include "Types.h"

namespace uberLog
{
extern LogCallbacks s_logCallbacks;
}

class UberLog
{
public:
	using SType = tOstream&(tOstream&);

public:
	UberLog(tOstream& oStream) :
		m_oStream(oStream)
	{}

	template<typename T>
	void log(T& msg)
	{
		std::lock_guard<std::mutex> lock(s_mtx);
		m_oStream << msg.str();

		for (auto& callback : uberLog::s_logCallbacks)
			callback(msg.str(), false);

		msg.flush();
	}

private:
	tOstream& m_oStream;
	static std::mutex s_mtx;
};

class UberLogBuffer
{
public:
	UberLogBuffer(const UberLogBuffer&)            = delete;
	UberLogBuffer& operator=(const UberLogBuffer&) = delete;
	UberLogBuffer& operator=(UberLogBuffer&&)      = delete;

	UberLogBuffer(UberLog* pLogger) :
		m_stream(),
		m_pLog(pLogger)
	{}

	UberLogBuffer(UberLogBuffer&& buf) :
		m_stream(std::move(buf.m_stream)),
		m_pLog(buf.m_pLog)
	{}

	template<typename T>
	UberLogBuffer& operator<<(T&& msg)
	{
		m_stream << std::forward<T>(msg);
		return *this;
	}

	UberLogBuffer& operator<<(UberLog::SType sType)
	{
		m_stream << sType;
		return *this;
	}

	~UberLogBuffer()
	{
		if (m_pLog)
			m_pLog->log(m_stream);
	}

private:
	tStringStream m_stream;
	UberLog* m_pLog;
};

template<typename T>
UberLogBuffer operator<<(UberLog& log, T&& msg)
{
	UberLogBuffer buf(&log);
	buf << std::forward<T>(msg);
	return buf;
}

namespace uberLog
{
extern UberLog s_info;
extern UberLog s_error;

static inline std::size_t AddLogCallback(const LogCallback& callback)
{
	const std::size_t idx = s_logCallbacks.size();
	s_logCallbacks.push_back(callback);
	return idx;
}

static inline void RemoveLogCallback(const std::size_t& id)
{
	if (id >= 0 && id < s_logCallbacks.size())
		s_logCallbacks.erase(s_logCallbacks.begin() + id);
}
} // namespace uberLog

template<typename... Args>
static inline std::wstring vFormat(std::wstring_view rt_fmt_str, Args&&... args)
{
	return std::vformat(rt_fmt_str, std::make_wformat_args(args...));
}

#define INFO_LOG  uberLog::s_info
#define ERROR_LOG uberLog::s_error
