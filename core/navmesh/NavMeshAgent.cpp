/****************************************************************************
 Copyright (c) 2015-2016 Chukong Technologies Inc.
 Copyright (c) 2017-2018 Xiamen Yaji Software Co., Ltd.
 Copyright (c) 2019-present Axmol Engine contributors (see AUTHORS.md).

 https://axmol.dev/

 Permission is hereby granted, free of charge, to any person obtaining a copy
 of this software and associated documentation files (the "Software"), to deal
 in the Software without restriction, including without limitation the rights
 to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 copies of the Software, and to permit persons to whom the Software is
 furnished to do so, subject to the following conditions:

 The above copyright notice and this permission notice shall be included in
 all copies or substantial portions of the Software.

 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 THE SOFTWARE.
 ****************************************************************************/

#include "navmesh/NavMeshAgent.h"
#if defined(AX_ENABLE_NAVMESH)

#    include "navmesh/NavMesh.h"
#    include "recast/DetourCrowd.h"
#    include "2d/Node.h"
#    include "2d/Scene.h"
#    include <algorithm>

NS_AX_BEGIN

NavMeshAgentParam::NavMeshAgentParam()
    : radius(0.6f)
    , height(2.0f)
    , maxAcceleration(8.0f)
    , maxSpeed(3.5f)
    , collisionQueryRange(radius * 12.0f)
    , pathOptimizationRange(radius * 30.0f)
    , separationWeight(2.0f)
    , updateFlags(DT_CROWD_ANTICIPATE_TURNS | DT_CROWD_OPTIMIZE_VIS | DT_CROWD_OPTIMIZE_TOPO |
                  DT_CROWD_OBSTACLE_AVOIDANCE)
    , obstacleAvoidanceType(3)
    , queryFilterType(0)
{}

NavMeshAgent* NavMeshAgent::create(const NavMeshAgentParam& param)
{
    auto ref = new NavMeshAgent();
    if (ref->initWith(param))
    {
        ref->autorelease();
        return ref;
    }
    AX_SAFE_DELETE(ref);
    return nullptr;
}

std::string_view NavMeshAgent::getNavMeshAgentComponentName()
{
    static std::string comName = "___NavMeshAgentComponent___";
    return comName;
}

ax::NavMeshAgent::NavMeshAgent()
    : _syncFlag(NODE_AND_NODE)
    , _rotRefAxes(Vec3::UNIT_Z)
    , _state(DT_CROWDAGENT_STATE_WALKING)
    , _needAutoOrientation(true)
    , _agentID(-1)
    , _needUpdateAgent(true)
    , _needMove(false)
    , _totalTimeAfterMove(0.0f)
    , _userData(nullptr)
    , _crowd(nullptr)
    , _navMeshQuery(nullptr)
{}

ax::NavMeshAgent::~NavMeshAgent() {}

bool NavMeshAgent::initWith(const NavMeshAgentParam& param)
{
    _param = param;
    setName(getNavMeshAgentComponentName());
    return true;
}

void ax::NavMeshAgent::setNavMeshQuery(dtNavMeshQuery* query)
{
    _navMeshQuery = query;
}

void ax::NavMeshAgent::removeFrom(dtCrowd* crowed)
{
    crowed->removeAgent(_agentID);
    _crowd   = nullptr;
    _agentID = -1;
}

void ax::NavMeshAgent::addTo(dtCrowd* crowed)
{
    _crowd = crowed;
    dtCrowdAgentParams ap;
    convertTodtAgentParam(_param, ap);
    Mat4 mat = _owner->getNodeToWorldTransform();
    _agentID = _crowd->addAgent(&mat.m[12], &ap);
}

void ax::NavMeshAgent::convertTodtAgentParam(const NavMeshAgentParam& inParam, dtCrowdAgentParams& outParam)
{
    memset(&outParam, 0, sizeof(outParam));
    outParam.collisionQueryRange   = inParam.collisionQueryRange;
    outParam.height                = inParam.height;
    outParam.maxAcceleration       = inParam.maxAcceleration;
    outParam.maxSpeed              = inParam.maxSpeed;
    outParam.obstacleAvoidanceType = inParam.obstacleAvoidanceType;
    outParam.pathOptimizationRange = inParam.pathOptimizationRange;
    outParam.queryFilterType       = inParam.queryFilterType;
    outParam.radius                = inParam.radius;
    outParam.separationWeight      = inParam.separationWeight;
    outParam.updateFlags           = inParam.updateFlags;
}

