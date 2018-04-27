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
  /*
  LogLevel logLevel = (LogLevel)(LOG_PREFIX_ALL | LOG_LEVEL_INFO);

  LogComponentEnable ("LteHelper", logLevel);
  LogComponentEnable ("EpcHelper", logLevel);
  LogComponentEnable ("EmuEpcHelper", logLevel);
  LogComponentEnable ("EpcEnbApplication", logLevel);
  LogComponentEnable ("EpcX2", logLevel);
  LogComponentEnable ("EpcSgwPgwApplication", logLevel);

  LogComponentEnable ("LteEnbRrc", logLevel);
  LogComponentEnable ("LteEnbNetDevice", logLevel);
  LogComponentEnable ("LteUeRrc", logLevel);
  LogComponentEnable ("LteUeNetDevice", logLevel);
  LogComponentEnable ("MobilityHelper", logLevel);

  LogComponentEnable ("UdpClient", logLevel);
  LogComponentEnable ("UdpTraceClient", logLevel);
  LogComponentEnable ("UdpServer", logLevel);
  */
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
  uint32_t numUEs = 2;
  NodeContainer ueNodes;
  ueNodes.Create (numUEs);
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
  uint16_t port = 10000;

  // randomize a bit start times to avoid simulation artifacts
  // (e.g., buffer overflows due to packet transmissions happening
  // exactly at the same time)
  Ptr<UniformRandomVariable> startTimeSeconds = CreateObject<UniformRandomVariable> ();
  startTimeSeconds->SetAttribute ("Min", DoubleValue (0.5));
  startTimeSeconds->SetAttribute ("Max", DoubleValue (0.600));

  ApplicationContainer clientApps; // for UEs
  ApplicationContainer serverApps; // on remoteHost

  std::vector<UdpServerHelper> servers;

  for (uint32_t u = 0; u < numUEs; ++u)
    {
      Ptr<Node> ue = ueNodes.Get (u);
      // Set the default gateway for the UE
      Ptr<Ipv4StaticRouting> ueStaticRouting = ipv4RoutingHelper.GetStaticRouting (ue->GetObject<Ipv4> ());
      ueStaticRouting->SetDefaultRoute (epcHelper->GetUeDefaultGatewayAddress (), 1);

      ++port;

      Time startTime = Seconds (startTimeSeconds->GetValue ());

      NS_LOG_LOGIC ("installing UDP Client+Server for UE " << u);

      // set up new server just for this client and attach it to the remoteHost
      UdpServerHelper server (port);
      ApplicationContainer thisServer = server.Install (remoteHost);
      thisServer.Start(startTime);
      serverApps.Add(thisServer);
      servers.push_back(server);

      // set up a client for this UE to connect to it's server
      UdpTraceClientHelper client(remoteHostAddr, port, "");
      client.SetAttribute ("MaxPacketSize", UintegerValue (1024)); // XXX/TODO: make this a parameter?
      ApplicationContainer thisClient = client.Install(ue);
      thisClient.Start(startTime);
      clientApps.Add(thisClient);

      Ptr<EpcTft> tft = Create<EpcTft> ();
      EpcTft::PacketFilter dlpf;
      dlpf.localPortStart = port;
      dlpf.localPortEnd = port;
      tft->Add (dlpf);
      EpcTft::PacketFilter ulpf;
      ulpf.remotePortStart = port;
      ulpf.remotePortEnd = port;
      tft->Add (ulpf);
      EpsBearer bearer (EpsBearer::NGBR_VIDEO_TCP_DEFAULT);
      lteHelper->ActivateDedicatedEpsBearer (ueLteDevs.Get (u), bearer, tft);
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

  Simulator::Stop (Seconds (simTime));
  Simulator::Run ();
  Simulator::Destroy ();

  //TODO/XXX, read server statistics like so, but iterate through each object in serverApps:
  /*
    NS_TEST_ASSERT_MSG_EQ (server.GetServer ()->GetLost (), 0, "Packets were lost !");
    NS_TEST_ASSERT_MSG_EQ (server.GetServer ()->GetReceived (), 247, "Did not receive expected number of packets !");
  */
 std::vector<UdpServerHelper>::iterator i;
 uint32_t u;
 for (u = 1, i = servers.begin(); i != servers.end(); i++, u++)
    {
      std::cout << "Server for UE " << u << " lost " << i->GetServer()->GetLost() << std::endl;
      std::cout << "Server for UE " << u << " recieved " << i->GetServer()->GetReceived() << std::endl;
    }

  return 0;
}
