/* =========================================================================
 * This file is part of cphd-c++
 * =========================================================================
 *
 * (C) Copyright 2004 - 2018, MDA Information Systems LLC
 *
 * cphd-c++ is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this program; If not,
 * see <http://www.gnu.org/licenses/>.
 *
 */


#ifndef __CPHD_METADATA_H__
#define __CPHD_METADATA_H__

#include <ostream>

#include <cphd/CollectionID.h>
#include <cphd/Channel.h>
#include <cphd/Data.h>
#include <cphd/Global.h>
#include <cphd/SceneCoordinates.h>
#include <cphd/PVP.h>
#include <cphd/Dwell.h>
#include <cphd/ReferenceGeometry.h>

namespace cphd
{
/*!
 *  \class Metadata
 *
 *  This class extends the Data object providing the CPHD
 *  layer and utilities for access.  In order to write a CPHD,
 *  you must have a CPHDData object that is populated with
 *  the mandatory parameters, and you must add it to the Container
 *  object first.
 *
 *  This object contains all of the sub-blocks for CPHD.
 *
 *
 */
struct Metadata
{
    Metadata()
    {
    }

    //!  CollectionInfo block.  Contains the general collection information
    //!  CPHD can use the SICD Collection Information block directly
    CollectionID collectionID;

    //!  Global block. Contains the information
    Global global;

    //! Scene Coordinates block. Parameters that define geographic
    //! coordinates for in the imaged scene
    SceneCoordinates sceneCoordinates;

    //!  Data block. Very unfortunate name, but matches the CPHD spec.
    //!  Contains parameters that describe binary data components contained
    //!  in the product
    Data data;

    //! Channel block. Parameters that describe the data channels contained
    //! in the product
    Channel channel;

    //! PVP block. Parameters that describe the size of position of each
    //! vector paramter
    Pvp pvp;

    //Dwell block.
    Dwell dwell;

    //Reference Geometry block.
    ReferenceGeometry referenceGeometry;

    //!  SRP block.  Contains the  information
    //SRP srp;

    //!  Optional Antenna block.  Contains the  information
    //!  This is similar to, but not identical to, the SICD Antenna
    //mem::ScopedCopyablePtr<Antenna> antenna;

    //!  VectorParameters block.  Contains the  information
    //VectorParameters vectorParameters;

    bool operator==(const Metadata& other) const;

    bool operator!=(const Metadata& other) const
    {
        return !((*this) == other);
    }
};

std::ostream& operator<< (std::ostream& os, const Metadata& d);
}

#endif
