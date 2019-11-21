///*|-----------------------------------------------------------------------------
// *|            This source code is provided under the Apache 2.0 license      --
// *|  and is provided AS IS with no warranty or guarantee of fit for purpose.  --
// *|                See the project's LICENSE.md for details.                  --
// *|           Copyright Thomson Reuters 2015. All rights reserved.            --
///*|-----------------------------------------------------------------------------

#include "SessionManager.h"

using namespace thomsonreuters::ema::access;
using namespace thomsonreuters::ema::rdm;
using namespace std;

void SessionManager::defineActiveConsumer()
{
	if (primaryConsumer.isServiceUp)
	{
		//Discover case
		activeConsumer = &primaryConsumer;
		inactiveConsumer = &backupConsumer;
	}
	else if (backupConsumer.isServiceUp)
	{
		//Failover case
		activeConsumer = &backupConsumer;
		inactiveConsumer = &primaryConsumer;
	}
	else
	{
		//Primary always be active.
		activeConsumer = &primaryConsumer;
		inactiveConsumer = &backupConsumer;
	}
}

UInt64 SessionManager::registerClient(ReqMsg& reqMsg, OmmConsumerClient& client, void* closure)
{
	UInt64 handle1,handle2, keyHandle;

	//Normal request for Active Consumer
	handle1 = activeConsumer->ommConsumer->registerClient(reqMsg.pause(false), client, closure);
	//Pause request for Inactive Consumer 
	handle2 = inactiveConsumer->ommConsumer->registerClient(reqMsg.pause(true), client, closure);
	
	//Keep Item handle for further usage (i.e. unregisterClient call)
	if (activeConsumer == &primaryConsumer)
	{
		itemHandleList.insert(ItemHandleList::value_type(handle1, handle2));
		keyHandle = handle1;
	}
	else
	{
		itemHandleList.insert(ItemHandleList::value_type(handle2, handle1));
		keyHandle = handle2;
	}
	return keyHandle;
}

void SessionManager::create(OmmConsumer& mConsumer, OmmConsumer& bConsumer, EmaString servicename)
{
	setMainOmmConsumer(mConsumer);
	setBackupOmmConsumer(bConsumer);
	_serviceName = servicename;

	if (primaryConsumer.ommConsumer != NULL && backupConsumer.ommConsumer != NULL)
	{
		//Send Login request for both Consumers
		primaryConsumer.loginHandle = primaryConsumer.ommConsumer->registerClient(ReqMsg().domainType(MMT_LOGIN), *this, &primaryConsumer);
		backupConsumer.loginHandle = backupConsumer.ommConsumer->registerClient(ReqMsg().domainType(MMT_LOGIN), *this, &backupConsumer);

	if (primaryConsumer.loginHandle != NULL && backupConsumer.loginHandle != NULL)
	{
		//Open Directory streams for both Consumers
		primaryConsumer.dirHandle = primaryConsumer.ommConsumer->registerClient(ReqMsg().domainType(MMT_DIRECTORY).serviceName(_serviceName), *this, &primaryConsumer);
		backupConsumer.dirHandle = backupConsumer.ommConsumer->registerClient(ReqMsg().domainType(MMT_DIRECTORY).serviceName(_serviceName), *this, &backupConsumer);
	}
	}

	while (primaryConsumer.isInitialized == false || backupConsumer.isInitialized == false)
	{
		Sleep(1000);
	}
	cout << endl << "DONE" << endl;
}

void SessionManager::setMainOmmConsumer(OmmConsumer& consumer)
{
	primaryConsumer.ommConsumer = &consumer;
	primaryConsumer.isServiceUp = false;
	primaryConsumer.isInitialized = false;
}

void SessionManager::setBackupOmmConsumer(OmmConsumer& consumer)
{
	backupConsumer.ommConsumer = &consumer;
	backupConsumer.isServiceUp = false;
	backupConsumer.isInitialized = false;
}

SessionManager::SessionManager() :
	_serviceName(""),
	activeConsumer(0)
{
	itemHandleList.clear();
}

void SessionManager::handleEvent()
{
	//Discover to main Consumer
	if (primaryConsumer.isServiceUp == true && isActive(&backupConsumer))
	{
		cout << "Discover to main Consumer" << endl;
		//Pause Backup's stream and resume Primary's Stream
		backupConsumer.ommConsumer->reissue(ReqMsg().domainType(MMT_LOGIN).initialImage(false).pause(true).name("user"), backupConsumer.loginHandle);
		primaryConsumer.ommConsumer->reissue(ReqMsg().domainType(MMT_LOGIN).initialImage(false).interestAfterRefresh(true).name("user"), primaryConsumer.loginHandle);
	}
	//failover to backup Consumer
	else if ((primaryConsumer.isServiceUp == false && backupConsumer.isServiceUp == true) && isActive(&primaryConsumer))
	{
		cout << "Failover to backup Consumer" << endl;
		//Pause Primary's stream and resume Backup's Stream
		primaryConsumer.ommConsumer->reissue(ReqMsg().domainType(MMT_LOGIN).initialImage(false).pause(true).name("user"), primaryConsumer.loginHandle);
		backupConsumer.ommConsumer->reissue(ReqMsg().domainType(MMT_LOGIN).initialImage(false).interestAfterRefresh(true).name("user"), backupConsumer.loginHandle);
	}
}

