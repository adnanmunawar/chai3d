//==============================================================================
/*
    Software License Agreement (BSD License)
    Copyright (c) 2003-2016, CHAI3D
    (www.chai3d.org)

    All rights reserved.

    Redistribution and use in source and binary forms, with or without
    modification, are permitted provided that the following conditions
    are met:

    * Redistributions of source code must retain the above copyright
    notice, this list of conditions and the following disclaimer.

    * Redistributions in binary form must reproduce the above
    copyright notice, this list of conditions and the following
    disclaimer in the documentation and/or other materials provided
    with the distribution.

    * Neither the name of CHAI3D nor the names of its contributors may
    be used to endorse or promote products derived from this software
    without specific prior written permission.

    THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
    "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
    LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
    FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
    COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
    INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
    BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
    LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
    CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
    LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
    ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
    POSSIBILITY OF SUCH DAMAGE.

    \author    <http://www.aimlab.wpi.edu>
    \author    Adnan Munawar
    \version   3.2.0 $Rev: 2161 $
*/
//==============================================================================

#ifndef CBulletGripper_H
#define CBulletGripper_H

#include "CBulletGenericObject.h"
#include "CBulletMultiBody.h"
#include "chai3d.h"


namespace chai3d {

class afBulletGripper;
class afBulletGripperLink;

typedef std::shared_ptr<afBulletGripper> afBulletGripperPtr;
typedef std::map<std::string, afBulletGripperLink*> afGripperBodyMap;

///
/// \brief The cBulletGripperBody class
///
class afBulletGripperLink : public afRigidBody{
    friend class afBulletGripper;
public:

    afBulletGripperLink(cBulletWorld* a_chaiWorld): afRigidBody(a_chaiWorld){
        m_surfaceProps.m_linear_damping = 0.5;
        m_surfaceProps.m_angular_damping = 1.0;
        m_surfaceProps.m_static_friction = 0.5;
        m_surfaceProps.m_rolling_friction = 0.5;
    }
    ~afBulletGripperLink(){}

};

///
/// \brief The cBulletGripper class
///
class afBulletGripper: public afBulletMultiBody
{

    friend class afBulletGripperLink;

public:

    afBulletGripper(cBulletWorld *a_chaiWorld) : afBulletMultiBody(a_chaiWorld){}
    ~afBulletGripper();
    void set_gripper_angle(const double &angle,double dt=0.001);
    virtual afBulletGripperLink* load_multibody(std::string a_file,
                                               std::string a_gripper_name,
                                               std::string a_suffix_name);

private:

//    cGripperBodyMap m_gripperBodyMap;
    std::string m_gripper_name, m_suffix_name;
};

}

#endif
