#ifndef __TCPSERVERAPP_H_INCLUDED__
#define __TCPSERVERAPP_H_INCLUDED__

#include "ns3/applications-module.h"

using namespace ns3;

class TcpServerApp : public Application
{
	public:
		MyApp();
		virtual ~MyApp();

		void Setup( Ptr<Socket> socket, Address address, uint32_t packetSize, uint32_t nPackets, DataRate dataRate );

	private:
		virtual void StartApplication( void );
		virtual void StopApplication( void );

		void ScheduleTx( void );
		void SendPacket( void );

		Ptr<Socket>		m_socket;
		Address			
};

#endif // __TCPSERVERAPP_H_INCLUDED__