void ax::NavMeshAgent::onExit()
{
    if (_agentID == -1)
        return;
    Component::onExit();

    auto scene = _owner->getScene();
    if (scene && scene->getNavMesh())
    {
        scene->getNavMesh()->removeNavMeshAgent(this);
    }
}

void ax::NavMeshAgent::onEnter()
{
    if (_agentID != -1)
        return;
    Component::onEnter();
    auto scene = _owner->getScene();
    if (scene && scene->getNavMesh())
    {
        scene->getNavMesh()->addNavMeshAgent(this);
    }
}

float NavMeshAgent::getMaxSpeed() const
{
    return _param.maxSpeed;
}

void NavMeshAgent::setSeparationWeight(float weight)
{
    _param.separationWeight = weight;
    _needUpdateAgent        = true;
}

float NavMeshAgent::getSeparationWeight() const
{
    return _param.separationWeight;
}

void ax::NavMeshAgent::setObstacleAvoidanceType(unsigned char type)
{
    _param.obstacleAvoidanceType = type;
    _needUpdateAgent             = true;
}

unsigned char NavMeshAgent::getObstacleAvoidanceType() const
{
    return _param.obstacleAvoidanceType;
}

Vec3 NavMeshAgent::getCurrentVelocity() const
{
    if (_crowd)
    {
        auto agent = _crowd->getAgent(_agentID);
        if (agent)
        {
            return Vec3(agent->vel[0], agent->vel[1], agent->vel[2]);
        }
    }
    return Vec3::ZERO;
}

void NavMeshAgent::setMaxSpeed(float maxSpeed)
{
    _param.maxSpeed  = maxSpeed;
    _needUpdateAgent = true;
}

float NavMeshAgent::getMaxAcceleration() const
{
    return _param.maxAcceleration;
}

void NavMeshAgent::setMaxAcceleration(float maxAcceleration)
{
    _param.maxAcceleration = maxAcceleration;
    _needUpdateAgent       = true;
}

float NavMeshAgent::getHeight() const
{
    return _param.height;
}

void NavMeshAgent::setHeight(float height)
{
    _param.height    = height;
    _needUpdateAgent = true;
}

float NavMeshAgent::getRadius() const
{
    return _param.radius;
}

void NavMeshAgent::setRadius(float radius)
{
    _param.radius    = radius;
    _needUpdateAgent = true;
}

void NavMeshAgent::move(const Vec3& destination, const MoveCallback& callback)
{
    _destination     = destination;
    _moveCallback    = callback;
    _needMove        = true;
    _needUpdateAgent = true;
}

OffMeshLinkData NavMeshAgent::getCurrentOffMeshLinkData()
{
    OffMeshLinkData data;
    if (_crowd && isOnOffMeshLink())
    {
        auto agentAnim = _crowd->getEditableAgentAnim(_agentID);
        if (agentAnim)
        {
            Mat4 mat;
            if (_owner && _owner->getParent())
                mat = _owner->getParent()->getWorldToNodeTransform();
            mat.transformPoint(agentAnim->startPos, &data.startPosition);
            mat.transformPoint(agentAnim->endPos, &data.endPosition);
        }
    }
    return data;
}

bool NavMeshAgent::isOnOffMeshLink()
{
    return _state == DT_CROWDAGENT_STATE_OFFMESH;
}

void ax::NavMeshAgent::completeOffMeshLink()
{
    if (_crowd && isOnOffMeshLink())
    {
        _state           = DT_CROWDAGENT_STATE_WALKING;
        _needUpdateAgent = true;
    }
}

void NavMeshAgent::setAutoTraverseOffMeshLink(bool isAuto)
{
    if (_crowd && isOnOffMeshLink())
    {
        auto agentAnim = _crowd->getEditableAgentAnim(_agentID);
        if (agentAnim)
        {
            agentAnim->active = isAuto;
        }
    }
}

void NavMeshAgent::stop()
{
    if (_state != DT_CROWDAGENT_STATE_INVALID)
        return;
    _state           = DT_CROWDAGENT_STATE_INVALID;
    _needUpdateAgent = true;
}

void NavMeshAgent::setOrientationRefAxes(const Vec3& rotRefAxes)
{
    _rotRefAxes = rotRefAxes;
}

