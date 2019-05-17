#include "NavMeshComponent.h"
#include "Path/PortalPath.h"
#include "Path/PathPlanner.h"
#include "Path/Route.h"

using namespace DirectX::SimpleMath;

namespace FusionCrowd
{
	NavMeshComponent::NavMeshComponent(Simulator & simulator, const char* navMeshPath) : _simulator(simulator)
	{
		_localizer = loadNavMeshLocalizer(navMeshPath, true);
		_navMesh = _localizer->getNavMesh();
	}

	void NavMeshComponent::AddAgent(size_t id)
	{
		Agent & agent = _simulator.getById(id);

		PathPlanner * planner = _localizer->getPlanner();
		unsigned int from = _localizer->getNode(agent.pos);
		unsigned int to = _localizer->getNode(agent.getCurrentGoal().getCentroid());
		PortalRoute * route = planner->getRoute(from, to, agent.radius);
		PortalPath * path = new PortalPath(agent.pos, &agent.getCurrentGoal(), route, agent.radius);

		NavMeshLocation location(_localizer->getNode(agent.pos));
		location.setPath(path);

		AgentStruct agtStruct;
		agtStruct.id = id;
		agtStruct.location = location;

		_agents.push_back(agtStruct);
	}

	bool NavMeshComponent::RemoveAgent(size_t id)
	{
		return false;
	}

	void NavMeshComponent::Update(float timeStep)
	{
		for (auto agtStruct : _agents)
		{
			auto agent = _simulator.getById(agtStruct.id);
			setPrefVelocity(agent, agtStruct);
			updateLocation(agent, agtStruct, false);
		}
	}

	void NavMeshComponent::setPrefVelocity(Agent & agent, AgentStruct& agentStruct)
	{
		PortalPath * path = agentStruct.location.getPath();
		if (path == nullptr)
		{
			Vector2 goalPoint = agent.getCurrentGoal().getCentroid();
			unsigned int goalNode = _localizer->getNode(goalPoint);
			if (goalNode == NavMeshLocation::NO_NODE)
			{
				return;
			}

			unsigned int agtNode = agentStruct.location.getNode();

			PortalRoute* route = _localizer->getPlanner()->getRoute(agtNode, goalNode, agent.radius * 2.f);

			path = new PortalPath(agent.pos, &(agent.getCurrentGoal()), route, agent.radius);
			// assign it to the localizer
			agentStruct.location.setPath(path);
		}
		agent.prefVelocity.setSpeed(agent.prefSpeed);
		path->setPreferredDirection(agent, _headingDevCos);
	}

	unsigned int NavMeshComponent::updateLocation(Agent & agent, const AgentStruct& agentStruct, bool force) const
	{
		const size_t ID = agent.id;
		// NOTE: This will create a default location instance if the agent didn't already
		//	have one
		NavMeshLocation loc = agentStruct.location;

		unsigned int oldLoc = loc.getNode();
		unsigned int newLoc = oldLoc;
		if (loc._hasPath)
		{
			newLoc = loc._path->updateLocation(agent, _navMesh, _localizer._data, _localizer->getPlanner());
		}
		else
		{
			//if ( _trackAll || force ) {
			const Vector2 & p = agent.pos;
			unsigned int oldNode = (unsigned int)loc._nodeID;
			if (loc._nodeID == NavMeshLocation::NO_NODE)
			{
				loc._nodeID = _localizer->findNodeBlind(p);
			}
			else
			{
				const NavMeshNode& node = _navMesh->GetNode((unsigned int)loc._nodeID);
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

		_localizer->updateAgentPosition(agent.id, oldLoc, newLoc);

		return newLoc;
	}

	unsigned int NavMeshComponent::getNode(size_t agentId) const
	{
		unsigned int node = NavMeshLocation::NO_NODE;
		return _agents[agentId].location.getNode();
	}

	unsigned int NavMeshComponent::getNode(size_t agentId, const std::string& grpName, bool searchAll)
	{
		unsigned int nodeId = getNode(agentId);
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