void SessionManager::onRefreshMsg( const RefreshMsg& refreshMsg, const OmmConsumerEvent& ommEvent )
{
	if (ommEvent.getHandle() == primaryConsumer.dirHandle)
		primaryConsumer.isInitialized = true;
	else
		backupConsumer.isInitialized = true;

	if (primaryConsumer.isInitialized && backupConsumer.isInitialized)
	{
		if (refreshMsg.getDomainType() == MMT_DIRECTORY)
		{
			decodeDirectory(refreshMsg, (ConsumerSessionInfo *)ommEvent.getClosure());

			if (primaryConsumer.isInitialized && backupConsumer.isInitialized)
			{
				if (activeConsumer!=0)
					cout << endl << "active consumer: " << activeConsumer->ommConsumer->getConsumerName() << endl;

				//verify whether there are changes on service status of each Conusmers
				handleEvent();

				//define new Active Consumer
				defineActiveConsumer();
			}
		}
	}
}

void SessionManager::onUpdateMsg( const UpdateMsg& updateMsg, const OmmConsumerEvent& ommEvent )
{
	if (primaryConsumer.isInitialized && backupConsumer.isInitialized)
	{
		if (updateMsg.getDomainType() == MMT_DIRECTORY)
		{
			decodeDirectory(updateMsg, (ConsumerSessionInfo *)ommEvent.getClosure());

			if (primaryConsumer.isInitialized && backupConsumer.isInitialized)
			{
				if (activeConsumer != 0)
					cout << endl << "active consumer: " << activeConsumer->ommConsumer->getConsumerName() << endl;

				//verify whether there are changes on service status of each Conusmers
				handleEvent();

				//define new Active Consumer
				defineActiveConsumer();
			}
		}
	}
}

void SessionManager::onStatusMsg( const StatusMsg& statusMsg, const OmmConsumerEvent& ommEvent )
{

	if (ommEvent.getHandle() == primaryConsumer.dirHandle)
		primaryConsumer.isInitialized = true;
	else
		backupConsumer.isInitialized = true;
	cout << endl << "OnStatus received." << endl;
	if (primaryConsumer.isInitialized && backupConsumer.isInitialized)
	{
		if (statusMsg.getDomainType() == MMT_LOGIN)
		{
			cout << "Login status received." << endl;
			decodeLoginState(statusMsg.getState(), (ConsumerSessionInfo *)ommEvent.getClosure());

			if (activeConsumer != 0)
				cout << endl << "active consumer: " << activeConsumer->ommConsumer->getConsumerName() << endl;

			//verify whether there are changes on service status of each Conusmers
			handleEvent();

			//define new Active Consumer
			defineActiveConsumer();
		}
	}
}

void SessionManager::decodeLoginState(const OmmState& state, ConsumerSessionInfo* info)
{
	if (state.getStreamState() == OmmState::OpenEnum)
	{
		if (state.getDataState() == OmmState::SuspectEnum)
			info->isServiceUp = false;
	}
}

void SessionManager::decodeDirectory( const Msg& msg, ConsumerSessionInfo* info )
{
	std::cout << "Received directory" << endl;
	bool serviceState, acceptingReq = false;
	if (msg.getDomainType() == MMT_DIRECTORY)
		if (msg.getPayload().getDataType() == DataType::MapEnum)
		{
			//Get Map from Payload
			const Map& map = msg.getPayload().getMap();
			while (map.forth())
			{
				const MapEntry& me = map.getEntry();
				if (me.getLoadType() == DataType::FilterListEnum)
				{
					const FilterList& ftl = me.getFilterList();
					while (ftl.forth())
					{
						const FilterEntry& fe = ftl.getEntry();
						//Decode "SERVICE_STATE_FILTER" Filter Entry.
						if (fe.getFilterId() == thomsonreuters::ema::rdm::SERVICE_STATE_FILTER)
						{
							//Get Element List from the Filter Entry.
							const ElementList& el = fe.getElementList();
							while (el.forth())
							{
								const ElementEntry& ee = el.getEntry();
								EmaString name = ee.getName();
								//Decode "ServiceState" Element Entry
								if (name == thomsonreuters::ema::rdm::ENAME_SVC_STATE)
									serviceState = ee.getUInt() == 1 ? true : false;
								//if (name == thomsonreuters::ema::rdm::ENAME_ACCEPTING_REQS)
								//	acceptingReq = ee.getUInt() == 1 ? true : false;
								}
							if (serviceState)
								info->isServiceUp = true;
							else
								info->isServiceUp = false;

								cout << "new Service status: " << info->isServiceUp << endl;
							}
						}
					}
				}
			}
}