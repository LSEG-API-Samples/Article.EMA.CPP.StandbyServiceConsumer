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

struct ConsumerSessionInfo {
	thomsonreuters::ema::access::OmmConsumer* ommConsumer;
	thomsonreuters::ema::access::UInt64 loginHandle;
	thomsonreuters::ema::access::UInt64 dirHandle;
	bool isServiceUp;
	bool isInitialized;

};

// application defined client class for receiving and processing of item messages
class SessionManager : public thomsonreuters::ema::access::OmmConsumerClient
{
public:

	SessionManager();
	void create(thomsonreuters::ema::access::OmmConsumer&, thomsonreuters::ema::access::OmmConsumer&,thomsonreuters::ema::access::EmaString);

	void decodeLoginState(const thomsonreuters::ema::access::OmmState&, ConsumerSessionInfo*);
	void decodeDirectory(const thomsonreuters::ema::access::Msg&, ConsumerSessionInfo*);

	void setMainOmmConsumer(thomsonreuters::ema::access::OmmConsumer&);
	void setBackupOmmConsumer(thomsonreuters::ema::access::OmmConsumer&);
	
	thomsonreuters::ema::access::UInt64 registerClient(thomsonreuters::ema::access::ReqMsg&, thomsonreuters::ema::access::OmmConsumerClient&, void* closure = 0);

	bool isActive(ConsumerSessionInfo* consumerSession) { return consumerSession == activeConsumer; };

protected:

	void onRefreshMsg(const thomsonreuters::ema::access::RefreshMsg&, const thomsonreuters::ema::access::OmmConsumerEvent&);

	void onUpdateMsg(const thomsonreuters::ema::access::UpdateMsg&, const thomsonreuters::ema::access::OmmConsumerEvent&);

	void onStatusMsg(const thomsonreuters::ema::access::StatusMsg&, const thomsonreuters::ema::access::OmmConsumerEvent&);

	void defineActiveConsumer();
	void handleEvent();

	ConsumerSessionInfo primaryConsumer;
	ConsumerSessionInfo backupConsumer;

	ConsumerSessionInfo* activeConsumer;
	ConsumerSessionInfo* inactiveConsumer;

	thomsonreuters::ema::access::EmaString _serviceName;

	typedef std::map<thomsonreuters::ema::access::UInt64, thomsonreuters::ema::access::UInt64>	ItemHandleList;
	ItemHandleList itemHandleList;
};

#endif // __session_manager_h_
