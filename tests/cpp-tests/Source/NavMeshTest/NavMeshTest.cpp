/****************************************************************************
 Copyright (c) 2012 cocos2d-x.org
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

#include "NavMeshTest.h"
#include "physics3d/Physics3DWorld.h"
#include "physics3d/Physics3D.h"
#include "3d/Bundle3D.h"
#include "2d/Light.h"

USING_NS_AX_EXT;
USING_NS_AX;

struct AgentUserData
{
    float time;
};

NavMeshTests::NavMeshTests()
{
#if !defined(AX_ENABLE_NAVMESH) || !defined(AX_ENABLE_PHYSICS)
    ADD_TEST_CASE(NavMeshDisabled);
#else
    ADD_TEST_CASE(NavMeshBasicTestDemo);
    ADD_TEST_CASE(NavMeshAdvanceTestDemo);
#endif
};

#if !defined(AX_ENABLE_NAVMESH) || !defined(AX_ENABLE_PHYSICS)
void NavMeshDisabled::onEnter()
{
    TTFConfig ttfConfig("fonts/arial.ttf", 16);
    auto label =
        Label::createWithTTF(ttfConfig, "Should enable AX_ENABLE_NAVMESH & AX_ENABLE_PHYSICS\n to run this test case");

    auto size = Director::getInstance()->getWinSize();
    label->setPosition(Vec2(size.width / 2, size.height / 2));

    addChild(label);

    TestCase::onEnter();
}
#else

NavMeshBaseTestDemo::NavMeshBaseTestDemo() : _camera(nullptr), _needMoveAgents(false) {}

NavMeshBaseTestDemo::~NavMeshBaseTestDemo()
{
    for (auto&& iter : _agents)
    {
        AgentUserData* data = static_cast<AgentUserData*>(iter.first->getUserData());
        delete data;
    }
}

bool NavMeshBaseTestDemo::init()
{
    if (!TestCase::init())
        return false;

    if (initPhysicsWorld())
    {

        _angle = 0.0f;

        Size size = Director::getInstance()->getWinSize();
        _camera = Camera::createPerspective(30.0f, size.width / size.height, 1.0f, 1000.0f);
        _camera->setPosition3D(Vec3(0.0f, 50.0f, 100.0f));
        _camera->lookAt(Vec3(0.0f, 0.0f, 0.0f), Vec3(0.0f, 1.0f, 0.0f));
        _camera->setCameraFlag(CameraFlag::USER1);
        this->addChild(_camera);

        auto listener = EventListenerTouchAllAtOnce::create();
        listener->onTouchesBegan = AX_CALLBACK_2(NavMeshBaseTestDemo::onTouchesBegan, this);
        listener->onTouchesMoved = AX_CALLBACK_2(NavMeshBaseTestDemo::onTouchesMoved, this);
        listener->onTouchesEnded = AX_CALLBACK_2(NavMeshBaseTestDemo::onTouchesEnded, this);
        _eventDispatcher->addEventListenerWithSceneGraphPriority(listener, this);

        initScene();
        scheduleUpdate();
    }

    return true;
}

void NavMeshBaseTestDemo::onTouchesBegan(const std::vector<ax::Touch*>& touches, ax::Event* event)
{
    _needMoveAgents = true;
    touchesBegan(touches, event);
}

void NavMeshBaseTestDemo::onTouchesMoved(const std::vector<ax::Touch*>& touches, ax::Event* event)
{
    if (touches.size() && _camera)
    {
        auto touch = touches[0];
        auto delta = touch->getDelta();

        _angle -= AX_DEGREES_TO_RADIANS(delta.x);
        _camera->setPosition3D(Vec3(100.0f * sinf(_angle), 50.0f, 100.0f * cosf(_angle)));
        _camera->lookAt(Vec3(0.0f, 0.0f, 0.0f), Vec3(0.0f, 1.0f, 0.0f));

        if (delta.lengthSquared() > 16)
        {
            _needMoveAgents = false;
        }
    }
    touchesMoved(touches, event);
}

void NavMeshBaseTestDemo::onTouchesEnded(const std::vector<ax::Touch*>& touches, ax::Event* event)
{
    touchesEnded(touches, event);
}

void NavMeshBaseTestDemo::initScene()
{
    getPhysics3DWorld()->setDebugDrawEnable(false);
    // create mesh
    std::vector<Vec3> trianglesList = Bundle3D::getTrianglesList("NavMesh/scene.obj");

    Physics3DRigidBodyDes rbDes;
    rbDes.mass = 0.0f;
    rbDes.shape = Physics3DShape::createMesh(&trianglesList[0], (int)trianglesList.size() / 3);
    auto rigidBody = Physics3DRigidBody::create(&rbDes);
    auto component = Physics3DComponent::create(rigidBody);
    auto mesh = MeshRenderer::create("NavMesh/scene.obj");
    mesh->addComponent(component);
    mesh->setCameraMask((unsigned short)CameraFlag::USER1);
    this->addChild(mesh);
    setPhysics3DDebugCamera(_camera);

    auto navMesh = NavMesh::create("NavMesh/all_tiles_tilecache.bin", "NavMesh/geomset.txt");
    navMesh->setDebugDrawEnable(true);
    setNavMesh(navMesh);
    setNavMeshDebugCamera(_camera);

    auto ambientLight = AmbientLight::create(Color3B(64, 64, 64));
    ambientLight->setCameraMask((unsigned short)CameraFlag::USER1);
    this->addChild(ambientLight);

    auto dirLight = DirectionLight::create(Vec3(1.2f, -1.1f, 0.5f), Color3B(255, 255, 255));
    dirLight->setCameraMask((unsigned short)CameraFlag::USER1);
    this->addChild(dirLight);
}

void NavMeshBaseTestDemo::createAgent(const Vec3& pos)
{
    std::string filePath = "MeshRendererTest/girl.c3b";
    NavMeshAgentParam param;
    param.radius = 2.0f;
    param.height = 8.0f;
    param.maxSpeed = 8.0f;
    auto agent = NavMeshAgent::create(param);
    auto agentNode = MeshRenderer::create(filePath);
    agent->setOrientationRefAxes(Vec3(-1.0f, 0.0f, 1.0f));
    AgentUserData* data = new AgentUserData{0.0f};
    agent->setUserData(data);
    agentNode->setScale(0.05f);
    agentNode->addComponent(agent);

    auto node = Node::create();
    node->addChild(agentNode);
    node->setPosition3D(pos);
    node->setCameraMask((unsigned short)CameraFlag::USER1);
    this->addChild(node);

    auto animation = Animation3D::create(filePath);
    auto animate = Animate3D::create(animation);
    if (animate)
    {
        agentNode->runAction(RepeatForever::create(animate));
        animate->setSpeed(0);
    }

    _agents.emplace_back(std::make_pair(agent, animate));
}

void NavMeshBaseTestDemo::createObstacle(const Vec3& pos)
{
    auto obstacle = NavMeshObstacle::create(2.0f, 8.0f);
    auto obstacleNode = MeshRenderer::create("MeshRendererTest/cylinder.c3b");
    obstacleNode->setPosition3D(pos + Vec3(0.0f, -0.5f, 0.0f));
    obstacleNode->setRotation3D(Vec3(-90.0f, 0.0f, 0.0f));
    obstacleNode->setScale(0.3f);
    obstacleNode->addComponent(obstacle);
    obstacleNode->setCameraMask((unsigned short)CameraFlag::USER1);
    this->addChild(obstacleNode);
}

Vec3 jump(const Vec3* pV1, const Vec3* pV2, float height, float t)
{
    Vec3 pOut;
    pOut.x = pV1->x + t * (pV2->x - pV1->x);
    pOut.y = pV1->y + t * (pV2->y - pV1->y);
    pOut.z = pV1->z + t * (pV2->z - pV1->z);
    pOut.y += height * sinf((float)M_PI * t);
    return pOut;
}

void NavMeshBaseTestDemo::moveAgents(const ax::Vec3& des)
{
    for (auto&& iter : _agents)
    {
        NavMeshAgent::MoveCallback callback = [](NavMeshAgent* agent, float totalTimeAfterMove) {
            AgentUserData* data = static_cast<AgentUserData*>(agent->getUserData());
            if (agent->isOnOffMeshLink())
            {
                agent->setAutoTraverseOffMeshLink(false);
                agent->setAutoOrientation(false);
                OffMeshLinkData linkdata = agent->getCurrentOffMeshLinkData();

                agent->getOwner()->setPosition3D(
                    jump(&linkdata.startPosition, &linkdata.endPosition, 10.0f, data->time));
                Vec3 dir = linkdata.endPosition - linkdata.startPosition;
                dir.y = 0.0f;
                dir.normalize();
                Vec3 axes;
                Vec3 refAxes = Vec3(-1.0f, 0.0f, 1.0f);
                refAxes.normalize();
                Vec3::cross(refAxes, dir, &axes);
                float angle = Vec3::dot(refAxes, dir);
                agent->getOwner()->setRotationQuat(Quaternion(axes, acosf(angle)));
                data->time += 0.01f;
                if (1.0f < data->time)
                {
                    agent->completeOffMeshLink();
                    agent->setAutoOrientation(true);
                    data->time = 0.0f;
                }
            }
        };
        iter.first->move(des, callback);
    }
}

void NavMeshBaseTestDemo::update(float delta)
{
    for (auto&& iter : _agents)
    {
        float speed = iter.first->getCurrentVelocity().length() * 0.2f;
        iter.second->setSpeed(0.0f < speed ? speed : 0.0f);
    }
}

NavMeshBasicTestDemo::NavMeshBasicTestDemo() {}

NavMeshBasicTestDemo::~NavMeshBasicTestDemo() {}

std::string NavMeshBasicTestDemo::title() const
{
    return "Navigation Mesh Test";
}

std::string NavMeshBasicTestDemo::subtitle() const
{
    return "Basic Test";
}

void NavMeshBasicTestDemo::touchesEnded(const std::vector<ax::Touch*>& touches, ax::Event* event)
{
    if (!_needMoveAgents)
        return;
    if (!touches.empty())
    {
        auto touch = touches[0];
        auto location = touch->getLocationInView();
        Vec3 nearP(location.x, location.y, 0.0f), farP(location.x, location.y, 1.0f);

        auto size = Director::getInstance()->getWinSize();
        _camera->unproject(size, &nearP, &nearP);
        _camera->unproject(size, &farP, &farP);

        Physics3DWorld::HitResult result;
        getPhysics3DWorld()->rayCast(nearP, farP, &result);
        moveAgents(result.hitPosition);
    }
}

bool NavMeshBasicTestDemo::init()
{
    if (!NavMeshBaseTestDemo::init())
        return false;

    TTFConfig ttfConfig("fonts/arial.ttf", 15);
    _debugLabel = Label::createWithTTF(ttfConfig, "Debug Draw ON");
    _debugLabel->retain();
    auto menuItem1 = MenuItemLabel::create(_debugLabel, [=](Object*) {
        bool enabledDebug = !getNavMesh()->isDebugDrawEnabled();
        getNavMesh()->setDebugDrawEnable(enabledDebug);
        if (enabledDebug)
        {
            _debugLabel->setString("Debug Draw ON");
        }
        else
        {
            _debugLabel->setString("Debug Draw OFF");
        }
    });
    menuItem1->setAnchorPoint(Vec2::ANCHOR_TOP_LEFT);
    menuItem1->setPosition(Vec2(VisibleRect::left().x, VisibleRect::top().y - 100));
    auto menu = Menu::create(menuItem1, nullptr);
    menu->setPosition(Vec2::ZERO);
    addChild(menu);

    return true;
}

void NavMeshBasicTestDemo::onEnter()
{
    NavMeshBaseTestDemo::onEnter();

    Physics3DWorld::HitResult result;
    getPhysics3DWorld()->rayCast(Vec3(0.0f, 50.0f, 0.0f), Vec3(0.0f, -50.0f, 0.0f), &result);
    createAgent(result.hitPosition);
}

NavMeshAdvanceTestDemo::NavMeshAdvanceTestDemo() {}

NavMeshAdvanceTestDemo::~NavMeshAdvanceTestDemo() {}

bool NavMeshAdvanceTestDemo::init()
{
    if (!NavMeshBaseTestDemo::init())
        return false;

    TTFConfig ttfConfig("fonts/arial.ttf", 15);
    _obstacleLabel = Label::createWithTTF(ttfConfig, "Create Obstacle");
    _obstacleLabel->retain();
    _agentLabel = Label::createWithTTF(ttfConfig, "Create Agent");
    _agentLabel->retain();
    _debugLabel = Label::createWithTTF(ttfConfig, "Debug Draw ON");
    _debugLabel->retain();

    auto menuItem0 = MenuItemLabel::create(_obstacleLabel, [=](Object*) {
        float x = ax::random(-50.0f, 50.0f);
        float z = ax::random(-50.0f, 50.0f);
        Physics3DWorld::HitResult result;
        getPhysics3DWorld()->rayCast(Vec3(x, 50.0f, z), Vec3(x, -50.0f, z), &result);
        createObstacle(result.hitPosition);
    });
    menuItem0->setAnchorPoint(Vec2::ANCHOR_TOP_LEFT);
    menuItem0->setPosition(Vec2(VisibleRect::left().x, VisibleRect::top().y - 50));

    auto menuItem1 = MenuItemLabel::create(_agentLabel, [=](Object*) {
        float x = ax::random(-50.0f, 50.0f);
        float z = ax::random(-50.0f, 50.0f);
        Physics3DWorld::HitResult result;
        getPhysics3DWorld()->rayCast(Vec3(x, 50.0f, z), Vec3(x, -50.0f, z), &result);
        createAgent(result.hitPosition);
    });
    menuItem1->setAnchorPoint(Vec2::ANCHOR_TOP_LEFT);
    menuItem1->setPosition(Vec2(VisibleRect::left().x, VisibleRect::top().y - 100));

    auto menuItem2 = MenuItemLabel::create(_debugLabel, [=](Object*) {
        bool enabledDebug = !getNavMesh()->isDebugDrawEnabled();
        getNavMesh()->setDebugDrawEnable(enabledDebug);
        if (enabledDebug)
        {
            _debugLabel->setString("Debug Draw ON");
        }
        else
        {
            _debugLabel->setString("Debug Draw OFF");
        }
    });
    menuItem2->setAnchorPoint(Vec2::ANCHOR_TOP_LEFT);
    menuItem2->setPosition(Vec2(VisibleRect::left().x, VisibleRect::top().y - 150));

    auto menu = Menu::create(menuItem0, menuItem1, menuItem2, nullptr);
    menu->setPosition(Vec2::ZERO);
    addChild(menu);

    return true;
}

void NavMeshAdvanceTestDemo::onEnter()
{
    NavMeshBaseTestDemo::onEnter();

    Physics3DWorld::HitResult result;
    getPhysics3DWorld()->rayCast(Vec3(0.0f, 50.0f, 0.0f), Vec3(0.0f, -50.0f, 0.0f), &result);
    createAgent(result.hitPosition);
}

std::string NavMeshAdvanceTestDemo::title() const
{
    return "Navigation Mesh Test";
}

std::string NavMeshAdvanceTestDemo::subtitle() const
{
    return "Advance Test";
}

void NavMeshAdvanceTestDemo::touchesEnded(const std::vector<ax::Touch*>& touches, ax::Event* event)
{
    if (!_needMoveAgents)
        return;
    if (!touches.empty())
    {
        auto touch = touches[0];
        auto location = touch->getLocationInView();
        Vec3 nearP(location.x, location.y, 0.0f), farP(location.x, location.y, 1.0f);

        auto size = Director::getInstance()->getWinSize();
        _camera->unproject(size, &nearP, &nearP);
        _camera->unproject(size, &farP, &farP);

        Physics3DWorld::HitResult result;
        getPhysics3DWorld()->rayCast(nearP, farP, &result);
        moveAgents(result.hitPosition);
    }
}

#endif
