#include "NavMeshComponent.h"

#include "Path/PortalPath.h"
#include "Path/PathPlanner.h"
#include "Path/Route.h"
#include "Navigation/AgentSpatialInfo.h"
#include "Navigation/NavSystem.h"
#include "StrategyComponent/Goal/Goal.h"
#include "TacticComponent/Path/PrefVelocity.h"
#include "Math/consts.h"
#include <iostream>

using namespace DirectX::SimpleMath;

namespace FusionCrowd
{
	NavMeshComponent::NavMeshComponent(std::shared_ptr<Simulator> simulator,
		std::shared_ptr<NavMeshLocalizer> localizer, std::shared_ptr<NavMeshSpatialQuery> spatial_query) :
		_simulator(simulator), _localizer(localizer), _navMesh(localizer->getNavMesh()), _headingDevCos(cos(MathUtil::PI)), _spatial_query(spatial_query)
	{
	}

	void NavMeshComponent::AddAgent(size_t id)
	{
		auto & agentGoal = _simulator->GetAgentGoal(id);
		AgentSpatialInfo & agentInfo = _simulator->GetSpatialInfo(id);
		agentInfo.pos = GetClosiestAvailablePoint(agentInfo.pos);

		unsigned int from = _localizer->getNodeId(agentInfo.pos);
		unsigned int to = _localizer->getNodeId(agentGoal.getCentroid());

		assert(from != NavMeshLocation::NO_NODE && "Agent is not on the nav mesh");
		//assert(to != NavMeshLocation::NO_NODE && "Agent goal is not on the nav mesh");
		if (from == NavMeshLocation::NO_NODE) {
			throw 1;
		}
		if (to==NavMeshLocation::NO_NODE){
			to = _localizer->getNodeId(GetClosiestAvailablePoint(agentGoal.getCentroid()));
		}
		std::shared_ptr<PathPlanner> planner = _localizer->getPlanner();
		PortalRoute * route = planner->getRoute(from, to, agentInfo.radius);
		auto path = std::make_shared<PortalPath>(agentInfo.pos, agentGoal, route, agentInfo.radius);

		NavMeshLocation location(_localizer->getNodeId(agentInfo.pos));
		location.setPath(path);

		AgentStruct agtStruct;
		agtStruct.id = id;
		agtStruct.location = location;

		_agents.push_back(agtStruct);
	}

	bool NavMeshComponent::DeleteAgent(size_t id)
	{
		return false;
	}

	void NavMeshComponent::Update(float timeStep)
	{
		for (auto & agtStruct : _agents)
		{
			size_t id = agtStruct.id;
			AgentSpatialInfo & info = _simulator->GetSpatialInfo(id);

			updateLocation(info, agtStruct, false);
			setPrefVelocity(info, agtStruct);
		}
	}

	DirectX::SimpleMath::Vector2 NavMeshComponent::GetClosiestAvailablePoint(DirectX::SimpleMath::Vector2 p) {
		if (_localizer->findNodeBlind(p) != NavMeshLocation::NO_NODE) return p;
		float min_dist = INFINITY;
		DirectX::SimpleMath::Vector2 res;
		for (int i = _localizer->getNavMesh()->getNodeCount() - 1; i >= 0; i--) {
			if (!_localizer->getNavMesh()->GetNodeByPos(i).deleted) {
				DirectX::SimpleMath::Vector2 center = _localizer->getNavMesh()->GetNodeByPos(i).getCenter();
				if ((p - center).LengthSquared() < min_dist) {
					min_dist = (p - center).LengthSquared();
					res = center;
				}
			}
		}
		return res;
	}

