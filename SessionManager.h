#ifndef __session_manager_h_
#define __session_manager_h_

#include <iostream>
#include <map>
#include "Ema.h"

#ifdef WIN32
#include <windows.h>
#else
#include <sys/time.h>
#endif

struct ConsumerInstanceInfo {
	refinitiv::ema::access::OmmConsumer* ommConsumer;
	refinitiv::ema::access::UInt64 loginHandle;
	refinitiv::ema::access::UInt64 dirHandle;
	bool isServiceUp;
	bool isInitialized;

};

// application defined client class for receiving and processing of item messages
class SessionManager : public refinitiv::ema::access::OmmConsumerClient
{
public:

	SessionManager();
	void create(refinitiv::ema::access::OmmConsumer&, refinitiv::ema::access::OmmConsumer&, refinitiv::ema::access::EmaString);

	void decodeLoginState(const refinitiv::ema::access::OmmState&, ConsumerInstanceInfo*);
	void decodeDirectory(const refinitiv::ema::access::Msg&, ConsumerInstanceInfo*);

	void setMainOmmConsumer(refinitiv::ema::access::OmmConsumer&);
	void setBackupOmmConsumer(refinitiv::ema::access::OmmConsumer&);
	
	refinitiv::ema::access::UInt64 registerClient(refinitiv::ema::access::ReqMsg&, refinitiv::ema::access::OmmConsumerClient&, void* closure = 0);

	bool isActive(ConsumerInstanceInfo* consumerSession) { return consumerSession == activeConsumer; };

protected:

	void onRefreshMsg(const refinitiv::ema::access::RefreshMsg&, const refinitiv::ema::access::OmmConsumerEvent&);

	void onUpdateMsg(const refinitiv::ema::access::UpdateMsg&, const refinitiv::ema::access::OmmConsumerEvent&);

	void onStatusMsg(const refinitiv::ema::access::StatusMsg&, const refinitiv::ema::access::OmmConsumerEvent&);

	void defineActiveConsumer();
	void handleEvent();

	ConsumerInstanceInfo primaryConsumer;
	ConsumerInstanceInfo backupConsumer;

	ConsumerInstanceInfo* activeConsumer;
	ConsumerInstanceInfo* inactiveConsumer;

	refinitiv::ema::access::EmaString _serviceName;

	typedef std::map<refinitiv::ema::access::UInt64, refinitiv::ema::access::UInt64>	ItemHandleList;
	ItemHandleList itemHandleList;
};

#endif // __session_manager_h_
