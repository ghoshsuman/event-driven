/*
 * Copyright (C) 2015 iCub Facility - Istituto Italiano di Tecnologia
 * Author: arren.glover@iit.it
 * Permission is granted to copy, distribute, and/or modify this program
 * under the terms of the GNU General Public License, version 2 or any
 * later version published by the Free Software Foundation.
 *
 * A copy of the license can be found at
 * http://www.robotcub.org/icub/license/gpl.txt
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General
 * Public License for more details
 */

// \defgroup Modules Modules
// \defgroup vParticleFilter vParticleFilter
// \ingroup Modules
// \brief tracks targets using a particle filter

#ifndef __V_SKINMODULE__
#define __V_SKINMODULE__

#include <yarp/os/RFModule.h>
#include "vSkinInterface.h"


/*////////////////////////////////////////////////////////////////////////////*/
//VSKINMODULE
/*////////////////////////////////////////////////////////////////////////////*/
class module : public yarp::os::RFModule
{

    skinInterface skinterface;

public:

    //the virtual functions that need to be overloaded
    virtual bool configure(yarp::os::ResourceFinder &rf);
    virtual bool interruptModule();
    virtual bool close();
    virtual double getPeriod();
    virtual bool updateModule();

};


#endif
