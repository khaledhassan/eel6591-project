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
 */

#ifndef URBAN_MACRO_CELL_PROPAGATION_LOSS_MODEL_H
#define URBAN_MACRO_CELL_PROPAGATION_LOSS_MODEL_H

#include <ns3/propagation-loss-model.h>

namespace ns3 {


/**
 * \ingroup propagation
 *
 * See http://www.etsi.org/deliver/etsi_TR/136900_136999/136931/09.00.00_60/tr_136931v090000p.pdf
 * PDF page 11 (says 10 on the page)
 */
class UrbanMacroCellPropagationLossModel : public PropagationLossModel
{

public:
  /**
   * \brief Get the type ID.
   * \return the object TypeId
   */
  static TypeId GetTypeId (void);

  UrbanMacroCellPropagationLossModel ();
  virtual ~UrbanMacroCellPropagationLossModel ();

  /** 
   * \param a the first mobility model
   * \param b the second mobility model
   * 
   * \return the loss in dBm for the propagation between
   * the two given mobility models
   */
  double GetLoss (Ptr<MobilityModel> a, Ptr<MobilityModel> b) const;

private:
  /**
   * \brief Copy constructor
   *
   * Defined and unimplemented to avoid misuse
   */
  UrbanMacroCellPropagationLossModel (const UrbanMacroCellPropagationLossModel &);
  /**
   * \brief Copy constructor
   *
   * Defined and unimplemented to avoid misuse
   * \returns
   */
  UrbanMacroCellPropagationLossModel & operator = (const UrbanMacroCellPropagationLossModel &);

  // inherited from PropagationLossModel
  virtual double DoCalcRxPower (double txPowerDbm,
                                Ptr<MobilityModel> a,
                                Ptr<MobilityModel> b) const;
  virtual int64_t DoAssignStreams (int64_t stream);
  
};

} // namespace ns3


#endif // KUN_2600MHZ_PROPAGATION_LOSS_MODEL_H

