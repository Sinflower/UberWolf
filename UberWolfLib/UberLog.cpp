#include "UberLog.h"

namespace uberLog
{
	LogCallbacks s_logCallbacks = LogCallbacks();
	UberLog s_info = UberLog(tcout);
	UberLog s_error = UberLog(tcerr);
}

/*
template<typename T>
void UberLog::log(T& msg)
{
	std::lock_guard<std::mutex> lock(s_mtx);
	m_oStream << msg.str();

	for (auto& callback : uberLog::s_logCallbacks)
		callback(msg.str(), false);


	msg.flush();
}
*/

std::mutex UberLog::s_mtx = std::mutex();