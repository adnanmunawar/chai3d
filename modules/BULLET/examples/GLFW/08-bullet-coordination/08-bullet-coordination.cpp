//===========================================================================
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
    \version   3.2.1 $Rev: 1869 $
*/
//===========================================================================

//---------------------------------------------------------------------------
#include "chai3d.h"
//---------------------------------------------------------------------------
#include <GLFW/glfw3.h>
//---------------------------------------------------------------------------
using namespace chai3d;
using namespace std;
//---------------------------------------------------------------------------
#include "CBullet.h"
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
// GENERAL SETTINGS
//---------------------------------------------------------------------------

// stereo Mode
/*
    C_STEREO_DISABLED:            Stereo is disabled
    C_STEREO_ACTIVE:              Active stereo for OpenGL NVDIA QUADRO cards
    C_STEREO_PASSIVE_LEFT_RIGHT:  Passive stereo where L/R images are rendered next to each other
    C_STEREO_PASSIVE_TOP_BOTTOM:  Passive stereo where L/R images are rendered above each other
*/
cStereoMode stereoMode = C_STEREO_DISABLED;

// fullscreen mode
bool fullscreen = false;

// mirrored display
bool mirroredDisplay = false;


//---------------------------------------------------------------------------
// BULLET MODULE VARIABLES
//---------------------------------------------------------------------------

// bullet world
cBulletWorld* bulletWorld;

// bullet objects
cBulletMesh* bulletMesh1;
cBulletMesh* bulletMesh2;
cBulletMesh* bulletCylinder;
cBulletMultiMesh* bulletGear;
cBulletMultiMesh* bulletTorus;
cBulletMultiMesh* bulletBase;

// bullet static walls and ground
cBulletStaticPlane* bulletGround;

cBulletStaticPlane* bulletBoxWall[5];

cVector3d camPos(0,0,0);
cVector3d dev_vel;
cMatrix3d cam_rot_last, dev_rot_last, dev_rot_cur;
// Default switch index for clutches


//---------------------------------------------------------------------------
// CHAI3D VARIABLES
//---------------------------------------------------------------------------


// a camera to render the world in the window display
cCamera* camera;

// a light source to illuminate the objects in the world
cSpotLight *light;

// a label to display the rates [Hz] at which the simulation is running
cLabel* labelRates;
cLabel* labelTimes;
cPrecisionClock clockWorld;


//---------------------------------------------------------------------------
// GENERAL VARIABLES
//---------------------------------------------------------------------------

// flag to indicate if the haptic simulation currently running
bool simulationRunning = false;

// flag to indicate if the haptic simulation has terminated
bool simulationFinished = true;

// a frequency counter to measure the simulation graphic rate
cFrequencyCounter freqCounterGraphics;

// a frequency counter to measure the simulation haptic rate
cFrequencyCounter freqCounterHaptics;

// haptic thread
cThread* hapticsThread;

// a handle to window display context
GLFWwindow* window = NULL;

// current width of window
int width = 0;

// current height of window
int height = 0;

// swap interval for the display context (vertical synchronization)
int swapInterval = 1;

// root resource path
string resourceRoot;


//---------------------------------------------------------------------------
// DECLARED MACROS
//---------------------------------------------------------------------------

// convert to resource path
#define RESOURCE_PATH(p)    (char*)((resourceRoot+string(p)).c_str())

//---------------------------------------------------------------------------
// DECLARED FUNCTIONS
//---------------------------------------------------------------------------

// callback when the window display is resized
void windowSizeCallback(GLFWwindow* a_window, int a_width, int a_height);

// callback when an error GLFW occurs
void errorCallback(int error, const char* a_description);

// callback when a key is pressed
void keyCallback(GLFWwindow* a_window, int a_key, int a_scancode, int a_action, int a_mods);

// this function renders the scene
void updateGraphics(void);

// this function contains the main haptics simulation loop
void updateHaptics(void);

// this function closes the application
void close(void);

const int MAX_DEVICES = 10;

///////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////
class DataExchange{
public:
    virtual cVector3d measured_pos(){};
    virtual cMatrix3d measured_rot(){};
    virtual cVector3d measured_vel(){};
    virtual bool button_pressed(int button_index){};
    virtual double measured_gripper_angle(){};
    virtual void apply_wrench(cVector3d force, cVector3d torque){};
    virtual void apply_force(cVector3d force){};
    virtual void apply_torque(cVector3d torque){};
};
///////////////////////////////////////////////////////////////////////////////////////////////////////////////
class Sim{
public:
    Sim(){
        workspaceScaleFactor = 30.0;
        linGain = 0.05;
        linG = 0.0;
        angG = 0.0;
        angGain = 0.03;
        linStiffness = 1000;
        angStiffness = 30;
        cont_lin_damping = 5.0;
        cont_ang_damping = 3.0;
        dPos.set(0,0,0);
        dPos_last.set(0,0,0);
        ddPos.set(0,0,0);
        dRot.identity();
        dRot_last.identity();
        ddRot.identity();
        cam_clutch = 1;
        pos_clutch = 0;
        _camTrigger = false;
        _posTrigger = false;
        posSimLast.set(0.0,0.0,0.0);
        rotSimLast.identity();
    }
    void set_sim_params(cHapticDeviceInfo &a_hInfo);
    cVector3d posSim, posSimLast;
    cMatrix3d rotSim, rotSimLast;
    cVector3d dPos, dPos_last, ddPos;
    cMatrix3d dRot, dRot_last, ddRot;
    double workspaceScaleFactor;
    double linGain;
    double angGain;
    double linG;
    double angG;
    double linStiffness;
    double angStiffness;
    double cont_lin_damping;
    double cont_ang_damping;
    int cam_clutch;
    int pos_clutch;
    bool _camTrigger;
    bool _posTrigger;
};

