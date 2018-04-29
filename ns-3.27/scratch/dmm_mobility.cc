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
#include "ns3/nstime.h"


using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("DMM_MOBILITY");

std::vector<Time> m_ueHandoverStart;
std::vector<Time> m_enbHandoverStart;

void
NotifyConnectionEstablishedUe (std::string context,
                               uint64_t imsi,
                               uint16_t cellid,
                               uint16_t rnti)
{
  std::cout << int(Simulator::Now().GetSeconds())
            << "s "
            << " UE IMSI " << imsi
            << ": connected to CellId " << cellid
            << " with RNTI " << rnti
            << std::endl;
}

void
NotifyHandoverStartUe (std::string context,
                       uint64_t imsi,
                       uint16_t cellid,
                       uint16_t rnti,
                       uint16_t targetCellId)
{
  std::cout << int(Simulator::Now().GetSeconds())
            << "s "
            << "s UE IMSI " << imsi
            << ": previously connected to CellId " << cellid
            << " with RNTI " << rnti
            << ", doing handover to CellId " << targetCellId
            << std::endl;
  m_ueHandoverStart[imsi-1] = Simulator::Now ();
}

void
NotifyHandoverEndOkUe (std::string context,
                       uint64_t imsi,
                       uint16_t cellid,
                       uint16_t rnti)
{
  std::cout << int(Simulator::Now().GetSeconds())
            << "s "
            << " UE IMSI " << imsi
            << ": successful handover to CellId " << cellid
            << " with RNTI " << rnti
            << std::endl;
  Time delay = Simulator::Now () - m_ueHandoverStart[imsi-1];
  std::cout << "UE"
            << imsi << " Delay: " << delay.GetSeconds()*1000 << "ms" << std::endl;
}

void
NotifyConnectionEstablishedEnb (std::string context,
                                uint64_t imsi,
                                uint16_t cellid,
                                uint16_t rnti)
{
  std::cout << int(Simulator::Now().GetSeconds())
            << "s "
            << " eNB CellId " << cellid
            << ": successful connection of UE with IMSI " << imsi
            << " RNTI " << rnti
            << std::endl;
}

void
NotifyHandoverStartEnb (std::string context,
                        uint64_t imsi,
                        uint16_t cellid,
                        uint16_t rnti,
                        uint16_t targetCellId)
{
  std::cout << int(Simulator::Now().GetSeconds())
            << "s "
            << " eNB CellId " << cellid
            << ": start handover of UE with IMSI " << imsi
            << " RNTI " << rnti
            << " to CellId " << targetCellId
            << std::endl;
  m_enbHandoverStart[imsi-1] = Simulator::Now ();
}

void
NotifyHandoverEndOkEnb (std::string context,
                        uint64_t imsi,
                        uint16_t cellid,
                        uint16_t rnti)
{
  std::cout << int(Simulator::Now().GetSeconds())
            << "s "
            << " eNB CellId " << cellid
            << ": completed handover of UE with IMSI " << imsi
            << " RNTI " << rnti
            << std::endl;
  Time delay = Simulator::Now () - m_enbHandoverStart[imsi-1];
  std::cout << "ENB"
            << cellid
            << " with UE" << imsi << "Delay: "<< delay.GetSeconds()*1000 << "ms" << std::endl;
}

static void
CourseChange (std::string foo, Ptr<const MobilityModel> mobility)
{
  Vector pos = mobility->GetPosition ();
  Vector vel = mobility->GetVelocity ();
  std::cout << float(Simulator::Now().GetMilliSeconds ())/1000 << "s, model=" << mobility << ", POS: x=" << pos.x << ", y=" << pos.y
            << ", z=" << pos.z << "; VEL: x=" << vel.x << ", y=" << vel.y
            << ", z=" << vel.z << std::endl;
}

