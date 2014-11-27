#include "ns3/point-to-point-module.h"
#include "ns3/wifi-module.h"
#include "ns3/mobility-module.h"

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/applications-module.h"
#include "ns3/csma-module.h"
#include "ns3/netanim-module.h"
#include "ns3/internet-module.h"
#include "ns3/ipv4-global-routing-helper.h"
#include <math.h>

using namespace ns3;

NS_LOG_COMPONENT_DEFINE( "BasicNetwork" );

int main( int argc, char *argv[] )
{
	bool tracing = false;
	uint32_t megaBytesToSend = 0;
	double nClients = 1;
	double nServers = 1;

	CommandLine cmd;
	cmd.AddValue( "tracing", "Enable/disable tracing", tracing );
	cmd.AddValue( "megaBytesToSend", "Megabytes application will send", megaBytesToSend );
	cmd.AddValue( "nClients", "Number of clients", nClients );
	cmd.AddValue( "nServers", "Number of servers", nServers );
	cmd.Parse( argc, argv );

	NS_LOG_UNCOND( "Creating nodes..." );
	std::vector<NodeContainer> csNodes( nServers );
	std::vector<NodeContainer> clientNodes( nServers );
	NodeContainer serverNodes;
	std::vector<NodeContainer>::iterator clientNodesIt;
	std::vector<NodeContainer>::iterator csNodesIt;

	double ratio = ceil( nClients / nServers );
	std::cout << "Ratio = " << ratio << std::endl;
	double nodesCreated = 0;
	uint32_t x;
	uint32_t y;

	serverNodes.Create( nServers );
	NS_LOG_UNCOND( "All server nodes created." );

	// for loop in order to create nodes
	x = 0;
	csNodesIt = csNodes.begin();
	for( clientNodesIt = clientNodes.begin(); clientNodesIt != clientNodes.end(); ++clientNodesIt )
	{
		( *csNodesIt ).Add( serverNodes.Get( x ) );	// place the server node reference to csNodes
		// create nodes at clientNodes and register them at cs nodes too
		if( x != nServers - 1)
		{
			( *clientNodesIt ).Create( ratio );
			( *csNodesIt ).Add( *clientNodesIt );
			nodesCreated += ratio;
		}
		else
		{
			( *clientNodesIt ).Create( nClients - nodesCreated);
			( *csNodesIt ).Add( *clientNodesIt );
			nodesCreated += nClients - nodesCreated;
		}
		NS_LOG_UNCOND( "Client nodes created: " << nodesCreated );
		x++;
		++csNodesIt;
	}
	NS_LOG_UNCOND( "All client nodes created." );

	// integrity check for nodes (do they point to same nodes?)
	x = 0;
	csNodesIt = csNodes.begin();
	bool csVsServer;
	std::string boolResult;
	NS_LOG_DEBUG( "serverNodes.GetN() = " << serverNodes.GetN() );
	for( clientNodesIt = clientNodes.begin(); clientNodesIt != clientNodes.end(); ++clientNodesIt )
	{
		csVsServer = ( *csNodesIt ).Get( 0 ) == serverNodes.Get( x );
		if( csVsServer == 1 ) boolResult = "Yes";
		else boolResult = "No";

		NS_LOG_DEBUG( "CS Node " << x << " == Server Node " << x << "?: " << boolResult );

		y = 0;
		bool csVsClient;
		NodeContainer::Iterator csNode;
		csNode = ( *csNodesIt ).Begin();
		for( ++csNode; csNode != ( *csNodesIt ).End(); ++csNode )
		{
			if( csVsClient == 1 ) boolResult = "Yes";
			else boolResult = "No";
			csVsClient = ( *csNode ) == ( *clientNodesIt ).Get( y );
			NS_LOG_DEBUG( "CS Node " << x << "." << y << " == Client Node " << x << "." << y << "?: " << boolResult );
			y++;
		}
		x++;
		++csNodesIt;
	}
}

