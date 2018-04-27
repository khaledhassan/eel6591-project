/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
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

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/lte-module.h"
#include "ns3/mobility-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include "ns3/eps-bearer.h"
#include "ns3/netanim-module.h"
#include "ns3/lte-helper.h"
#include "ns3/epc-helper.h"
#include "ns3/lte-net-device.h"
#include "ns3/lte-ue-net-device.h"
#include "ns3/lte-handover-algorithm.h"


using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("DMM_MOBILITY");

static void
CourseChange (std::string foo, Ptr<const MobilityModel> mobility)
{
  Vector pos = mobility->GetPosition ();
  Vector vel = mobility->GetVelocity ();
  std::cout << Simulator::Now () << ", model=" << mobility << ", POS: x=" << pos.x << ", y=" << pos.y
            << ", z=" << pos.z << "; VEL:" << vel.x << ", y=" << vel.y
            << ", z=" << vel.z << std::endl;
}

int
main (int argc, char *argv[])
{

/***********************************************************
 * Log level and coommand line parsing                     *
 ***********************************************************/
  LogLevel logLevel = (LogLevel)(LOG_PREFIX_ALL | LOG_LEVEL_INFO);

//  LogComponentEnable ("LteHelper", logLevel);
//  LogComponentEnable ("EpcHelper", logLevel);
//  LogComponentEnable ("EmuEpcHelper", logLevel);
//  LogComponentEnable ("EpcEnbApplication", logLevel);
//  LogComponentEnable ("EpcX2", logLevel);
//  LogComponentEnable ("EpcSgwPgwApplication", logLevel);

//  LogComponentEnable ("LteEnbRrc", logLevel);
//  LogComponentEnable ("LteEnbNetDevice", logLevel);
//  LogComponentEnable ("LteUeRrc", logLevel);
//  LogComponentEnable ("LteUeNetDevice", logLevel);
//  LogComponentEnable ("MobilityHelper", logLevel);

  LogComponentEnable ("UdpClient", logLevel);

  Time::SetResolution (Time::NS);
  LogComponentEnable ("UdpEchoClientApplication", LOG_LEVEL_INFO);
  LogComponentEnable ("UdpEchoServerApplication", LOG_LEVEL_INFO);

  // change some default attributes so that they are reasonable for
  // this scenario, but do this before processing command line
  // arguments, so that the user is allowed to override these settings
  Config::SetDefault ("ns3::UdpClient::Interval", TimeValue (MilliSeconds (10)));
  Config::SetDefault ("ns3::UdpClient::MaxPackets", UintegerValue (1000000));
  Config::SetDefault ("ns3::LteHelper::UseIdealRrc", BooleanValue (true));

  // Command line arguments
  CommandLine cmd;

  double speed = 20;       // m/s
  double enbTxPowerDbm = 25.0;
  double simTime = 2; //TODO/XXX old value: (double)(numberOfEnbs + 1) * distance / speed; // 1500 m / 20 m/s = 75 secs

  cmd.AddValue ("speed", "Speed of the UE (default = 20 m/s)", speed);
  cmd.AddValue ("enbTxPowerDbm", "TX power [dBm] used by HeNBs (default = 25.0)", enbTxPowerDbm);
  cmd.AddValue ("simTime", "Total duration of the simulation (in seconds, default = 15)", simTime);
  std::string animFile = "ProjectAnimation.xml" ;  // Name of file for animation output
  cmd.Parse (argc, argv);



/***********************************************************
 * Create LTE, EPC, and UE/eNB Nodes                       *
 ***********************************************************/
  Ptr<LteHelper> lteHelper = CreateObject<LteHelper> ();
  Ptr<PointToPointEpcHelper> epcHelper = CreateObject<PointToPointEpcHelper> ();
  lteHelper->SetEpcHelper (epcHelper);
  lteHelper->SetSchedulerType ("ns3::RrFfMacScheduler");

  //  lteHelper->SetHandoverAlgorithmType ("ns3::A3RsrpHandoverAlgorithm");
  //  lteHelper->SetHandoverAlgorithmAttribute ("Hysteresis",
  //                                            DoubleValue (3.0));
  //  lteHelper->SetHandoverAlgorithmAttribute ("TimeToTrigger",
  //                                            TimeValue (MilliSeconds (256)));

  Ptr<Node> pgw = epcHelper->GetPgwNode (); // TODO/XXX: used later?


//creates 20 nodes we can use as mobile nodes
  NodeContainer ueNodes;
  ueNodes.Create (20);
  NetDeviceContainer ueLteDevs;
  Ipv4InterfaceContainer ueIpIfaces;

//create 18 eNodeBs
  NodeContainer enbNodes;
  enbNodes.Create(18);
  NetDeviceContainer enbLteDevs;


/***********************************************************
 * Create Internet and IP addresses for non-LTE devices    *
 ***********************************************************/
  InternetStackHelper internet;
  internet.Install(ueNodes);
//  internet.Install(enbNodes);
  Ipv4AddressHelper ipAddresses;
  ipAddresses.SetBase ("1.0.0.0", "255.255.255.0");

  // Create a single RemoteHost for the UDP application
  NodeContainer remoteHostContainer;
  remoteHostContainer.Create (1);
  Ptr<Node> remoteHost = remoteHostContainer.Get (0);
  internet.Install (remoteHostContainer);
  // Create the PointToPoint link between the remoteHost and the EPC
  PointToPointHelper p2ph;
  p2ph.SetDeviceAttribute ("DataRate", DataRateValue (DataRate ("100Gb/s")));
  p2ph.SetDeviceAttribute ("Mtu", UintegerValue (1500));
  p2ph.SetChannelAttribute ("Delay", TimeValue (Seconds (0.010)));
  NetDeviceContainer internetDevices = p2ph.Install (pgw, remoteHost);

  Ipv4InterfaceContainer internetIpIfaces = ipAddresses.Assign (internetDevices);

  // Routing of the Internet Host (towards the LTE network)
  Ipv4StaticRoutingHelper ipv4RoutingHelper;
  Ptr<Ipv4StaticRouting> remoteHostStaticRouting = ipv4RoutingHelper.GetStaticRouting (remoteHost->GetObject<Ipv4> ());
  // interface 0 is localhost, 1 is the p2p device
  remoteHostStaticRouting->AddNetworkRouteTo (Ipv4Address ("7.0.0.0"), Ipv4Mask ("255.0.0.0"), 1);


/***********************************************************
 * Add UEs and eNBs to the LTE network                     *
 * Requires setting up mobility models for UEs and eNBs    *
 ***********************************************************/
  // Install Mobility Model in eNB
  Ptr<ListPositionAllocator> enbPositionAlloc = CreateObject<ListPositionAllocator> ();
  for (uint16_t i = 0; i < 18; i++)
    {
      Vector enbPosition (i*10, i*10, 0); // TODO/XXX: must fix this
      enbPositionAlloc->Add (enbPosition);
    }
  MobilityHelper enbMobility;
  enbMobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  enbMobility.SetPositionAllocator (enbPositionAlloc);
  enbMobility.Install (enbNodes);

/***********************************************************
 * Attach RandomWalk Mobility to UEs                       *
 ***********************************************************/
  MobilityHelper ueMobility;
  ueMobility.SetPositionAllocator ("ns3::GridPositionAllocator",
    "MinX", DoubleValue (0.0),
    "MinY", DoubleValue (0.0),
    "DeltaX", DoubleValue (5.0),
    "DeltaY", DoubleValue (10.0),
    "GridWidth", UintegerValue (3),
    "LayoutType", StringValue ("RowFirst"));
  ueMobility.SetMobilityModel ("ns3::RandomWalk2dMobilityModel",
    "Time", TimeValue (Seconds (1.0)),
    "Mode", EnumValue (RandomWalk2dMobilityModel::MODE_TIME),
    "Bounds", RectangleValue (Rectangle (-100, 100, -100, 100)));
  ueMobility.Install (ueNodes);

  Config::SetDefault ("ns3::LteEnbPhy::TxPower", DoubleValue (enbTxPowerDbm));
  enbLteDevs = lteHelper->InstallEnbDevice (enbNodes);
  ueLteDevs = lteHelper->InstallUeDevice (ueNodes);


/***********************************************************
 * Add X2 interfaces to eNBs                               *
 ***********************************************************/
//Based on the topology in the paper, we connect each enodebs in each network
  lteHelper->AddX2Interface(enbNodes.Get(0),  enbNodes.Get(1));
  lteHelper->AddX2Interface(enbNodes.Get(1),  enbNodes.Get(2));

  lteHelper->AddX2Interface(enbNodes.Get(3),  enbNodes.Get(4));
  lteHelper->AddX2Interface(enbNodes.Get(4),  enbNodes.Get(5));

  lteHelper->AddX2Interface(enbNodes.Get(6),  enbNodes.Get(7));
  lteHelper->AddX2Interface(enbNodes.Get(7),  enbNodes.Get(8));

  lteHelper->AddX2Interface(enbNodes.Get(9),  enbNodes.Get(10));
  lteHelper->AddX2Interface(enbNodes.Get(10), enbNodes.Get(11));

  lteHelper->AddX2Interface(enbNodes.Get(12), enbNodes.Get(13));
  lteHelper->AddX2Interface(enbNodes.Get(13), enbNodes.Get(14));

  lteHelper->AddX2Interface(enbNodes.Get(15), enbNodes.Get(16));
  lteHelper->AddX2Interface(enbNodes.Get(16), enbNodes.Get(17));

  lteHelper->SetEnbAntennaModelType ("ns3::IsotropicAntennaModel");
//  lte.SetFadingModel("");
//  lte.SetHandoverAlgorithmType(""); //TODO/XXX


/***********************************************************
 * Assign IP addresses to UEs and attach them to the LTE   *
 * network                                                 *
 ***********************************************************/
//assign UE IP addresses after the lteHelper is aware of the UE's InternetStack
  ueIpIfaces = epcHelper->AssignUeIpv4Address (ueLteDevs);
  //lteHelper->Attach(ueLteDevs); // TODO/XXX: see issue #2
  lteHelper->AttachToClosestEnb(ueLteDevs, enbLteDevs); // TODO/XXX: see issue #2


/***********************************************************
 * Set up UDP application                                  *
 ***********************************************************/
  Ipv4Address remoteHostAddr = internetIpIfaces.GetAddress (1);
  uint16_t dlPort = 10000;
  uint16_t ulPort = 20000;

  // randomize a bit start times to avoid simulation artifacts
  // (e.g., buffer overflows due to packet transmissions happening
  // exactly at the same time)
  Ptr<UniformRandomVariable> startTimeSeconds = CreateObject<UniformRandomVariable> ();
  startTimeSeconds->SetAttribute ("Min", DoubleValue (0));
  startTimeSeconds->SetAttribute ("Max", DoubleValue (0.010));

  for (uint32_t u = 0; u < 20; ++u)
    {
      Ptr<Node> ue = ueNodes.Get (u);
      // Set the default gateway for the UE
      Ptr<Ipv4StaticRouting> ueStaticRouting = ipv4RoutingHelper.GetStaticRouting (ue->GetObject<Ipv4> ());
      ueStaticRouting->SetDefaultRoute (epcHelper->GetUeDefaultGatewayAddress (), 1);

      ++dlPort;
      ++ulPort;

      ApplicationContainer clientApps;
      ApplicationContainer serverApps;

      NS_LOG_LOGIC ("installing UDP DL app for UE " << u);
      UdpClientHelper dlClientHelper (ueIpIfaces.GetAddress (u), dlPort);
      clientApps.Add (dlClientHelper.Install (remoteHost));
      PacketSinkHelper dlPacketSinkHelper ("ns3::UdpSocketFactory",
                                            InetSocketAddress (Ipv4Address::GetAny (), dlPort));
      serverApps.Add (dlPacketSinkHelper.Install (ue));

      NS_LOG_LOGIC ("installing UDP UL app for UE " << u);
      UdpClientHelper ulClientHelper (remoteHostAddr, ulPort);
      clientApps.Add (ulClientHelper.Install (ue));
      PacketSinkHelper ulPacketSinkHelper ("ns3::UdpSocketFactory",
                                            InetSocketAddress (Ipv4Address::GetAny (), ulPort));
      serverApps.Add (ulPacketSinkHelper.Install (remoteHost));

      Ptr<EpcTft> tft = Create<EpcTft> ();
      EpcTft::PacketFilter dlpf;
      dlpf.localPortStart = dlPort;
      dlpf.localPortEnd = dlPort;
      tft->Add (dlpf);
      EpcTft::PacketFilter ulpf;
      ulpf.remotePortStart = ulPort;
      ulpf.remotePortEnd = ulPort;
      tft->Add (ulpf);
      EpsBearer bearer (EpsBearer::NGBR_VIDEO_TCP_DEFAULT);
      lteHelper->ActivateDedicatedEpsBearer (ueLteDevs.Get (u), bearer, tft);

      Time startTime = Seconds (startTimeSeconds->GetValue ());
      serverApps.Start (startTime);
      clientApps.Start (startTime);
    }

/***********************************************************
 * Run simulation                                          *
 ***********************************************************/
  lteHelper->EnablePhyTraces ();
  lteHelper->EnableMacTraces ();
  lteHelper->EnableRlcTraces ();
  lteHelper->EnablePdcpTraces ();
  Ptr<RadioBearerStatsCalculator> rlcStats = lteHelper->GetRlcStats ();
  rlcStats->SetAttribute ("EpochDuration", TimeValue (Seconds (1.0)));
  Ptr<RadioBearerStatsCalculator> pdcpStats = lteHelper->GetPdcpStats ();
  pdcpStats->SetAttribute ("EpochDuration", TimeValue (Seconds (1.0)));


  // Output every time position changes
  Config::Connect ("/NodeList/*/$ns3::MobilityModel/CourseChange",
                  MakeCallback (&CourseChange));


//An animation for the project
AnimationInterface anim (animFile);

  for (uint32_t i = 0; i < ueNodes.GetN (); ++i)
    {
      anim.UpdateNodeDescription (ueNodes.Get (i), "UE");
      anim.UpdateNodeColor (ueNodes.Get (i), 255, 0, 0);
   }

  for (uint32_t i = 0; i < enbNodes.GetN (); ++i)
    {
      anim.UpdateNodeDescription (enbNodes.Get (i), "ENB"); 
      anim.UpdateNodeColor (enbNodes.Get (i), 0, 255, 0); 
    }


  anim.EnablePacketMetadata ();
  anim.SetMobilityPollInterval (Seconds (0.0001));
 // anim.EnableIpv4RouteTracking (animFile, Seconds (0), Seconds (5), Seconds (0.25)); //Optional
  //anim.EnableWifiMacCounters (Seconds (0), Seconds (10));
  //anim.EnableWifiPhyCounters (Seconds (0), Seconds (10));

  anim.EnableIpv4L3ProtocolCounters (Seconds (0), Seconds (10));


  Simulator::Stop (Seconds (simTime));
  Simulator::Run ();
  std::cout << "Animation Trace file created:" << animFile.c_str ()<< std::endl;
  Simulator::Destroy ();
  return 0;
}