int
main (int argc, char *argv[])
{


/***********************************************************
 * Log level and coommand line parsing                     *
 ***********************************************************/
  LogLevel logLevelLTE = (LogLevel)(LOG_PREFIX_ALL | LOG_LEVEL_INFO);
  LogLevel logLevelMobility = (LogLevel)(LOG_PREFIX_ALL | LOG_LEVEL_INFO);
  LogLevel logLevelUDP = (LogLevel)(LOG_PREFIX_ALL | LOG_LEVEL_INFO);

  LogComponentEnable ("LteHelper", logLevelLTE);
  LogComponentEnable ("EpcHelper", logLevelLTE);
  LogComponentEnable ("EmuEpcHelper", logLevelLTE);
  LogComponentEnable ("EpcEnbApplication", logLevelLTE);
  LogComponentEnable ("EpcSgwPgwApplication", logLevelLTE);

  LogComponentEnable ("LteEnbRrc", logLevelLTE);
  LogComponentEnable ("LteEnbNetDevice", logLevelLTE);
  LogComponentEnable ("LteUeRrc", logLevelLTE);
  LogComponentEnable ("LteUeNetDevice", logLevelLTE);

  LogComponentEnable ("EpcX2", logLevelMobility);
  LogComponentEnable ("MobilityHelper", logLevelMobility);
  LogComponentEnable ("A3RsrpHandoverAlgorithm", logLevelMobility);

  LogComponentEnable ("UdpClient", logLevelUDP);
  LogComponentEnable ("UdpTraceClient", logLevelUDP);
  LogComponentEnable ("UdpServer", logLevelUDP);
  LogComponentEnable ("UdpEchoClientApplication", logLevelUDP);
  LogComponentEnable ("UdpEchoServerApplication", logLevelUDP);

  Time::SetResolution (Time::NS);

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
  double simTime = 100; //TODO/XXX old value: (double)(numberOfEnbs + 1) * distance / speed; // 1500 m / 20 m/s = 75 secs
  double cellSize = 500; // m
  uint32_t numUEs = 2;

  cmd.AddValue ("speed", "Speed of the UE (default = 20 m/s)", speed);
  cmd.AddValue ("enbTxPowerDbm", "TX power [dBm] used by HeNBs (default = 25.0)", enbTxPowerDbm);
  cmd.AddValue ("simTime", "Total duration of the simulation (in seconds, default = 100)", simTime);
  cmd.AddValue ("cellSize", "Cell grid spacing in X and Y (in meters, default = 500)", cellSize);
  cmd.AddValue ("numUEs", "Number of UEs (default = 2)", numUEs);

  cmd.Parse (argc, argv);

  m_ueHandoverStart.reserve(numUEs);
  m_enbHandoverStart.reserve(numUEs);

/***********************************************************
 * Create LTE, EPC, and UE/eNB Nodes                       *
 ***********************************************************/
  Ptr<LteHelper> lteHelper = CreateObject<LteHelper> ();
  Ptr<PointToPointEpcHelper> epcHelper = CreateObject<PointToPointEpcHelper> ();
  lteHelper->SetEpcHelper (epcHelper);
  lteHelper->SetSchedulerType ("ns3::RrFfMacScheduler");

   lteHelper->SetHandoverAlgorithmType ("ns3::A3RsrpHandoverAlgorithm");
   lteHelper->SetHandoverAlgorithmAttribute ("Hysteresis",
                                             DoubleValue (3.0));
   lteHelper->SetHandoverAlgorithmAttribute ("TimeToTrigger",
                                             TimeValue (MilliSeconds (300)));

  Ptr<Node> pgw = epcHelper->GetPgwNode (); // TODO/XXX: used later?


//creates 20 nodes we can use as mobile nodes
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
  MobilityHelper enbMobility;
  enbMobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  enbMobility.SetPositionAllocator ("ns3::GridPositionAllocator",
    "MinX", DoubleValue (0.0),
    "MinY", DoubleValue (0.0),
    "DeltaX", DoubleValue (cellSize),
    "DeltaY", DoubleValue (cellSize),
    "GridWidth", UintegerValue (3),
    "LayoutType", StringValue ("RowFirst"));
  enbMobility.Install (enbNodes);

  for (NodeContainer::Iterator j = enbNodes.Begin (); j != enbNodes.End (); ++j)
    {
      Ptr<Node> object = *j;
      Ptr<MobilityModel> position = object->GetObject<MobilityModel> ();
      NS_ASSERT (position != 0);
      Vector pos = position->GetPosition ();
      std::cout << "x=" << pos.x << ", y=" << pos.y << ", z=" << pos.z << std::endl;
    }

/***********************************************************
 * Attach RandomWalk Mobility to UEs                       *
 ***********************************************************/
  MobilityHelper ueMobility;
  ueMobility.SetPositionAllocator ("ns3::GridPositionAllocator",
    "MinX", DoubleValue (0.0),
    "MinY", DoubleValue (0.0),
    "DeltaX", DoubleValue (cellSize*0.75),
    "DeltaY", DoubleValue (cellSize*0.75),
    "GridWidth", UintegerValue (3),
    "LayoutType", StringValue ("RowFirst"));
  // ueMobility.SetMobilityModel ("ns3::RandomWalk2dMobilityModel",
  //   "Time", TimeValue (Seconds (1.0)),
  //   "Mode", EnumValue (RandomWalk2dMobilityModel::MODE_TIME),
  //   "Bounds", RectangleValue (Rectangle (-100, 100, -100, 100)));
  ueMobility.SetMobilityModel ("ns3::RandomDirection2dMobilityModel",
                              "Bounds", RectangleValue (Rectangle (-cellSize, (18/3)*cellSize, -cellSize, (3)*cellSize)), // TODO/XXX: parameterize GridWidth of eNBs, which is the 3 in this line
                              "Speed", StringValue ("ns3::ConstantRandomVariable[Constant=100]"),
                              "Pause", StringValue ("ns3::ConstantRandomVariable[Constant=0]"));
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

  ApplicationContainer ueApps; // for UEs
  ApplicationContainer remoteHostApps; // on remoteHost

  std::vector<UdpServerHelper> remoteHostServers;
  std::vector<UdpServerHelper> ueServers;


  for (uint32_t u = 0; u < numUEs; ++u)
    {
      Ptr<Node> ue = ueNodes.Get (u);
      Ipv4Address ueIpAddress = ueIpIfaces.GetAddress(u);
      // Set the default gateway for the UE
      Ptr<Ipv4StaticRouting> ueStaticRouting = ipv4RoutingHelper.GetStaticRouting (ue->GetObject<Ipv4> ());
      ueStaticRouting->SetDefaultRoute (epcHelper->GetUeDefaultGatewayAddress (), 1);

      ++port;

      Time startTime = Seconds (startTimeSeconds->GetValue ());

      NS_LOG_LOGIC ("installing UDP Client for UE + Server for remoteHost" << u);
      // set up new rhServer just for this ueClient and attach it to the remoteHost
      UdpServerHelper rhServer (port);
      ApplicationContainer thisServer = rhServer.Install (remoteHost);
      thisServer.Start(startTime);
      remoteHostApps.Add(thisServer);
      remoteHostServers.push_back(rhServer);

      // set up a ueClient for this UE to connect to it's rhServer
      UdpTraceClientHelper ueClient(remoteHostAddr, port, "");
      ueClient.SetAttribute ("MaxPacketSize", UintegerValue (1024)); // XXX/TODO: make this a parameter?
      ApplicationContainer thisClient = ueClient.Install(ue);
      thisClient.Start(startTime);
      ueApps.Add(thisClient);

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

      ++port;
      NS_LOG_LOGIC ("installing UDP Client for remoteHost + Server for UE" << u);
      // set up new ueServer just for this ueClient and attach it to the remoteHost
      UdpServerHelper ueServer (port);
      ApplicationContainer thisServer2 = ueServer.Install (ue);
      thisServer2.Start(startTime);
      ueApps.Add(thisServer2);
      ueServers.push_back(ueServer);

      // set up a ueClient for this UE to connect to it's rhServer
      UdpTraceClientHelper rhClient(ueIpAddress, port, "");
      rhClient.SetAttribute ("MaxPacketSize", UintegerValue (1024)); // XXX/TODO: make this a parameter?
      ApplicationContainer thisClient2 = rhClient.Install(remoteHost);
      thisClient2.Start(startTime);
      ueApps.Add(thisClient2);

      Ptr<EpcTft> tft2 = Create<EpcTft> ();
      EpcTft::PacketFilter dlpf2;
      dlpf2.localPortStart = port;
      dlpf2.localPortEnd = port;
      tft2->Add (dlpf2);
      EpcTft::PacketFilter ulpf2;
      ulpf2.remotePortStart = port;
      ulpf2.remotePortEnd = port;
      tft2->Add (ulpf2);
      EpsBearer bearer2 (EpsBearer::NGBR_VIDEO_TCP_DEFAULT);
      lteHelper->ActivateDedicatedEpsBearer (ueLteDevs.Get (u), bearer2, tft2);
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

  // connect custom trace sinks for RRC connection establishment and handover notification
  Config::Connect ("/NodeList/*/DeviceList/*/LteEnbRrc/ConnectionEstablished",
                   MakeCallback (&NotifyConnectionEstablishedEnb));
  Config::Connect ("/NodeList/*/DeviceList/*/LteUeRrc/ConnectionEstablished",
                   MakeCallback (&NotifyConnectionEstablishedUe));
  Config::Connect ("/NodeList/*/DeviceList/*/LteEnbRrc/HandoverStart",
                   MakeCallback (&NotifyHandoverStartEnb));
  Config::Connect ("/NodeList/*/DeviceList/*/LteUeRrc/HandoverStart",
                   MakeCallback (&NotifyHandoverStartUe));
  Config::Connect ("/NodeList/*/DeviceList/*/LteEnbRrc/HandoverEndOk",
                   MakeCallback (&NotifyHandoverEndOkEnb));
  Config::Connect ("/NodeList/*/DeviceList/*/LteUeRrc/HandoverEndOk",
                   MakeCallback (&NotifyHandoverEndOkUe));


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
 for (u = 1, i = remoteHostServers.begin(); i != remoteHostServers.end(); i++, u++)
    {
      std::cout << "rhServer for UE " << u << " lost " << i->GetServer()->GetLost() << std::endl;
      std::cout << "rhServer for UE " << u << " recieved " << i->GetServer()->GetReceived() << std::endl;
    }

 for (u = 1, i = ueServers.begin(); i != ueServers.end(); i++, u++)
    {
      std::cout << "ueServer for UE " << u << " lost " << i->GetServer()->GetLost() << std::endl;
      std::cout << "ueServer for UE " << u << " recieved " << i->GetServer()->GetReceived() << std::endl;
    }

  return 0;
}
