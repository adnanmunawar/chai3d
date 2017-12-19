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
#include "CBulletGripper.h"
#include <string.h>

// root resource path
std::string resourceRootGripper;
// convert to resource path
#define RESOURCE_PATH(p)    (char*)((resourceRootGripper+std::string(p)).c_str())

namespace chai3d {

cBulletGripper::cBulletGripper(cBulletWorld *bulletWorld):cBulletMultiMesh(bulletWorld, "Gripper"){


    loadFromFile(RESOURCE_PATH("../resources/models/gripper/gripper_L1.3ds"));
    scale(0.1);
    setLocalPos(0.0,-0.2,0.0);

    bulletMeshGripperL2 = new cBulletMultiMesh(bulletWorld);
    bulletMeshGripperL2->loadFromFile(RESOURCE_PATH("../resources/models/gripper/gripper_L1.3ds"));
    bulletMeshGripperL2->scale(0.1);
    bulletMeshGripperL2->setLocalPos(0.0,0.2,0.0);
    cMatrix3d rotMat;
    rotMat.setAxisAngleRotationDeg(1,0,0,180);
    bulletMeshGripperL2->setLocalRot(rotMat);
}

void cBulletGripper::build(){
    setMass(0.05);
    buildContactTriangles(0.001);
    setShowFrame(true);
    estimateInertia();
    buildDynamicModel();
    m_dynamicWorld->addChild(this);


    bulletMeshGripperL2->setMass(0.05);
    bulletMeshGripperL2->buildContactTriangles(0.001);
    bulletMeshGripperL2->setShowFrame(true);
    bulletMeshGripperL2->estimateInertia();
    bulletMeshGripperL2->buildDynamicModel();
    m_dynamicWorld->addChild(bulletMeshGripperL2);

    axisA.setValue(0.0,0.0,1.0);
    axisB = -axisA;
    pvtA.setValue(0.2,0.1,0.0);
    pvtB.setValue(0.2,0.1,0.0);
    bulletHinge = new btHingeConstraint(*this->m_bulletRigidBody,
                                        *bulletMeshGripperL2->m_bulletRigidBody,
                                        pvtA, pvtB, axisA, axisB, true);

    m_dynamicWorld->m_bulletWorld->addConstraint(bulletHinge, true);
    bulletHinge->enableMotor(true);
    bulletHinge->setMaxMotorImpulse(0.3);
    bulletHinge->setLimit(2.2, 3.139);

    GripperSurfaceProperties props;
    props.set_default();
    props.lin_damping = 0.5;
    set_surface_props(props);

    mat.setBlueLightSteel();
    setMaterial(mat);
    mat.setBlueMediumSlate();
    bulletMeshGripperL2->setMaterial(mat);
}

void cBulletGripper::set_gripper_angle(const double &angle){
    bulletHinge->setMotorTarget(angle, 0.001);
}

void cBulletGripper::set_scale(double a_scale){
    // Do nothing for now
}

void cBulletGripper::set_surface_props(GripperSurfaceProperties &props){

    this->m_bulletRigidBody->setDamping(props.lin_damping, props.ang_damping);
    this->m_bulletRigidBody->setFriction(props.friction);
    this->m_bulletRigidBody->setRollingFriction(props.rolling_friction);

    bulletMeshGripperL2->m_bulletRigidBody->setDamping(props.lin_damping, props.ang_damping);
    bulletMeshGripperL2->m_bulletRigidBody->setFriction(props.friction);
    bulletMeshGripperL2->m_bulletRigidBody->setRollingFriction(props.rolling_friction);

}

}
