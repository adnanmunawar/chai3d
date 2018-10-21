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
    \Motivation: https://www.gamedev.net/articles/programming/engines-and-middleware/yaml-basics-and-parsing-with-yaml-cpp-r3508/
    \version   3.2.1 $Rev: 2161 $
*/
//==============================================================================

#ifndef CBulletMultiBody_H
#define CBulletMultiBody_H

#include "CBullet.h"
#include <yaml-cpp/yaml.h>

namespace chai3d {

class cBulletMultiBody;
class cBulletWorld;
class Link;
class Joint;

typedef std::shared_ptr<cBulletMultiBody> cBulletMultiBodyPtr;
typedef std::map<std::string, Link*> cLinkMap;
typedef std::map<std::string, Joint*> cJointMap;

class Link : public cBulletMultiMesh{
private:
    double m_scale;
    std::string m_mesh_name;
    cMultiMesh m_lowResMesh;
    cVector3d pos;
    cMatrix3d rot;
    cMaterial m_mat;
public:
    Link(cBulletWorld* a_world):cBulletMultiMesh(a_world){}
    bool load (std::string file, std::string name, cBulletMultiBody* multiBody);
};

class Joint{
private:
    std::string m_name;
    std::string m_parent_name, m_child_name;
    std::string m_joint_name;
    btVector3 m_axisA, m_axisB;
    btVector3 m_pvtA, m_pvtB;
    bool enable_motor;
    double jnt_lim_low, jnt_lim_high, max_motor_impluse;
    btHingeConstraint* m_hinge;
    void assign_vec(std::string name, btVector3* v, YAML::Node* node);
    void print_vec(std::string name, btVector3* v);
public:
    bool load (std::string file, std::string name, cBulletMultiBody* multiBody);
};

class cBulletMultiBody{
    friend Link;
    friend Joint;
public:
    cBulletMultiBody(cBulletWorld *bulletWorld);
    ~cBulletMultiBody(){}
    bool load_yaml(std::string file);

protected:
    cBulletWorld *m_chaiWorld;
    cLinkMap m_linkMap;
    cJointMap m_jointMap;
    std::string high_res_path, low_res_path;
private:
    cMaterial mat;
};

}

#endif