void ax::NavMeshAgent::setAutoOrientation(bool isAuto)
{
    _needAutoOrientation = isAuto;
}

void NavMeshAgent::resume()
{
    if (_state != DT_CROWDAGENT_STATE_INVALID)
        return;
    _state           = DT_CROWDAGENT_STATE_WALKING;
    _needUpdateAgent = true;
}

void NavMeshAgent::pause()
{
    if (_state == DT_CROWDAGENT_STATE_INVALID)
        return;
    _state           = DT_CROWDAGENT_STATE_INVALID;
    _needUpdateAgent = true;
}

void NavMeshAgent::preUpdate(float delta)
{
    if (_state != DT_CROWDAGENT_STATE_INVALID)
        _totalTimeAfterMove += delta;
    if (_moveCallback && _state != DT_CROWDAGENT_STATE_INVALID)
        _moveCallback(this, _totalTimeAfterMove);

    if ((_syncFlag & NODE_TO_AGENT) != 0)
        syncToAgent();

    if (_needMove && _crowd && _navMeshQuery)
    {
        if (_state == DT_CROWDAGENT_STATE_OFFMESH)
            return;
        _state              = DT_CROWDAGENT_STATE_WALKING;
        _totalTimeAfterMove = 0.0f;
        dtPolyRef pRef      = 0;
        float nearestPos[3];
        _navMeshQuery->findNearestPoly(&_destination.x, _crowd->getQueryExtents(), _crowd->getFilter(0), &pRef,
                                       nearestPos);
        _crowd->requestMoveTarget(_agentID, pRef, nearestPos);
        _needMove = false;
    }
}

void NavMeshAgent::postUpdate(float /*delta*/)
{
    if ((_syncFlag & AGENT_TO_NODE) != 0)
        syncToNode();
}

void NavMeshAgent::syncToNode()
{
    const dtCrowdAgent* agent = nullptr;
    if (_crowd)
    {
        agent = _crowd->getAgent(_agentID);
    }

    if (agent)
    {
        Mat4 wtop;
        Vec3 pos;
        if (_owner->getParent())
            wtop = _owner->getParent()->getWorldToNodeTransform();
        wtop.transformPoint(Vec3(agent->npos[0], agent->npos[1], agent->npos[2]), &pos);
        _owner->setPosition3D(pos);
        _state = agent->state;
        if (_needAutoOrientation)
        {
            if (std::abs(agent->vel[0]) > 0.3f || std::abs(agent->vel[1]) > 0.3f || std::abs(agent->vel[2]) > 0.3f)
            {
                Vec3 axes(_rotRefAxes);
                axes.normalize();
                Vec3 dir;
                wtop.transformVector(Vec3(agent->vel[0], agent->vel[1], agent->vel[2]), &dir);
                dir.normalize();
                float cosTheta = Vec3::dot(axes, dir);
                Vec3 rotAxes;
                Vec3::cross(axes, dir, &rotAxes);
                Quaternion rot = Quaternion(rotAxes, acosf(cosTheta));
                _owner->setRotationQuat(rot);
            }
        }
    }
}

void NavMeshAgent::syncToAgent()
{
    if (_crowd)
    {
        auto agent     = _crowd->getEditableAgent(_agentID);
        Mat4 mat       = _owner->getNodeToWorldTransform();
        agent->npos[0] = mat.m[12];
        agent->npos[1] = mat.m[13];
        agent->npos[2] = mat.m[14];
        // if (_needAutoOrientation){
        //	Vec3 vel = mat * _rotRefAxes;
        //	agent->vel[0] = vel.x;
        //	agent->vel[1] = vel.y;
        //	agent->vel[2] = vel.z;
        // }
        if (_needUpdateAgent)
        {
            dtCrowdAgentParams ap;
            convertTodtAgentParam(_param, ap);
            agent->params    = ap;
            agent->state     = _state;
            _needUpdateAgent = false;
        }
    }
}

Vec3 NavMeshAgent::getVelocity() const
{
    const dtCrowdAgent* agent = nullptr;
    if (_crowd)
    {
        agent = _crowd->getAgent(_agentID);
    }
    if (agent)
    {
        return Vec3(agent->vel[0], agent->vel[1], agent->vel[2]);
    }
    return Vec3::ZERO;
}

NS_AX_END

#endif  // AX_ENABLE_NAVMESH
