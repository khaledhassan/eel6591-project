/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2016 University of Campinas (Unicamp)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */


#include "ns3/lte-helper.h"
#include "ns3/epc-helper.h"
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/ipv4-global-routing-helper.h"
#include "ns3/internet-module.h"
#include "ns3/mobility-module.h"
#include "ns3/lte-module.h"
#include "ns3/applications-module.h"
#include "ns3/point-to-point-helper.h"
#include "ns3/config-store.h"
#include <ns3/csma-module.h>
#include <ns3/ofswitch13-module.h>
#include <ns3/internet-apps-module.h>
#include <ns3/lte-controller.h>
#include <ns3/qos-controller.h>
#include <ns3/flow-monitor-helper.h>
#include <ns3/radio-bearer-stats-calculator.h>
#include <ns3/ipv4-flow-classifier.h>

using namespace ns3;
using namespace std;

/**
 * Sample simulation script for LTE+EPC. It instantiates several eNodeB,
 * attaches one UE per eNodeB starts a flow for each UE to  and from a remote host.
 * It also  starts yet another flow between each UE pair.
 */

NS_LOG_COMPONENT_DEFINE ("EpcFirstExample");

void ThroughputMonitor (FlowMonitorHelper *fmhelper, Ptr<FlowMonitor> flowMon)
	{
		std::map<FlowId, FlowMonitor::FlowStats> flowStats = flowMon->GetFlowStats();
		Ptr<Ipv4FlowClassifier> classing = DynamicCast<Ipv4FlowClassifier> (fmhelper->GetClassifier());
		for (std::map<FlowId, FlowMonitor::FlowStats>::const_iterator stats = flowStats.begin (); stats != flowStats.end (); ++stats)
		{
			Ipv4FlowClassifier::FiveTuple fiveTuple = classing->FindFlow (stats->first);
			std::cout<<"Flow ID			: " << stats->first <<" ; "<< fiveTuple.sourceAddress <<" -----> "<<fiveTuple.destinationAddress<<std::endl;
			std::cout<<"Tx Packets = " << stats->second.txPackets<<std::endl;
			std::cout<<"Rx Packets = " << stats->second.rxPackets<<std::endl;
			std::cout<<"Duration		: "<<stats->second.timeLastRxPacket.GetSeconds()-stats->second.timeFirstTxPacket.GetSeconds()<<std::endl;
			std::cout<<"Last Received Packet	: "<< stats->second.timeLastRxPacket.GetSeconds()<<" Seconds"<<std::endl;
			std::cout<<"Throughput: " << stats->second.rxBytes * 8.0 / (stats->second.timeLastRxPacket.GetSeconds()-stats->second.timeFirstTxPacket.GetSeconds())/1024  << " Mbps"<<std::endl;
			std::cout<<"---------------------------------------------------------------------------"<<std::endl;
		}
			Simulator::Schedule(Seconds(1),&ThroughputMonitor, fmhelper, flowMon);

	}

