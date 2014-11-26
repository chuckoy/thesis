#include "ns3/point-to-point-module.h"
#include "ns3/wifi-module.h"
#include "ns3/mobility-module.h"


#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/applications-module.h"
#include "ns3/csma-module.h"
#include "ns3/netanim-module.h"
#include "ns3/internet-module.h"
#include <math.h>

using namespace ns3;

NS_LOG_COMPONENT_DEFINE( "TestNetwork" );

int main ( int argc, char *argv[] )
{
	bool verbose = true;
	uint32_t nClients = 1;
	uint32_t nServers = 1;

	CommandLine cmd;
	cmd.AddValue( "nClients", "Number of clients", nClients );
	cmd.AddValue( "nServers", "Number of servers", nServers );
	cmd.AddValue( "verbose", "Tell echo applications to log if true", verbose );

	cmd.Parse( argc, argv );	

	int x, y, createdNodes;
	uint32_t ratio = ( uint32_t ) ceil( nClients / nServers );

	NodeContainer csNodes[ nServers ];
	NodeContainer serverNodes;
	std::vector<NodeContainer> clientNodes( nServers );
	
	for( x = 0, createdNodes = 0; x < nServers; x++ )
	{
		std::vector<NodeContainer>::iterator clientIt = clientNodes.begin();
		csNodes[ x ].Create( 1 );
		serverNodes.Add( csNodes[ x ].Get( 0 ) );
		if( x != nServers - 1 )
		{
			csNodes[ x ].Create( ratio );
			*clientIt = NodeContainer( csNodes[ x ] );
			createdNodes += ratio;
		}
		else
		{
			csNodes[ x ].Create( nClients - createdNodes );
			*clientIt = NodeContainer( csNodes[ x ] );
		}
		++clientIt;
	}

	CsmaHelper csma;
	csma.SetChannelAttribute( "DataRate", StringValue( "1Mbps" ) );
	csma.SetChannelAttribute( "Delay", TimeValue( NanoSeconds( 6560 ) ) );

	NetDeviceContainer csmaDevices[ nServers ];
	InternetStackHelper stack;
	for( x = 0; x < csNodes.GetN(); x++ )
	{
		csmaDevices[ x ] = csma.Install( csNodes[ x ] );
		stack.Install( csNodes[ x ] );
	}

	Ipv4AddressHelper address;
	Ipv4InterfaceContainer csInterfaces[ nServers ];
	for( x = 0; x < csNodes.GetN(); x++ )
	{
		std::ostringstream subnet;
		subnet << "10.1." << x + 1 << ".0";
		address.SetBase( subnet.str(), "255.255.255.0" );
		csInterfaces[ x ] = address.Assign( csmaDevices[ x ] );
	}

	Ipv4GlobalRoutingHelper::PopulateRoutingTables();

	uint16_t port = 50000;
	Address sinkLocalAddress( InetSocketAddress( Ipv4Address::GetAny(), port ) );
	PacketSinkHelper sinkHelper( "ns3::TcpSocketFactory", sinkLocalAddress );
	std::vector<ApplicationContainer> clientApps( nClients );
	for( x = 0; x < clientNodes.GetN(); x++ )
	{
		for( y = 0; y < clientNodes[ x ].GetN(); y++ )
		{
			clientApps[ x ] = sink.Install( clientNodes[ x ].Get( y ) );
		}
	}
}