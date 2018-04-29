/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2011, 2012 Centre Tecnologic de Telecomunicacions de Catalunya (CTTC)
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
 *
 * Author: Marco Miozzo  <marco.miozzo@cttc.es>,
 *         Nicola Baldo <nbaldo@cttc.es>
 * 
 */
#include "ns3/log.h"
#include "ns3/double.h"
#include "ns3/enum.h"
#include "ns3/mobility-model.h"
#include <cmath>

#include "urban-macro-cell-propagation-loss-model.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("UrbanMacroCellPropagationLossModel");

NS_OBJECT_ENSURE_REGISTERED (UrbanMacroCellPropagationLossModel);


TypeId
UrbanMacroCellPropagationLossModel::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::UrbanMacroCellPropagationLossModel")
    .SetParent<PropagationLossModel> ()
    .SetGroupName ("Propagation")
    .AddConstructor<UrbanMacroCellPropagationLossModel> ()
    ;

  return tid;
}

UrbanMacroCellPropagationLossModel::UrbanMacroCellPropagationLossModel ()
  : PropagationLossModel ()
{
}

UrbanMacroCellPropagationLossModel::~UrbanMacroCellPropagationLossModel ()
{
}

double
UrbanMacroCellPropagationLossModel::GetLoss (Ptr<MobilityModel> a, Ptr<MobilityModel> b) const
{
  double dist = a->GetDistanceFrom (b);  
  double loss = 128.1 + 37.6 * std::log10 (dist/1000);
  return loss;
}

double 
UrbanMacroCellPropagationLossModel::DoCalcRxPower (double txPowerDbm,
					       Ptr<MobilityModel> a,
					       Ptr<MobilityModel> b) const
{
  return (txPowerDbm - GetLoss (a, b));
}

int64_t
UrbanMacroCellPropagationLossModel::DoAssignStreams (int64_t stream)
{
  return 0;
}


} // namespace ns3