int
main (int argc, char *argv[])
{

	uint16_t numberOfNodes = 20;
	double simTime = 10.1;
	double distance = 60.0;
	double interPacketInterval = 100;

	// Command line arguments
	CommandLine cmd;
	cmd.AddValue("numberOfNodes", "Number of eNodeBs + UE pairs", numberOfNodes);
	cmd.AddValue("simTime", "Total duration of the simulation [s])", simTime);
	cmd.AddValue("distance", "Distance between eNBs [m]", distance);
	cmd.AddValue("interPacketInterval", "Inter packet interval [ms])", interPacketInterval);
	cmd.Parse(argc, argv);

	// Enable checksum computations (required by OFSwitch13 module)
	GlobalValue::Bind ("ChecksumEnabled", BooleanValue (true));

	FlowMonitorHelper fl;
    Ptr<FlowMonitor> monitor;

	// Create the switch node
	Ptr<Node> switchNode = CreateObject<Node> ();

	// Use the CsmaHelper to connect host nodes to the switch node
	CsmaHelper csmaHelper;
	csmaHelper.SetChannelAttribute ("DataRate", DataRateValue (DataRate ("100Gb/s")));
	//csmaHelper.SetChannelAttribute("Mtu", UintegerValue (1500));
	csmaHelper.SetChannelAttribute ("Delay", TimeValue (Seconds (0.010)));

	Ptr<LteHelper> lteHelper = CreateObject<LteHelper> ();
	Ptr<PointToPointEpcHelper>  epcHelper = CreateObject<PointToPointEpcHelper> ();
	lteHelper->SetEpcHelper (epcHelper);

	ConfigStore inputConfig;
	inputConfig.ConfigureDefaults();

	// parse again so you can override default values from the command line
	cmd.Parse(argc, argv);

	Ptr<Node> pgw = epcHelper->GetPgwNode ();

	// Create a single RemoteHost
	NodeContainer remoteHostContainer;
	remoteHostContainer.Create (1);
	Ptr<Node> remoteHost = remoteHostContainer.Get (0);

	NetDeviceContainer hostDevices;
	NetDeviceContainer switchPorts;
	NodeContainer hosts;
	hosts.Add(pgw);
	hosts.Add(remoteHost);
	for (size_t i = 0; i < hosts.GetN (); i++)
	{
	  NodeContainer pair (hosts.Get (i), switchNode);
	  NetDeviceContainer link = csmaHelper.Install (pair);
	  hostDevices.Add (link.Get (0));
	  switchPorts.Add (link.Get (1));
	}

	// Create the controller node
	Ptr<Node> controllerNode = CreateObject<Node> ();

	// Configure the OpenFlow network domain
	Ptr<OFSwitch13InternalHelper> of13Helper = CreateObject<OFSwitch13InternalHelper> ();
	Ptr<OFSwitch13LearningController> learnCtrl = CreateObject<OFSwitch13LearningController> ();
	of13Helper->InstallController (controllerNode, learnCtrl);
	of13Helper->InstallSwitch (switchNode, switchPorts);
	of13Helper->CreateOpenFlowChannels ();

	InternetStackHelper internet;
	internet.Install (remoteHostContainer);

	Ipv4AddressHelper ipv4h;
	ipv4h.SetBase ("1.0.0.0", "255.0.0.0");
	Ipv4InterfaceContainer internetIpIfaces = ipv4h.Assign (hostDevices);
	// interface 0 is localhost, 1 is the p2p device
	Ipv4Address remoteHostAddr = internetIpIfaces.GetAddress (1);

	Ipv4StaticRoutingHelper ipv4RoutingHelper;
	Ptr<Ipv4StaticRouting> remoteHostStaticRouting = ipv4RoutingHelper.GetStaticRouting (remoteHost->GetObject<Ipv4> ());
	remoteHostStaticRouting->AddNetworkRouteTo (Ipv4Address ("7.0.0.0"), Ipv4Mask ("255.0.0.0"), 1);

	NodeContainer ueNodes;
	NodeContainer enbNodes;
	enbNodes.Create(1);
	ueNodes.Create(numberOfNodes);

	// Install Mobility Model
	Ptr<ListPositionAllocator> positionAlloc = CreateObject<ListPositionAllocator> ();
	for (uint16_t i = 0; i < numberOfNodes; i++)
	{
	  positionAlloc->Add (Vector(distance * i, 0, 0));
	}
	MobilityHelper mobility;
	mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
	mobility.SetPositionAllocator(positionAlloc);
	mobility.Install(ueNodes);
	positionAlloc->Add (Vector(0, 0, 0));
	mobility.SetPositionAllocator(positionAlloc);
	mobility.Install(enbNodes);

	lteHelper->SetSchedulerType ("ns3::CqaFfMacScheduler");

	// Install LTE Devices to the nodes
	NetDeviceContainer enbLteDevs = lteHelper->InstallEnbDevice (enbNodes);
	NetDeviceContainer ueLteDevs = lteHelper->InstallUeDevice (ueNodes);

	// Install the IP stack on the UEs
	internet.Install (ueNodes);
	Ipv4InterfaceContainer ueIpIface;
	ueIpIface = epcHelper->AssignUeIpv4Address (NetDeviceContainer (ueLteDevs));
	// Assign IP address to UEs, and install applications
	for (uint32_t u = 0; u < ueNodes.GetN (); ++u)
	{
	  Ptr<Node> ueNode = ueNodes.Get (u);
	  // Set the default gateway for the UE
	  Ptr<Ipv4StaticRouting> ueStaticRouting = ipv4RoutingHelper.GetStaticRouting (ueNode->GetObject<Ipv4> ());
	  ueStaticRouting->SetDefaultRoute (epcHelper->GetUeDefaultGatewayAddress (), 1);
	}

	// Attach one UE per eNodeB
	for (uint16_t i = 0; i < numberOfNodes; i++)
	  {
	    lteHelper->Attach (ueLteDevs.Get(i), enbLteDevs.Get(0));
	    // side effect: the default EPS bearer will be activated
	  }

for (uint32_t u = 0; u < ueNodes.GetN (); ++u)
    {
      Ptr<NetDevice> ueDevice = ueLteDevs.Get (u);
      /*GbrQosInformation qos;
      qos.gbrDl = 132;  // bit/s, considering IP, UDP, RLC, PDCP header size
      qos.gbrUl = 132;
      qos.mbrDl = qos.gbrDl;
      qos.mbrUl = qos.gbrUl;*/
      enum EpsBearer::Qci q;
      switch (u+1) 
      {
         case 1:
            q = EpsBearer::GBR_CONV_VOICE;
            break;
         case 2:
        	 q = EpsBearer::GBR_CONV_VIDEO;
            break;
         case 3:
        	 q = EpsBearer::GBR_GAMING;
            break;
         case 4:
        	 q = EpsBearer::GBR_NON_CONV_VIDEO;
            break;
         case 5:
			 q = EpsBearer:: NGBR_IMS;
			 break;
         case 6:
			 q = EpsBearer::NGBR_VIDEO_TCP_OPERATOR;
			 break;
         case 7:
			 q = EpsBearer::NGBR_VOICE_VIDEO_GAMING;
			 break;
         case 8:
			 q = EpsBearer::NGBR_VIDEO_TCP_PREMIUM;
			 break;
         case 9:
			 q = EpsBearer::NGBR_VIDEO_TCP_DEFAULT;
			 break;
         default:
             q = EpsBearer::NGBR_VIDEO_TCP_DEFAULT;
      }

      EpsBearer bearer (q);
      //cout << bearer.qci;
      /*bearer.arp.priorityLevel = 15 - (u + 1);
      bearer.arp.preemptionCapability = true;
      bearer.arp.preemptionVulnerability = true;*/
      lteHelper->ActivateDedicatedEpsBearer (ueDevice, bearer, EpcTft::Default ());
    }


	// Install and start applications on UEs and remote host
	uint16_t ulport = 8000;
	uint16_t dlport = 3000;
	ApplicationContainer clientApps;
	ApplicationContainer serverApps;
	for (uint32_t u = 0; u < ueNodes.GetN (); ++u)
	{
	  //PacketSinkHelper dlPacketSinkHelper ("ns3::UdpSocketFactory", InetSocketAddress (Ipv4Address::GetAny (), port));

	  /*DOWNLINK SERVER->UE APP*/
	  /*PacketSinkHelper dlClientPacketSinkHelper ("ns3::UdpSocketFactory", InetSocketAddress (ueIpIface.GetAddress (u), dlport));
	  clientApps.Add (dlClientPacketSinkHelper.Install (ueNodes.Get(u)));

	  UdpClientHelper ulServer (ueIpIface.GetAddress (u), ulport);
	  ulServer.SetAttribute ("Interval", TimeValue (MilliSeconds(interPacketInterval)));
	  ulServer.SetAttribute ("MaxPackets", UintegerValue(1000000));
	  ulServer.SetAttribute ("PacketSize", UintegerValue(1024));
	  serverApps.Add (ulServer.Install (remoteHostContainer));*/


	  /*UPLINK UE->SERVER APP*/
	  PacketSinkHelper dlServerPacketSinkHelper ("ns3::UdpSocketFactory", InetSocketAddress (remoteHostAddr, dlport));
	  serverApps.Add (dlServerPacketSinkHelper.Install (remoteHostContainer));

	  UdpClientHelper ulClient (remoteHostAddr, ulport);
	  ulClient.SetAttribute ("Interval", TimeValue (MilliSeconds(interPacketInterval)));
	  ulClient.SetAttribute ("MaxPackets", UintegerValue(1000000));
	  ulClient.SetAttribute ("PacketSize", UintegerValue(1024));
	  clientApps.Add (ulClient.Install (ueNodes.Get(u)));

	  
	  dlport++;
	  ulport++;

	}
	serverApps.Start (Seconds (0.01));
	clientApps.Start (Seconds (0.01));
	

	monitor = fl.Install(ueNodes);
    monitor = fl.Install(remoteHost);

   /*monitor->SetAttribute ("DelayBinWidth", DoubleValue(0.001));
   monitor->SetAttribute ("JitterBinWidth", DoubleValue(0.001));
   monitor->SetAttribute ("PacketSizeBinWidth", DoubleValue(20));*/

   Simulator::Schedule(Seconds(3),&ThroughputMonitor,&fl, monitor);

	//lteHelper->EnableTraces ();
	// Uncomment to enable PCAP tracing
	//p2ph.EnablePcapAll("lena-epc-first");


	Simulator::Stop(Seconds(simTime));
	Simulator::Run();

	ThroughputMonitor(&fl, monitor);

	//monitor->SerializeToXmlFile ("results.xml" , true, true );

	/*GtkConfigStore config;
	config.ConfigureAttributes();*/

	Simulator::Destroy();
	return 0;

}

