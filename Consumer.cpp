///*|-----------------------------------------------------------------------------
// *|            This source code is provided under the Apache 2.0 license      --
// *|  and is provided AS IS with no warranty or guarantee of fit for purpose.  --
// *|                See the project's LICENSE.md for details.                  --
// *|           Copyright Thomson Reuters 2015. All rights reserved.            --
///*|-----------------------------------------------------------------------------

#include "Consumer.h"

#include "SessionManager.h""

using namespace refinitiv::ema::access;
using namespace refinitiv::ema::rdm;
using namespace std;

void AppClient::onRefreshMsg( const RefreshMsg& refreshMsg, const OmmConsumerEvent& ommEvent )
{
	cout << endl << "Refresh";
	cout << endl << "Host: " << ommEvent.getChannelInformation().getHostname();
	cout << endl << "Handle: " << ommEvent.getHandle() << " Closure: " << ommEvent.getClosure();
	cout << endl << "Item Name: " << ( refreshMsg.hasName() ? refreshMsg.getName() : EmaString( "<not set>" ) ) << endl
		<< "Service Name: " << (refreshMsg.hasServiceName() ? refreshMsg.getServiceName() : EmaString( "<not set>" ) );

	cout << endl << "Item State: " << refreshMsg.getState().toString() << endl;

	if ( DataType::FieldListEnum == refreshMsg.getPayload().getDataType() )
		decode( refreshMsg.getPayload().getFieldList() );
}

void AppClient::onUpdateMsg( const UpdateMsg& updateMsg, const OmmConsumerEvent& ommEvent )
{
	cout << endl << "Update";
	cout << endl << "Host: " << ommEvent.getChannelInformation().getHostname();
	cout << endl << "Handle: " << ommEvent.getHandle() << " Closure: " << ommEvent.getClosure();

	cout << endl << "Item Name: " << ( updateMsg.hasName() ? updateMsg.getName() : EmaString( "<not set>" ) ) << endl
		<< "Service Name: " << (updateMsg.hasServiceName() ? updateMsg.getServiceName() : EmaString( "<not set>" ) ) << endl;

	if ( DataType::FieldListEnum == updateMsg.getPayload().getDataType() )
		decode( updateMsg.getPayload().getFieldList() );
}

void AppClient::onStatusMsg( const StatusMsg& statusMsg, const OmmConsumerEvent& ommEvent )
{
	cout << endl << "Host: " << ommEvent.getChannelInformation().getHostname();
	cout << endl << "Handle: " << ommEvent.getHandle() << " Closure: " << ommEvent.getClosure();

	cout << endl << "Item Name: " << ( statusMsg.hasName() ? statusMsg.getName() : EmaString( "<not set>" ) ) << endl
		<< "Service Name: " << (statusMsg.hasServiceName() ? statusMsg.getServiceName() : EmaString( "<not set>" ) );

	if ( statusMsg.hasState() )
		cout << endl << "Item State: " << statusMsg.getState().toString() << endl;
}

void AppClient::decode( const FieldList& fl )
{
	while (fl.forth("BID"))	// look for a fid with matching name
		cout << "Fid: " << fl.getEntry().getFieldId() << " Name: " << fl.getEntry().getName() << " value: " << fl.getEntry().getLoad().toString() << endl;
}

AppClient::AppClient()
{
}

int main()
{
	try {
		AppClient client;
		SessionManager sessionManager;
		OmmConsumer consumer( OmmConsumerConfig().username( "user" ).consumerName("Consumer_1"));
		OmmConsumer consumer2(OmmConsumerConfig().username( "user" ).consumerName("Consumer_2"));

		sessionManager.create(consumer, consumer2, "DIRECT_FEED");
		sessionManager.registerClient( ReqMsg().serviceName( "DIRECT_FEED" ).name( "JPY=" ), client);

		sleep(20000);
		cout << "request second items" << endl;
		sessionManager.registerClient( ReqMsg().serviceName( "DIRECT_FEED" ).name( "EUR=" ), client);

		sleep( 600000000 );			// API calls onRefreshMsg(), onUpdateMsg(), or onStatusMsg()
	}
	catch ( const OmmException& excp ) {
		cout << excp << endl;
	}
	return 0;
}