void Sim::set_sim_params(cHapticDeviceInfo &a_hInfo){
    double maxStiffness	= a_hInfo.m_maxLinearStiffness / workspaceScaleFactor;

    // clamp the force output gain to the max device stiffness
    linGain = cMin(linGain, maxStiffness / linStiffness);

    if (strcmp(a_hInfo.m_modelName.c_str(), "MTM-R") == 0 ||
        strcmp(a_hInfo.m_modelName.c_str(), "MTM-L") == 0)
    {
        workspaceScaleFactor = 10.0;
        linGain = linGain/3;
        pos_clutch = 1;
        cam_clutch = 2;
    }
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////

class ToolGripper: public Sim, public DataExchange{
public:
    ToolGripper();
    ~ToolGripper();
    virtual cVector3d measured_pos();
    virtual cMatrix3d measured_rot();
    virtual void apply_force(cVector3d force);
    virtual void apply_torque(cVector3d torque);
    cBulletGripper* tool;
    cVector3d posTool;
    cMatrix3d rotTool;
};

ToolGripper::ToolGripper(){
}
ToolGripper::~ToolGripper(){

}

cVector3d ToolGripper::measured_pos(){
    posTool = tool->getLocalPos();
    return posTool;
}

cMatrix3d ToolGripper::measured_rot(){
    rotTool = tool->getLocalRot();
    return rotTool;
}

void ToolGripper::apply_force(cVector3d force){
    tool->addExternalForce(force);
}

void ToolGripper::apply_torque(cVector3d torque){
    tool->addExternalTorque(torque);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////
class Device: public DataExchange{
public:
    Device();
    ~Device(){};
    virtual cVector3d measured_pos();
    virtual cMatrix3d measured_rot();
    virtual cVector3d measured_vel();
    virtual double measured_gripper_angle();
    virtual void apply_wrench(cVector3d force, cVector3d torque);
    virtual bool button_pressed(int button_index);
    cGenericHapticDevicePtr hDevice;
    cHapticDeviceInfo hInfo;
    cVector3d posDevice, posDeviceClutched, velDevice;
    cMatrix3d rotDevice, rotDeviceClutched;
    cVector3d force, torque;
};

Device::Device(){
}

cVector3d Device::measured_pos(){
    hDevice->getPosition(posDevice);
    return posDevice;
}

cMatrix3d Device::measured_rot(){
    hDevice->getRotation(rotDevice);
    return rotDevice;
}

cVector3d Device::measured_vel(){
    hDevice->getLinearVelocity(velDevice);
    return velDevice;
}

double Device::measured_gripper_angle(){
    double angle;
    hDevice->getGripperAngleRad(angle);
    return angle;
}

bool Device::button_pressed(int button_index){
    bool status;
    hDevice->getUserSwitch(button_index, status);
    return status;
}

void Device::apply_wrench(cVector3d force, cVector3d torque){
    hDevice->setForceAndTorqueAndGripperForce(force, torque, 0.0);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////

class Coordination{
    public:
    Coordination(cBulletWorld* a_bullet_world);
    bool retrieve_device_handle(uint dev_num);
    void create_bullet_gripper(uint dev_num);
    void open_devices();
    void close_devices();

    double increment_lin_gains(double a_gain);
    double increment_ang_gains(double a_gain);
    double increment_lin_stifnesses(double a_stiffness);
    double increment_ang_stiffnesses(double a_stiffness);

    cHapticDeviceHandler *device_handler;
    ToolGripper bulletTools[MAX_DEVICES];
    Device hapticDevices[MAX_DEVICES];
    uint m_num_devices;
    cBulletWorld* m_bullet_world;

    // bool to enable the rotation of tool be in camera frame. i.e. Orienting the camera
    // re-orients the tool.
    bool _useCamFrameRot;
};

Coordination::Coordination(cBulletWorld* a_bullet_world){
    m_bullet_world = NULL;
    m_bullet_world = a_bullet_world;
    device_handler = new cHapticDeviceHandler();
    m_num_devices = device_handler->getNumDevices();
    std::cerr << "Num of devices " << m_num_devices << std::endl;
    for (uint i = 0; i < m_num_devices; i++){
        retrieve_device_handle(i);
        create_bullet_gripper(i);
    }
    _useCamFrameRot = true;
}

bool Coordination::retrieve_device_handle(uint dev_num){
    device_handler->getDeviceSpecifications(hapticDevices[dev_num].hInfo, dev_num);
    return device_handler->getDevice(hapticDevices[dev_num].hDevice, dev_num);
}

void Coordination::create_bullet_gripper(uint dev_num){
    std::ostringstream dev_str;
    dev_str << (dev_num + 1);
    std::string gripper_name = "Gripper" + dev_str.str();
    bulletTools[dev_num].tool = new cBulletGripper(m_bullet_world, gripper_name);
    bulletTools[dev_num].set_sim_params(hapticDevices[dev_num].hInfo);
    bulletTools[dev_num].tool->build();
    m_bullet_world->addChild(bulletTools[dev_num].tool);
}

void Coordination::open_devices(){
    for (int i = 0 ; i < m_num_devices ; i++){
        hapticDevices[i].hDevice->open();
    }
}

void Coordination::close_devices(){
    for (int i = 0 ; i < m_num_devices ; i++){
        hapticDevices[i].hDevice->close();
    }
}


double Coordination::increment_lin_gains(double a_gain){
    for (int i = 0 ; i < m_num_devices ; i++){
        if (bulletTools[i].linGain + a_gain <= 0)
        {
            bulletTools[i].linGain = 0.0;
        }
        else{
            bulletTools[i].linGain += a_gain;
        }
    }
    //Set the return value to the gain of the last device
    if(m_num_devices > 0){
        a_gain = bulletTools[m_num_devices-1].linGain;
    }
    return a_gain;
}

double Coordination::increment_ang_gains(double a_gain){
    for (int i = 0 ; i < m_num_devices ; i++){
        if (bulletTools[i].angGain + a_gain <=0){
            bulletTools[i].angGain = 0.0;
        }
        else{
            bulletTools[i].angGain += a_gain;
        }
    }
    //Set the return value to the gain of the last device
    if(m_num_devices > 0){
        a_gain = bulletTools[m_num_devices-1].angGain;
    }
    return a_gain;
}

double Coordination::increment_lin_stifnesses(double a_stiffness){
    for (int i = 0 ; i < m_num_devices ; i++){
        if (bulletTools[i].linStiffness + a_stiffness <=0){
            bulletTools[i].linStiffness = 0.0;
        }
        else{
            bulletTools[i].linStiffness += a_stiffness;
        }
    }
    //Set the return value to the stiffness of the last device
    if(m_num_devices > 0){
        a_stiffness = bulletTools[m_num_devices-1].linStiffness;
    }
    return a_stiffness;
}

double Coordination::increment_ang_stiffnesses(double a_stiffness){
    for (int i = 0 ; i < m_num_devices ; i++){
        if (bulletTools[i].angStiffness + a_stiffness <=0){
            bulletTools[i].angStiffness = 0.0;
        }
        else{
            bulletTools[i].angStiffness += a_stiffness;
        }
    }
    //Set the return value to the stiffness of the last device
    if(m_num_devices > 0){
        a_stiffness = bulletTools[m_num_devices-1].angStiffness;
    }
    return a_stiffness;
}

std::shared_ptr<Coordination> coordPtr;

///////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////
//===========================================================================
/*
    DEMO:    08-bullet-tool.cpp

    This example illustrates the use of the Bullet framework for simulating
    haptic interaction with dynamic bodies. In this scene we create 4
    cubic meshes that we individually attach to ODE bodies. One of the blocks
    is attached to the haptic device through a virtual spring.
 */
//===========================================================================

int main(int argc, char* argv[])
{
    //-----------------------------------------------------------------------
    // INITIALIZATION
    //-----------------------------------------------------------------------

    cout << endl;
    cout << "-----------------------------------" << endl;
    cout << "CHAI3D" << endl;
    cout << "Demo: 08-bullet-coordination" << endl;
    cout << "Copyright 2003-2016" << endl;
    cout << "-----------------------------------" << endl << endl << endl;
    cout << "Keyboard Options:" << endl << endl;
    cout << "[h] - Display help menu" << endl;
    cout << "[1] - Enable gravity" << endl;
    cout << "[2] - Disable gravity" << endl << endl;
    cout << "[3] - decrease linear haptic gain" << endl;
    cout << "[4] - increase linear haptic gain" << endl;
    cout << "[5] - decrease angular haptic gain" << endl;
    cout << "[6] - increase angular haptic gain" << endl  << endl;
    cout << "[7] - decrease linear stiffness" << endl;
    cout << "[8] - increase linear stiffness" << endl;
    cout << "[9] - decrease angular stiffness" << endl;
    cout << "[0] - increase angular stiffness" << endl << endl;
    cout << "[q] - Exit application\n" << endl;
    cout << endl << endl;


    //-----------------------------------------------------------------------
    // OPEN GL - WINDOW DISPLAY
    //-----------------------------------------------------------------------

    // initialize GLFW library
    if (!glfwInit())
    {
        cout << "failed initialization" << endl;
        cSleepMs(1000);
        return 1;
    }

    // set error callback
    glfwSetErrorCallback(errorCallback);

    // compute desired size of window
    const GLFWvidmode* mode = glfwGetVideoMode(glfwGetPrimaryMonitor());
    int w = 0.8 * mode->height;
    int h = 0.5 * mode->height;
    int x = 0.5 * (mode->width - w);
    int y = 0.5 * (mode->height - h);

    // set OpenGL version
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);

    // set active stereo mode
    if (stereoMode == C_STEREO_ACTIVE)
    {
        glfwWindowHint(GLFW_STEREO, GL_TRUE);
    }
    else
    {
        glfwWindowHint(GLFW_STEREO, GL_FALSE);
    }

    // create display context
    window = glfwCreateWindow(w, h, "CHAI3D", NULL, NULL);
    if (!window)
    {
        cout << "failed to create window" << endl;
        cSleepMs(1000);
        glfwTerminate();
        return 1;
    }

    // get width and height of window
    glfwGetWindowSize(window, &width, &height);

    // set position of window
    glfwSetWindowPos(window, x, y);

    // set key callback
    glfwSetKeyCallback(window, keyCallback);

    // set resize callback
    glfwSetWindowSizeCallback(window, windowSizeCallback);

    // set current display context
    glfwMakeContextCurrent(window);

    // sets the swap interval for the current display context
    glfwSwapInterval(swapInterval);

    // initialize GLEW library
#ifdef GLEW_VERSION
    if (glewInit() != GLEW_OK)
    {
        cout << "failed to initialize GLEW library" << endl;
        glfwTerminate();
        return 1;
    }
#endif


    //-----------------------------------------------------------------------
    // 3D - SCENEGRAPH
    //-----------------------------------------------------------------------

    // create a dynamic world.
    bulletWorld = new cBulletWorld();

    // set the background color of the environment
    bulletWorld->m_backgroundColor.setWhite();

    // create a camera and insert it into the virtual world
    camera = new cCamera(bulletWorld);
    bulletWorld->addChild(camera);

    // position and orient the camera
    camera->set(cVector3d(3.0, 0.0, 0.3),    // camera position (eye)
                cVector3d(0.0, 0.0,-0.5),    // lookat position (target)
                cVector3d(0.0, 0.0, 1.0));   // direction of the "up" vector

    // set the near and far clipping planes of the camera
    camera->setClippingPlanes(0.01, 10.0);

    // set stereo mode
    camera->setStereoMode(stereoMode);

    // set stereo eye separation and focal length (applies only if stereo is enabled)
    camera->setStereoEyeSeparation(0.02);
    camera->setStereoFocalLength(2.0);

    // set vertical mirrored display mode
    camera->setMirrorVertical(mirroredDisplay);

    // create a light source
    light = new cSpotLight(bulletWorld);

    // attach light to camera
    bulletWorld->addChild(light);

    // enable light source
    light->setEnabled(true);

    // position the light source
    light->setLocalPos( 0, 0, 1.2);

    // define the direction of the light beam
    light->setDir(0,0,-1.0);

    // set uniform concentration level of light
    light->setSpotExponent(0.0);

    // enable this light source to generate shadows
    light->setShadowMapEnabled(true);

    // set the resolution of the shadow map
    light->m_shadowMap->setQualityLow();
    //light->m_shadowMap->setQualityMedium();

    // set light cone half angle
    light->setCutOffAngleDeg(45);


    //--------------------------------------------------------------------------
    // WIDGETS
    //--------------------------------------------------------------------------

    // create a font
    cFontPtr font = NEW_CFONTCALIBRI20();

    // create a label to display the haptic and graphic rate of the simulation
    labelRates = new cLabel(font);
    labelTimes = new cLabel(font);
    labelRates->m_fontColor.setBlack();
    labelTimes->m_fontColor.setBlack();
    camera->m_frontLayer->addChild(labelRates);
    camera->m_frontLayer->addChild(labelTimes);


    //////////////////////////////////////////////////////////////////////////
    // BULLET WORLD
    //////////////////////////////////////////////////////////////////////////
    // set some gravity
    bulletWorld->setGravity(cVector3d(0.0, 0.0, -9.8));


    //////////////////////////////////////////////////////////////////////////
    // 3 BULLET BLOCKS
    //////////////////////////////////////////////////////////////////////////
    double size = 0.40;
    cMaterial meshMat;

    bulletGear = new cBulletMultiMesh(bulletWorld, "Gear");
    bulletGear->loadFromFile(RESOURCE_PATH("../resources/models/gear/gear.3ds"));
    bulletGear->scale(0.0014);
    bulletWorld->addChild(bulletGear);
    bulletGear->buildContactTriangles(0.001);
    bulletGear->setMass(0.3);
    bulletGear->estimateInertia();
    bulletGear->buildDynamicModel();
    meshMat.setPinkDeep();
    bulletGear->setMaterial(meshMat);
    bulletGear->m_bulletRigidBody->setFriction(1);

    bulletTorus = new cBulletMultiMesh(bulletWorld, "Torus");
    bulletTorus->loadFromFile(RESOURCE_PATH("../resources/models/gear/torus.3ds"));
    bulletTorus->scale(0.2);
    bulletTorus->setLocalPos(cVector3d(0.3,0,0));
    bulletWorld->addChild(bulletTorus);
    bulletTorus->buildContactTriangles(0.001);
    bulletTorus->setMass(0.8);
    bulletTorus->estimateInertia();
    bulletTorus->buildDynamicModel();
    meshMat.setOrangeTomato();
    bulletTorus->setMaterial(meshMat);

    bulletBase = new cBulletMultiMesh(bulletWorld, "Base");
    bulletBase->loadFromFile(RESOURCE_PATH("../resources/models/gear/base.3ds"));
    bulletBase->scale(0.3);
    bulletBase->setLocalPos(cVector3d(-0.3,0,0));
    bulletWorld->addChild(bulletBase);
    bulletBase->buildContactTriangles(0.001);
    bulletBase->setMass(10);
    bulletBase->estimateInertia();
    bulletBase->buildDynamicModel();
    meshMat.setBlueNavy();
    bulletBase->setMaterial(meshMat);
    bulletBase->m_bulletRigidBody->setFriction(1);

    coordPtr = std::make_shared<Coordination>(bulletWorld);
    usleep(100);


    //////////////////////////////////////////////////////////////////////////
    // INVISIBLE WALLS
    //////////////////////////////////////////////////////////////////////////

    // we create 5 static walls to contain the dynamic objects within a limited workspace
    double planeWidth = 1.0;
    bulletBoxWall[0] = new cBulletStaticPlane(bulletWorld, cVector3d(0.0, 0.0, -1.0), -2.0 * planeWidth);
    bulletBoxWall[1] = new cBulletStaticPlane(bulletWorld, cVector3d(0.0, -1.0, 0.0), -1.5*planeWidth);
    bulletBoxWall[2] = new cBulletStaticPlane(bulletWorld, cVector3d(0.0, 1.0, 0.0), -1.5*planeWidth);
    bulletBoxWall[3] = new cBulletStaticPlane(bulletWorld, cVector3d(-1.0, 0.0, 0.0), -planeWidth);
    bulletBoxWall[4] = new cBulletStaticPlane(bulletWorld, cVector3d(1.0, 0.0, 0.0), -0.8 * planeWidth);

    for (int i = 0 ; i < 5 ; i++){
        cVector3d worldZ;
        worldZ.set(0,0,1);
        cVector3d planeOri = cCross(bulletBoxWall[i]->getPlaneNormal(), worldZ);
        cMatrix3d planeRot;
        planeRot.setAxisAngleRotationDeg(planeOri, 90);
        worldZ.set(0,0,1);
        bulletWorld->addChild(bulletBoxWall[i]);
        cCreatePlane(bulletBoxWall[i], 2.0, 3.0,
                     bulletBoxWall[i]->getPlaneConstant() * bulletBoxWall[i]->getPlaneNormal(),
                     planeRot);
        cMaterial matPlane;
        matPlane.setBlueSky();
        bulletBoxWall[i]->setMaterial(matPlane);
        bulletBoxWall[i]->setTransparencyLevel(0.5, true, true);
    }


    //////////////////////////////////////////////////////////////////////////
    // GROUND
    //////////////////////////////////////////////////////////////////////////

    // create ground plane
    bulletGround = new cBulletStaticPlane(bulletWorld, cVector3d(0.0, 0.0, 1.0), -planeWidth);

    // add plane to world as we will want to make it visibe
    bulletWorld->addChild(bulletGround);

    // create a mesh plane where the static plane is located
    cCreatePlane(bulletGround, 3.0, 3.0, bulletGround->getPlaneConstant() * bulletGround->getPlaneNormal());

    // define some material properties and apply to mesh
    cMaterial matGround;
    matGround.setGreenChartreuse();
    matGround.m_emission.setGrayLevel(0.3);
    bulletGround->setMaterial(matGround);
    bulletGround->m_bulletRigidBody->setFriction(1);

    //-----------------------------------------------------------------------
    // START SIMULATION
    //-----------------------------------------------------------------------

    // create a thread which starts the main haptics rendering loop
    hapticsThread = new cThread();
    hapticsThread->start(updateHaptics, CTHREAD_PRIORITY_HAPTICS);

    // setup callback when application exits
    atexit(close);


    //--------------------------------------------------------------------------
    // MAIN GRAPHIC LOOP
    //--------------------------------------------------------------------------

    // call window size callback at initialization
    windowSizeCallback(window, width, height);

    // main graphic loop
    while (!glfwWindowShouldClose(window))
    {
        // get width and height of window
        glfwGetWindowSize(window, &width, &height);

        // render graphics
        updateGraphics();

        // swap buffers
        glfwSwapBuffers(window);

        // process events
        glfwPollEvents();

        // signal frequency counter
        freqCounterGraphics.signal(1);
    }

    // close window
    glfwDestroyWindow(window);

    // terminate GLFW library
    glfwTerminate();

    // exit
    return 0;
}

//---------------------------------------------------------------------------

void windowSizeCallback(GLFWwindow* a_window, int a_width, int a_height)
{
    // update window size
    width = a_width;
    height = a_height;
}

//---------------------------------------------------------------------------

void errorCallback(int a_error, const char* a_description)
{
    cout << "Error: " << a_description << endl;
}

//---------------------------------------------------------------------------

void keyCallback(GLFWwindow* a_window, int a_key, int a_scancode, int a_action, int a_mods)
{
    // filter calls that only include a key press
    if ((a_action != GLFW_PRESS) && (a_action != GLFW_REPEAT))
    {
        return;
    }

    // option - exit
    else if ((a_key == GLFW_KEY_ESCAPE) || (a_key == GLFW_KEY_Q))
    {
        glfwSetWindowShouldClose(a_window, GLFW_TRUE);
    }

    // option - toggle fullscreen
    else if (a_key == GLFW_KEY_F)
    {
        // toggle state variable
        fullscreen = !fullscreen;

        // get handle to monitor
        GLFWmonitor* monitor = glfwGetPrimaryMonitor();

        // get information about monitor
        const GLFWvidmode* mode = glfwGetVideoMode(monitor);

        // set fullscreen or window mode
        if (fullscreen)
        {
            glfwSetWindowMonitor(window, monitor, 0, 0, mode->width, mode->height, mode->refreshRate);
            glfwSwapInterval(swapInterval);
        }
        else
        {
            int w = 0.8 * mode->height;
            int h = 0.5 * mode->height;
            int x = 0.5 * (mode->width - w);
            int y = 0.5 * (mode->height - h);
            glfwSetWindowMonitor(window, NULL, x, y, w, h, mode->refreshRate);
            glfwSwapInterval(swapInterval);
        }
    }

    // option - toggle vertical mirroring
    else if (a_key == GLFW_KEY_M)
    {
        mirroredDisplay = !mirroredDisplay;
        camera->setMirrorVertical(mirroredDisplay);
    }

    // option - help menu
    else if (a_key == GLFW_KEY_H)
    {
        cout << "Keyboard Options:" << endl << endl;    coordPtr->close_devices();
        cout << "[h] - Display help menu" << endl;
        cout << "[1] - Enable gravity" << endl;
        cout << "[2] - Disable gravity" << endl << endl;    coordPtr->close_devices();
        cout << "[3] - decrease linear haptic gain" << endl;
        cout << "[4] - increase linear haptic gain" << endl;
        cout << "[5] - decrease angular haptic gain" << endl;
        cout << "[6] - increase angular haptic gain" << endl << endl;
        cout << "[7] - decrease linear stiffness" << endl;
        cout << "[8] - increase linear stiffness" << endl;
        cout << "[9] - decrease angular stiffness" << endl;
        cout << "[0] - increase angular stiffness" << endl << endl;
        cout << "[q] - Exit application\n" << endl;
        cout << endl << endl;
    }

    // option - enable gravity
    else if (a_key == GLFW_KEY_1)
    {
        // enable gravity
        bulletWorld->setGravity(cVector3d(0.0, 0.0, -9.8));
        printf("gravity ON:\n");
    }

    // option - disable gravity
    else if (a_key == GLFW_KEY_2)
    {
        // disable gravity
        bulletWorld->setGravity(cVector3d(0.0, 0.0, 0.0));
        printf("gravity OFF:\n");
    }

    // option - decrease linear haptic gain
    else if (a_key == GLFW_KEY_3)
    {
        printf("linear haptic gain:  %f\n", coordPtr->increment_lin_gains(-0.05));
    }

    // option - increase linear haptic gain
    else if (a_key == GLFW_KEY_4)
    {
        printf("linear haptic gain:  %f\n", coordPtr->increment_lin_gains(0.05));
    }

    // option - decrease angular haptic gain
    else if (a_key == GLFW_KEY_5)
    {
        printf("angular haptic gain:  %f\n", coordPtr->increment_ang_gains(-0.05));
    }

    // option - increase angular haptic gain
    else if (a_key == GLFW_KEY_6)
    {
        printf("angular haptic gain:  %f\n", coordPtr->increment_ang_gains(0.05));
    }

    // option - decrease linear stiffness
    else if (a_key == GLFW_KEY_7)
    {
        printf("linear stiffness:  %f\n", coordPtr->increment_lin_stifnesses(-50));
    }

    // option - increase linear stiffness
    else if (a_key == GLFW_KEY_8)
    {
        printf("linear stiffness:  %f\n", coordPtr->increment_lin_stifnesses(50));
    }

    // option - decrease angular stiffness
    else if (a_key == GLFW_KEY_9)
    {
        printf("angular stiffness:  %f\n", coordPtr->increment_ang_stiffnesses(-1));
    }

    // option - increase angular stiffness
    else if (a_key == GLFW_KEY_0)
    {
        printf("angular stiffness:  %f\n", coordPtr->increment_ang_stiffnesses(1));
    }
    else if (a_key == GLFW_KEY_C){
        coordPtr->_useCamFrameRot = true;
        printf("Gripper Rotation w.r.t Camera Frame:\n");
    }
    else if (a_key == GLFW_KEY_W){
        coordPtr->_useCamFrameRot = false;
        printf("Gripper Rotation w.r.t World Frame:\n");
    }
//    // option - open gripper
//    else if (a_key == GLFW_KEY_S)
//    {
//        grip_angle -= 0.01;
//        printf("gripper angle:  %f\n", grip_angle);
//    }
//    // option - open close gripper
//    else if (a_key == GLFW_KEY_D)
//    {
//        grip_angle += 0.01;
//        printf("gripper angle:  %f\n", grip_angle);
//    }
}

//---------------------------------------------------------------------------

void close(void)
{
    // stop the simulation
    simulationRunning = false;

    // wait for graphics and haptics loops to terminate
    while (!simulationFinished) { cSleepMs(100); }

    // delete resources
    coordPtr->close_devices();
    delete hapticsThread;
    delete bulletWorld;
    delete coordPtr->device_handler;
}

//---------------------------------------------------------------------------

void updateGraphics(void)
{
    /////////////////////////////////////////////////////////////////////
    // UPDATE WIDGETS
    /////////////////////////////////////////////////////////////////////

    // update haptic and graphic rate data
    labelTimes->setText("Wall Time: " + cStr(clockWorld.getCurrentTimeSeconds(),2) + " s" +
                        + " / "+" Simulation Time: " + cStr(bulletWorld->getSimulationTime(),2) + " s");
    labelRates->setText(cStr(freqCounterGraphics.getFrequency(), 0) + " Hz / " +
        cStr(freqCounterHaptics.getFrequency(), 0) + " Hz");

    // update position of label
    labelTimes->setLocalPos((int)(0.5 * (width - labelTimes->getWidth())), 30);
    labelRates->setLocalPos((int)(0.5 * (width - labelRates->getWidth())), 10);
    bool _pressed;
    if (coordPtr->m_num_devices > 0){
        coordPtr->hapticDevices[0].hDevice->getUserSwitch(coordPtr->bulletTools[0].cam_clutch, _pressed);
        if(_pressed){
            double scale = 0.3;
            dev_vel = coordPtr->hapticDevices[0].measured_vel();
            coordPtr->hapticDevices[0].hDevice->getRotation(dev_rot_cur);
            camera->setLocalPos(camera->getLocalPos() + cMul(scale, cMul(camera->getGlobalRot(),dev_vel)));
            camera->setLocalRot(cMul(cam_rot_last, cMul(cTranspose(dev_rot_last), dev_rot_cur)));
        }
        if(!_pressed){
            cam_rot_last = camera->getGlobalRot();
            coordPtr->hapticDevices[0].hDevice->getRotation(dev_rot_last);
        }
    }

    /////////////////////////////////////////////////////////////////////
    // RENDER SCENE
    /////////////////////////////////////////////////////////////////////

    // update shadow maps (if any)
    bulletWorld->updateShadowMaps(false, mirroredDisplay);

    // render world
    camera->renderView(width, height);

    // wait until all GL commands are completed
    glFinish();

    // check for any OpenGL errors
    GLenum err = glGetError();
    if (err != GL_NO_ERROR) printf("Error:  %s\n", gluErrorString(err));
}

// Function to fix time dilation

double compute_dt(bool adjust_int_steps = true){
    double dt = clockWorld.getCurrentTimeSeconds() - bulletWorld->getSimulationTime();
    if (adjust_int_steps){
        if (dt >= bulletWorld->getIntegrationTimeStep() * bulletWorld->getIntegrationMaxIterations()){
            int int_steps_max =  bulletWorld->getIntegrationTimeStep() / dt;
            if (int_steps_max > bulletWorld->getIntegrationMaxIterations()){
                bulletWorld->setIntegrationMaxIterations(int_steps_max);
            }
        }}
    return dt;
}

//---------------------------------------------------------------------------

void updateHaptics(void)
{
    // simulation in now running
    simulationRunning = true;
    simulationFinished = false;

    // start haptic device
    coordPtr->open_devices();

    clockWorld.start(true);

    // update position and orientation of tool


    for(int i = 0 ; i < coordPtr->m_num_devices ; i++){
        coordPtr->hapticDevices[i].posDeviceClutched.set(0.0,0.0,0.0);
        coordPtr->hapticDevices[i].measured_rot();
        coordPtr->hapticDevices[i].rotDeviceClutched.identity();
        coordPtr->bulletTools[i].rotSimLast = coordPtr->hapticDevices[i].rotDevice;
    }

    // main haptic simulation loop
    while(simulationRunning)
    {
        // signal frequency counter
        freqCounterHaptics.signal(1);

        // Adjust time dilation by computing dt from clockWorld time and the simulationTime
        double dt = compute_dt();
        double nextSimInterval = dt;

        // compute global reference frames for each object
        for(int i = 0 ; i < coordPtr->m_num_devices ; i++){
            coordPtr->bulletTools[i].tool->set_gripper_angle(
                        3.0 - coordPtr->hapticDevices[i].measured_gripper_angle());
            bulletWorld->computeGlobalPositions(true);

            coordPtr->hapticDevices[i].posDevice = coordPtr->hapticDevices[i].measured_pos();
            coordPtr->hapticDevices[i].rotDevice = coordPtr->hapticDevices[i].measured_rot();

            //if(i == 0){
            if(coordPtr->hapticDevices[i].button_pressed(coordPtr->bulletTools[i].cam_clutch)){
                if(coordPtr->bulletTools[i]._camTrigger){
                    coordPtr->bulletTools[i]._camTrigger = false;
                    coordPtr->bulletTools[i].posSimLast = coordPtr->bulletTools[i].posSim / coordPtr->bulletTools[i].workspaceScaleFactor;
                    coordPtr->bulletTools[i].rotSimLast = coordPtr->bulletTools[i].rotSim;
                }
                coordPtr->hapticDevices[i].posDeviceClutched = coordPtr->hapticDevices[i].posDevice;
                coordPtr->hapticDevices[i].rotDeviceClutched = coordPtr->hapticDevices[i].rotDevice;
            }
            else{
                coordPtr->bulletTools[i]._camTrigger = true;
            }
            if(coordPtr->hapticDevices[i].button_pressed(coordPtr->bulletTools[i].pos_clutch)){
                if(coordPtr->bulletTools[i]._posTrigger){
                    coordPtr->bulletTools[i]._posTrigger = false;
                    coordPtr->bulletTools[i].posSimLast = coordPtr->bulletTools[i].posSim / coordPtr->bulletTools[i].workspaceScaleFactor;
                    coordPtr->bulletTools[i].rotSimLast = coordPtr->bulletTools[i].rotSim;
                }
                coordPtr->hapticDevices[i].posDeviceClutched = coordPtr->hapticDevices[i].posDevice;
                coordPtr->hapticDevices[i].rotDeviceClutched = coordPtr->hapticDevices[i].rotDevice;
            }
            else{
                coordPtr->bulletTools[i]._posTrigger = true;
            }
            //}
            coordPtr->bulletTools[i].posSim = coordPtr->bulletTools[i].posSimLast +
                    (camera->getLocalRot() * (coordPtr->hapticDevices[i].posDevice - coordPtr->hapticDevices[i].posDeviceClutched));
            if (!coordPtr->_useCamFrameRot){
                coordPtr->bulletTools[i].rotSim = coordPtr->bulletTools[i].rotSimLast * camera->getLocalRot() *
                    cTranspose(coordPtr->hapticDevices[i].rotDeviceClutched) * coordPtr->hapticDevices[i].rotDevice *
                    cTranspose(camera->getLocalRot());
            }
            else{
                coordPtr->bulletTools[i].rotSim = coordPtr->hapticDevices[i].rotDevice;
            }
            coordPtr->bulletTools[i].posSim.mul(coordPtr->bulletTools[i].workspaceScaleFactor);

            // read position of tool
            coordPtr->bulletTools[i].measured_pos();
            coordPtr->bulletTools[i].measured_rot();
            coordPtr->bulletTools[i].dPos_last = coordPtr->bulletTools[i].dPos;
            coordPtr->bulletTools[i].dRot_last = coordPtr->bulletTools[i].dRot;
            coordPtr->bulletTools[i].dPos = coordPtr->bulletTools[i].posSim - coordPtr->bulletTools[i].posTool;
            coordPtr->bulletTools[i].dRot = cTranspose(coordPtr->bulletTools[i].rotTool) * coordPtr->bulletTools[i].rotSim;
            double angle, dangle;
            cVector3d axis, daxis;
            coordPtr->bulletTools[i].dRot.toAxisAngle(axis, angle);

            coordPtr->bulletTools[i].ddPos = (coordPtr->bulletTools[i].dPos -
                                              coordPtr->bulletTools[i].dPos_last) / nextSimInterval;
            coordPtr->bulletTools[i].ddRot = (cTranspose(coordPtr->bulletTools[i].dRot)*
                                              coordPtr->bulletTools[i].dRot_last);
            coordPtr->bulletTools[i].ddRot.toAxisAngle(daxis, dangle);
            cVector3d force, torque;
            force = coordPtr->bulletTools[i].linStiffness * coordPtr->bulletTools[i].dPos +
                    coordPtr->bulletTools[i].cont_lin_damping * coordPtr->bulletTools[i].ddPos;
            coordPtr->bulletTools[i].apply_force(force);

            torque = (coordPtr->bulletTools[i].angStiffness * angle) * axis;
            //(3.0 * dangle * daxis ) / nextSimInterval;
            coordPtr->bulletTools[i].rotTool.mul(torque);
            coordPtr->bulletTools[i].apply_torque(torque);
            force = - coordPtr->bulletTools[i].linG * force;
            torque = -coordPtr->bulletTools[i].angG * torque;
            force.set(0,0,0);
            torque.set(0,0,0);

//            std::cerr << coordPtr->hapticDevices[i].posDeviceClutched.x() << " " <<
//                         coordPtr->hapticDevices[i].posDeviceClutched.y() << " " <<
//                         coordPtr->hapticDevices[i].posDeviceClutched.z() << " " <<
//                         std::endl;

            coordPtr->hapticDevices[i].apply_wrench(force, torque);

            if (coordPtr->bulletTools[i].linG < coordPtr->bulletTools[i].linGain)
            {
                coordPtr->bulletTools[i].linG = coordPtr->bulletTools[i].linG + 0.1 * dt * coordPtr->bulletTools[i].linGain;
            }
            else
            {
                coordPtr->bulletTools[i].linG = coordPtr->bulletTools[i].linGain;
            }

            if (coordPtr->bulletTools[i].angG < coordPtr->bulletTools[i].angGain)
            {
                coordPtr->bulletTools[i].angG = coordPtr->bulletTools[i].angG + 0.1 * dt * coordPtr->bulletTools[i].angGain;
            }
            else
            {
                coordPtr->bulletTools[i].angG = coordPtr->bulletTools[i].angGain;
            }

        }
        bulletWorld->updateDynamics(nextSimInterval, clockWorld.getCurrentTimeSeconds());
    }
    // exit haptics thread
    simulationFinished = true;
}