	void NavMeshComponent::setPrefVelocity(AgentSpatialInfo & agentInfo, AgentStruct & agentStruct)
	{
		auto & agentGoal = _simulator->GetAgentGoal(agentInfo.id);
		auto path = agentStruct.location.getPath();
		if (path == nullptr || path->getGoal().getID() != agentGoal.getID())
		{
			Vector2 goalPoint = agentGoal.getCentroid();
			unsigned int goalNode = _localizer->getNodeId(goalPoint);
			unsigned int agentNode = _localizer->getNodeId(agentInfo.pos);
			if (goalNode == NavMeshLocation::NO_NODE) {
				goalPoint = GetClosiestAvailablePoint(agentGoal.getCentroid());
				_simulator->SetAgentGoal(agentInfo.id, goalPoint);
				goalNode = _localizer->getNodeId(goalPoint);
			}
			if (goalNode == NavMeshLocation::NO_NODE || agentNode == NavMeshLocation::NO_NODE)
			{
				agentInfo.prefVelocity.setSpeed(0);
				return;
			}
			agentStruct.location.setNode(agentNode);
			unsigned int agtNode = agentStruct.location.getNode();

			PortalRoute* route = _localizer->getPlanner()->getRoute(agtNode, goalNode, agentInfo.radius * 2.f);

			auto newPath = std::make_shared<PortalPath>(agentInfo.pos, agentGoal, route, agentInfo.radius);
			// assign it to the localizer
			agentStruct.location.setPath(newPath);
		}
		agentInfo.prefVelocity.setSpeed(agentInfo.prefSpeed);
		path->setPreferredDirection(agentInfo, _headingDevCos);
	}

	unsigned int NavMeshComponent::updateLocation(AgentSpatialInfo & agentInfo, AgentStruct& agentStruct, bool force) const
	{
		// NOTE: This will create a default location instance if the agent didn't already
		//	have one
		NavMeshLocation & loc = agentStruct.location;

		unsigned int oldLoc = loc.getNode();
		unsigned int newLoc = oldLoc;
		if (loc._hasPath)
		{
			newLoc = loc._path->updateLocation(agentInfo, _navMesh, _localizer, _localizer->getPlanner());
		}
		else
		{
			//if ( _trackAll || force ) {
			const Vector2 & p = agentInfo.pos;
			unsigned int oldNode = (unsigned int)loc._nodeID;
			if (loc._nodeID == NavMeshLocation::NO_NODE)
			{
				loc._nodeID = _localizer->findNodeBlind(p);
			}
			else
			{
				const NavMeshNode& node = _navMesh->GetNodeByPos((unsigned int)loc._nodeID);
				if (!node.containsPoint(p))
				{
					// not in current node
					loc._nodeID = _localizer->testNeighbors(node, p);
					if (loc._nodeID == NavMeshLocation::NO_NODE)
					{
						loc._nodeID = _localizer->findNodeBlind(p);
					}
				}
			}
			if (loc._nodeID == NavMeshLocation::NO_NODE)
			{
				loc._nodeID = oldNode;
			}
			newLoc = (unsigned int)loc._nodeID;
		}

		_localizer->updateAgentPosition(agentInfo.id, oldLoc, newLoc);

		return newLoc;
	}

	unsigned int NavMeshComponent::getNodeId(size_t agentId) const
	{
		unsigned int node = NavMeshLocation::NO_NODE;
		return _agents[agentId].location.getNode();
	}

	unsigned int NavMeshComponent::getNodeId(size_t agentId, const std::string& grpName, bool searchAll)
	{
		unsigned int nodeId = getNodeId(agentId);
		if (nodeId == NavMeshLocation::NO_NODE)
		{
			//node = findNodeInGroup(agent->_pos, grpName, searchAll);
			if (nodeId != NavMeshLocation::NO_NODE)
			{
				_agents[agentId].location.setNode(nodeId);
			}
		}
		return nodeId;
	}

	std::shared_ptr<NavMeshLocalizer> NavMeshComponent::GetLocalizer() const
	{
		return _localizer;
	}

	/*
	void NavMeshComponent::setNode(size_t agentID, unsigned int nodeID)
	{
		_locations[agentID].setNode(nodeID);
	}

	PortalPath * NavMeshComponent::getPath(size_t id)
	{
		PortalPath * path = 0x0;
		auto loc = _agents[id].location;
		if (loc.isPath()) {
			path = loc._path;
		}
		return path;
	}

	void NavMeshComponent::setPath(size_t agentID, PortalPath * path)
	{
		_locations[agentID].setPath(path);
	}

	void NavMeshComponent::clearPath(size_t agentID)
	{
		if (_locations.count(agentID) > 0) {
			_locations[agentID].clearPath();
		}
	}*/

	NavMeshComponent::~NavMeshComponent()
	{
	}
}
