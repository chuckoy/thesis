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

	std::vector<NodeContainer> csNodes( nServers );
	std::vector<NodeContainer> clientNodes( nServers );
	NodeContainer serverNodes;
	
	std::vector<NodeContainer>::iterator clientNodesIt;
	std::vector<NodeContainer>::iterator csNodesIt;
	NodeContainer::Iterator csNode;

	std::string boolResult;

	double ratio = ceil( nClients / nServers );
	std::cout << "Ratio = " << ratio << std::endl;
	double nodesCreated = 0;
	uint32_t x;
	uint32_t y;

	NS_LOG_UNCOND( "Creating nodes..." );
	serverNodes.Create( nServers );
	NS_LOG_UNCOND( "All server nodes created." );

	// for loop in order to create nodes
	x = 0;
	csNodesIt = csNodes.begin();
	for( clientNodesIt = clientNodes.begin(); clientNodesIt != clientNodes.end(); ++clientNodesIt )
	{
		( *csNodesIt ).Add( serverNodes.Get( x ) );	// place the server node reference to csNodes
		// create nodes at clientNodes and register them at cs nodes too
		if( nodesCreated + ratio < nClients )
		{
			( *clientNodesIt ).Create( ratio );
			( *csNodesIt ).Add( *clientNodesIt );
			nodesCreated += ratio;
		}
		/* when we're at the last node container, add the remaining
		 * nodes instead
		 */
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
	// todo: find some way to run this only if debugging mode is on?
	x = 0;
	csNodesIt = csNodes.begin();
	bool csVsServer;
	bool csVsClient;
	NS_LOG_DEBUG( "serverNodes.GetN() = " << serverNodes.GetN() );
	for( clientNodesIt = clientNodes.begin(); clientNodesIt != clientNodes.end(); ++clientNodesIt )
	{
		csVsServer = ( *csNodesIt ).Get( 0 ) == serverNodes.Get( x );
		if( csVsServer == 1 ) boolResult = "Yes";
		else boolResult = "No";

		NS_LOG_DEBUG( "CS Node " << x << " == Server Node " << x << "?: " << boolResult );

		y = 0;
		csNode = ( *csNodesIt ).Begin();
		for( ++csNode; csNode != ( *csNodesIt ).End(); ++csNode )
		{
			csVsClient = ( *csNode ) == ( *clientNodesIt ).Get( y );
			if( csVsClient == 1 ) boolResult = "Yes";
			else boolResult = "No";			

			NS_LOG_DEBUG( "CS Node " << x << "." << y << " == Client Node " << x << "." << y << "?: " << boolResult );

			y++;
		}
		x++;
		++csNodesIt;
	}

	// install internet stacks on all nodes
	InternetStackHelper stack;
	x = 0;
	stack.InstallAll();
	NS_LOG_UNCOND( "Internet stacks installed." );

	// define csma channel attributes
	CsmaHelper csma;
	csma.SetChannelAttribute( "DataRate", StringValue( "1Mbps" ) );
	csma.SetChannelAttribute( "Delay", TimeValue( NanoSeconds( 6560 ) ) );

	std::vector<NetDeviceContainer> csmaDevices( nServers );
	std::vector<NetDeviceContainer>::iterator csmaDevIt;
	Ipv4AddressHelper address;
	std::vector<Ipv4InterfaceContainer> csmaInterfaces( nServers );
	std::vector<Ipv4InterfaceContainer>::iterator csmaInterIt;

	// install csma net devices on nodes and assign addresses
	x = 0;
	NS_LOG_UNCOND( "Installing CSMA net devices and IP addresses..." );
	for( csmaDevIt = csmaDevices.begin(); csmaDevIt != csmaDevices.end(); ++csmaDevIt)
	{
		csNodesIt = csNodes.begin();
		csmaInterIt = csmaInterfaces.begin();

		*csmaDevIt = csma.Install( *csNodesIt );
		NS_LOG_UNCOND( "CSMA net devices installed on csNodeContainer " << x << "." );

		std::ostringstream addressString;
		addressString << "10.1." << x + 1 << ".0";
		address.SetBase( addressString.str().c_str(), "255.255.255.0" );
		*csmaInterIt = address.Assign( *csmaDevIt );
		NS_LOG_UNCOND( "Subnet " << addressString.str() << " assigned to csmaDeviceContainer " << x );

		++csmaInterIt;
		++csNodesIt;
		x++;
	}
	NS_LOG_UNCOND( "CSMA net devices with IP addresses installed." );

}