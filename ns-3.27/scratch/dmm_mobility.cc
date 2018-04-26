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

int
main (int argc, char *argv[])
{

/***********************************************************
 * Log level and coommand line parsing                     *
 ***********************************************************/
  LogLevel logLevel = (LogLevel)(LOG_PREFIX_ALL | LOG_LEVEL_INFO);

  LogComponentEnable ("LteHelper", logLevel);
  LogComponentEnable ("EpcHelper", logLevel);
  LogComponentEnable ("EpcEnbApplication", logLevel);
  LogComponentEnable ("EpcX2", logLevel);
  LogComponentEnable ("EpcSgwPgwApplication", logLevel);

  LogComponentEnable ("LteEnbRrc", logLevel);
  LogComponentEnable ("LteEnbNetDevice", logLevel);
  LogComponentEnable ("LteUeRrc", logLevel);
  LogComponentEnable ("LteUeNetDevice", logLevel);

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
  double simTime = 15.0; //TODO/XXX old value: (double)(numberOfEnbs + 1) * distance / speed; // 1500 m / 20 m/s = 75 secs

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


/***********************************************************
 * Add UEs and eNBs to the LTE network                     *
 * Requires setting up mobility models for UEs and eNBs    *
 ***********************************************************/
  // Install Mobility Model in eNB
  Ptr<ListPositionAllocator> enbPositionAlloc = CreateObject<ListPositionAllocator> ();
  for (uint16_t i = 0; i < 18; i++)
    {
      Vector enbPosition (0, 0, 0); // TODO/XXX: must fix this
      enbPositionAlloc->Add (enbPosition);
    }
  MobilityHelper enbMobility;
  enbMobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  enbMobility.SetPositionAllocator (enbPositionAlloc);
  enbMobility.Install (enbNodes);

  // Install Mobility Model in UEs XXX/TODO: change this to real mobility model!
  Ptr<ListPositionAllocator> uePositionAlloc = CreateObject<ListPositionAllocator> ();
  for (uint16_t i = 0; i < 20; i++)
    {
      Vector uePosition (0, 0, 0); // TODO/XXX: must fix this
      uePositionAlloc->Add (uePosition);
    }
  MobilityHelper ueMobility;
  ueMobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  ueMobility.SetPositionAllocator (uePositionAlloc);
  ueMobility.Install (ueNodes);

  Config::SetDefault ("ns3::LteEnbPhy::TxPower", DoubleValue (enbTxPowerDbm));
  enbLteDevs = lteHelper->InstallEnbDevice (enbNodes);
  ueLteDevs = lteHelper->InstallUeDevice (ueNodes);
//  EpsBearer bearer;
//  lteHelper->ActivateDataRadioBearer(ueLteDevs,bearer);


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

  lteHelper->AddX2Interface(enbNodes.Get(9), enbNodes.Get(10));
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
  lteHelper->Attach(ueLteDevs); // TODO/XXX: see issue #2


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

  Simulator::Stop (Seconds (60));
  Simulator::Run ();
  Simulator::Destroy ();
  return 0;
}
