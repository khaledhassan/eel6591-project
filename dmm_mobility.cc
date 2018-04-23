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
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include "ns3/eps-bearer.h"
#include "ns3/lte-helper.h"
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

  NetDeviceContainer ueDevices;

  NodeContainer enbNode;
  enbNode.create(18);

  NetDeviceContainer enbNodes;


//creates the lte framework
  LteHelper lte;
  lte.ActivateDataRadioBearer(ueDevices,EpsBearer bearer);


//Based on the topology in the paper, we connect each enodebs in each network
  lte.AddX2Interface(Ptr<Node> enbNode.Get(1), Ptr<Node> enbNode.Get(2));
  lte.AddX2Interface(Ptr<Node> enbNode.Get(2), Ptr<Node> enbNode.Get(3));

  lte.AddX2Interface(Ptr<Node> enbNode.Get(4), Ptr<Node> enbNode.Get(5));
  lte.AddX2Interface(Ptr<Node> enbNode.Get(5), Ptr<Node> enbNode.Get(6));

  lte.AddX2Interface(Ptr<Node> enbNode.Get(7), Ptr<Node> enbNode.Get(8));
  lte.AddX2Interface(Ptr<Node> enbNode.Get(8), Ptr<Node> enbNode.Get(9));

  lte.AddX2Interface(Ptr<Node> enbNode.Get(10), Ptr<Node> enbNode.Get(11));
  lte.AddX2Interface(Ptr<Node> enbNode.Get(11), Ptr<Node> enbNode.Get(12));

  lte.AddX2Interface(Ptr<Node> enbNode.Get(13), Ptr<Node> enbNode.Get(14));
  lte.AddX2Interface(Ptr<Node> enbNode.Get(14), Ptr<Node> enbNode.Get(15));

  lte.AddX2Interface(Ptr<Node> enbNode.Get(16), Ptr<Node> enbNode.Get(17));
  lte.AddX2Interface(Ptr<Node> enbNode.Get(17), Ptr<Node> enbNode.Get(18));

  lte.SetEnbAntennaModelType ("IsotropicAntennaModel");
  lte.SetFadingModel("TraceFadingLossModel");
  lte.SetHandoverAlgorithmType("A3RsrpHandoverAlgorithm");
  lte.SetSpectrumChannelType("SingleModelSpectrumChannel");

  enbNodes = lte.InstallEnbDevice (enbNode);
  ueDevices =lte.InstallUeDevice (Nodes);

  lte.Attach(ueDevices);
  lte.SetEpcHelper(Ptr<EpcHelper> h); 	

  InternetStackHelper stack;
  stack.Install (nodes);

  Simulator::Run ();
  Simulator::Destroy ();
  return 0;
}
