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
  NodeContainer ueNodes;
  ueNodes.Create (20);
  NetDeviceContainer ueDevices;

//create 18 eNodeBs
  NodeContainer enbNodes;
  enbNodes.Create(18);
  NetDeviceContainer enbDevices;

//creates the lte framework
  Ptr<LteHelper> lteHelper = CreateObject<LteHelper> ();
  Ptr<PointToPointEpcHelper> epcHelper = CreateObject<PointToPointEpcHelper> ();
  lteHelper->SetEpcHelper (epcHelper);
  lteHelper->SetSchedulerType ("ns3::RrFfMacScheduler"); // XXX/TODO: what is this?


  EpsBearer bearer;
  lteHelper->ActivateDataRadioBearer(ueDevices,bearer);


//Based on the topology in the paper, we connect each enodebs in each network
  lteHelper->AddX2Interface(enbNodes.Get(1),  enbNodes.Get(2));
  lteHelper->AddX2Interface(enbNodes.Get(2),  enbNodes.Get(3));

  lteHelper->AddX2Interface(enbNodes.Get(4),  enbNodes.Get(5));
  lteHelper->AddX2Interface(enbNodes.Get(5),  enbNodes.Get(6));

  lteHelper->AddX2Interface(enbNodes.Get(7),  enbNodes.Get(8));
  lteHelper->AddX2Interface(enbNodes.Get(8),  enbNodes.Get(9));

  lteHelper->AddX2Interface(enbNodes.Get(10), enbNodes.Get(11));
  lteHelper->AddX2Interface(enbNodes.Get(11), enbNodes.Get(12));

  lteHelper->AddX2Interface(enbNodes.Get(13), enbNodes.Get(14));
  lteHelper->AddX2Interface(enbNodes.Get(14), enbNodes.Get(15));

  lteHelper->AddX2Interface(enbNodes.Get(16), enbNodes.Get(17));
  lteHelper->AddX2Interface(enbNodes.Get(17), enbNodes.Get(18));

  lteHelper->SetEnbAntennaModelType ("IsotropicAntennaModel");
//  lte.SetFadingModel("");
//  lte.SetHandoverAlgorithmType("");


  enbDevices = lteHelper->InstallEnbDevice (enbNodes);
  ueDevices = lteHelper->InstallUeDevice (ueNodes);

  lteHelper->Attach(ueDevices);

  InternetStackHelper stack;
  stack.Install (ueNodes);

  Simulator::Run ();
  Simulator::Destroy ();
  return 0;
}
