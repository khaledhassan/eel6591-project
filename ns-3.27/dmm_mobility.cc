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
  CommandLine cmd;
  cmd.Parse (argc, argv);

  Time::SetResolution (Time::NS);
  LogComponentEnable ("UdpEchoClientApplication", LOG_LEVEL_INFO);
  LogComponentEnable ("UdpEchoServerApplication", LOG_LEVEL_INFO);


//creates 20 nodes we can use as mobile nodes
  NodeContainer nodes;
  nodes.Create (20);

//create 18 eNodeBs
  NodeContainer enbNode;
  enbNode.Create(18);

//point to point connection of UEDevices 
  PointToPointHelper pointToPoint;
  pointToPoint.SetDeviceAttribute ("DataRate", StringValue ("5Mbps"));
  pointToPoint.SetChannelAttribute ("Delay", StringValue ("2ms"));


//install lte and point to point capabilty on UEs
  NetDeviceContainer ueDevices;
  ueDevices = pointToPoint.Install (nodes);

  enbNodes = lteHelper->InstallEnbDevice (enbNode);
  ueDevices = lteHelper->InstallUeDevice (nodes);

  lteHelper->Attach(ueDevices);

  InternetStackHelper stack;
  stack.Install (nodes);
  stack.Install (enbNode);

  Ipv4AddressHelper IPaddress;
  IPaddress.SetBase ("10.10.10.0", "255.255.255.0");

  Ipv4InterfaceContainer interfaces = IPaddress.Assign (ueDevices);

  NetDeviceContainer enbNodes;


//creates the lte framework
  Ptr<LteHelper> lteHelper = CreateObject<LteHelper> ();
  Ptr<PointToPointEpcHelper> epcHelper = CreateObject<PointToPointEpcHelper> ();
  lteHelper->SetEpcHelper (epcHelper);
  lteHelper->SetSchedulerType ("ns3::RrFfMacScheduler"); // XXX/TODO: what is this?


  EpsBearer bearer;
  lteHelper->ActivateDataRadioBearer(ueDevices,bearer);


//Based on the topology in the paper, we connect each enodebs in each network
  lteHelper->AddX2Interface(enbNode.Get(1),  enbNode.Get(2));
  lteHelper->AddX2Interface(enbNode.Get(2),  enbNode.Get(3));

  lteHelper->AddX2Interface(enbNode.Get(4),  enbNode.Get(5));
  lteHelper->AddX2Interface(enbNode.Get(5),  enbNode.Get(6));

  lteHelper->AddX2Interface(enbNode.Get(7),  enbNode.Get(8));
  lteHelper->AddX2Interface(enbNode.Get(8),  enbNode.Get(9));

  lteHelper->AddX2Interface(enbNode.Get(10), enbNode.Get(11));
  lteHelper->AddX2Interface(enbNode.Get(11), enbNode.Get(12));

  lteHelper->AddX2Interface(enbNode.Get(13), enbNode.Get(14));
  lteHelper->AddX2Interface(enbNode.Get(14), enbNode.Get(15));

  lteHelper->AddX2Interface(enbNode.Get(16), enbNode.Get(17));
  lteHelper->AddX2Interface(enbNode.Get(17), enbNode.Get(18));

  lteHelper->SetEnbAntennaModelType ("IsotropicAntennaModel");
//  lte.SetFadingModel("");
//  lte.SetHandoverAlgorithmType("");


  Simulator::Run ();
  Simulator::Destroy ();
  return 0;
}
