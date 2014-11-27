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

// The number of bytes to send in this simulation.
static const uint32_t totalTxBytes = 10000000;
static uint32_t currentTxBytes = 0;
// Perform series of 1040 byte writes (this is a multiple of 26 since
// we want to detect data splicing in the output stream)
static const uint32_t writeSize = 1040;
uint8_t data[writeSize];

void StartFlow (Ptr<Socket>, Ipv4Address, uint16_t);
void WriteUntilBufferFull (Ptr<Socket>, uint32_t);

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

	uint32_t x, createdNodes;
	uint32_t ratio = ( uint32_t ) ceil( nClients / nServers );

	NodeContainer csNodes[ nServers ];
	NodeContainer serverNodes;
	std::vector<NodeContainer> clientNodes( nServers );
	
	for( x = 0, createdNodes = 0; x < nServers; x++ )
	{
		std::vector<NodeContainer>::iterator clientIt = clientNodes.begin();
		csNodes[ x ].Create( 1 );
		serverNodes = NodeContainer( csNodes[ x ].Get( 0 ) );
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
	for( x = 0; x < nServers; x++ )
	{
		csmaDevices[ x ] = csma.Install( csNodes[ x ] );
		stack.Install( csNodes[ x ] );
	}

	Ipv4AddressHelper address;
	std::vector<Ipv4InterfaceContainer> csInterfaces( nServers );
	for( x = 0; x < nServers; x++ )
	{
		std::vector<Ipv4InterfaceContainer>::iterator csInterIt = csInterfaces.begin();
		std::ostringstream subnet;
		subnet << "10.1." << x + 1 << ".0";
		address.SetBase( subnet.str().c_str(), "255.255.255.0" );
		*csInterIt = address.Assign( csmaDevices[ x ] );
		++csInterIt;
	}

	Ipv4GlobalRoutingHelper::PopulateRoutingTables();

	uint16_t port = 50000;
	Address sinkLocalAddress( InetSocketAddress( Ipv4Address::GetAny(), port ) );
	PacketSinkHelper sinkHelper( "ns3::TcpSocketFactory", sinkLocalAddress );
	std::vector<ApplicationContainer> clientApps( nServers );
	for( std::vector<NodeContainer>::iterator clientNodesIt = clientNodes.begin(); clientNodesIt != clientNodes.end(); ++clientNodesIt )
	{
		std::vector<ApplicationContainer>::iterator clientAppsIt = clientApps.begin();
		*clientAppsIt = sinkHelper.Install( *clientNodesIt );
		(*clientAppsIt).Start( Seconds( 1.0 ) );
		(*clientAppsIt).Stop( Seconds( 50.0 ) );
		++clientAppsIt;
	}

	for( NodeContainer::Iterator serverNode = serverNodes.Begin(); serverNode != serverNodes.End(); ++serverNode )
	{
		Ptr<Socket> localSocket = Socket::CreateSocket( *serverNode, TcpSocketFactory::GetTypeId() );
		localSocket->Bind();
		for( std::vector<Ipv4InterfaceContainer>::iterator csInterIt = csInterfaces.begin(); csInterIt != csInterfaces.end(); ++csInterIt )
		{
			Simulator::ScheduleNow( &StartFlow, localSocket, (*csInterIt).GetAddress( 1 ), port );
		}
	}

	AnimationInterface anim( "animation.xml" );
	/*
	for( x = 0; x < nServers; x++ )
	{
		anim.SetConstantPosition( serverNodes.Get( x ), 0, 2 * x );
		
	}
	x = 0;
	for( std::vector<NodeContainer>::iterator clientNodeContainer = clientNodes.begin(); clientNodeContainer != clientNodes.end(); ++clientNodeContainer )
	{
		y = 0;
		for( NodeContainer::Iterator clientNode = (*clientNodeContainer).Begin(); clientNode != (*clientNodeContainer).End(); ++clientNode )
		{
			anim.SetConstantPosition( *clientNode, 1 * x, ( 1 * y ) + 1 );
			y++;
		}
		x++;
	}*/
		/*
	for( x = 0; x < nServers; x++ )
	{
		y= 0;
		for( NodeContainer::Iterator node = csNodes[ x ].Begin(); node != csNodes[ x ].End(); ++node )
		{
			anim.SetConstantPosition( *node, y, x + 1 );
			y++;
		}
	}
	*/
	anim.SetConstantPosition( csNodes[ 0 ].Get( 0 ), 0, 0 );
	anim.SetConstantPosition( csNodes[ 0 ].Get( 1 ), 1, 0 );

	Simulator::Stop (Seconds (1000));
	Simulator::Run ();
	Simulator::Destroy ();
}

void StartFlow (Ptr<Socket> localSocket,
                Ipv4Address servAddress,
                uint16_t servPort)
{
  NS_LOG_LOGIC ("Starting flow at time " <<  Simulator::Now ().GetSeconds ());
  localSocket->Connect (InetSocketAddress (servAddress, servPort)); //connect

  // tell the tcp implementation to call WriteUntilBufferFull again
  // if we blocked and new tx buffer space becomes available
  localSocket->SetSendCallback (MakeCallback (&WriteUntilBufferFull));
  WriteUntilBufferFull (localSocket, localSocket->GetTxAvailable ());
}

void WriteUntilBufferFull (Ptr<Socket> localSocket, uint32_t txSpace)
{
  while (currentTxBytes < totalTxBytes && localSocket->GetTxAvailable () > 0) 
    {
      uint32_t left = totalTxBytes - currentTxBytes;
      uint32_t dataOffset = currentTxBytes % writeSize;
      uint32_t toWrite = writeSize - dataOffset;
      toWrite = std::min (toWrite, left);
      toWrite = std::min (toWrite, localSocket->GetTxAvailable ());
      int amountSent = localSocket->Send (&data[dataOffset], toWrite, 0);
      if(amountSent < 0)
        {
          // we will be called again when new tx space becomes available.
          return;
        }
      currentTxBytes += amountSent;
    }
  localSocket->Close ();
}
