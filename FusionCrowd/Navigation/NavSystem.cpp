#include "NavSystem.h"


#include "Math/Util.h"
#include "TacticComponent/NavMeshComponent.h"

#include "Navigation/AgentSpatialInfo.h"
#include "Navigation/Obstacle.h"
#include "Navigation/NavMesh/NavMesh.h"
#include "Navigation/SpatialQuery/NavMeshSpatialQuery.h"
#include "Navigation/FastFixedRadiusNearestNeighbors/NeighborsSeeker.h"

#include <limits>
#include <unordered_map>

using namespace DirectX::SimpleMath;

namespace FusionCrowd
{
	class NavSystem::NavSystemImpl
	{
	public:
		NavSystemImpl(std::shared_ptr<NavMeshComponent> component)
			: _navMeshQuery(NavMeshSpatialQuery(component->GetLocalizer()))
		{
			_navMesh = component->GetNavMesh();
		}

		~NavSystemImpl() { }

		PublicSpatialInfo GetPublicSpatialInfo(size_t agentId)
		{
			PublicSpatialInfo publicInfo;
			auto & info = _agentSpatialInfos[agentId];
			publicInfo.id = agentId;

			publicInfo.posX = info.pos.x;
			publicInfo.posY = info.pos.y;

			publicInfo.velX = info.vel.x;
			publicInfo.velY = info.vel.y;

			publicInfo.orientX = info.orient.x;
			publicInfo.orientY = info.orient.y;
			publicInfo.radius = info.radius;

			return publicInfo;
		}

		//TEST METHOD, MUST BE DELETED
		int CountNeighbors(size_t agentId) const
		{
			auto neighbors = _agentsNeighbours.find(agentId);
			if (neighbors != _agentsNeighbours.end()) {
				return neighbors->second.size();
			}
			else
			{
				return 0;
			}
		}

		void SetAgentsSensitivityRadius(float radius)
		{
			_agentsSensitivityRadius = radius;
		}

		void AddAgent(size_t agentId, DirectX::SimpleMath::Vector2 position)
		{
			AgentSpatialInfo info;
			info.id = agentId;
			info.pos = position;

			_agentSpatialInfos.insert({agentId, info});
		}
		void AddAgent(AgentSpatialInfo spatialInfo)
		{
			_agentSpatialInfos.insert({spatialInfo.id, spatialInfo});
		}

		AgentSpatialInfo & GetSpatialInfo(size_t agentId)
		{
			return _agentSpatialInfos[agentId];
		}

		std::vector<AgentSpatialInfo> GetNeighbours(size_t agentId) const
		{
			auto result = _agentsNeighbours.find(agentId);
			if (result != _agentsNeighbours.end()) {
				return result->second;
			}
			else
			{
				return std::vector<AgentSpatialInfo>();
			}
		}

		std::vector<Obstacle> GetClosestObstacles(size_t agentId) const
		{
			AgentSpatialInfo agent = _agentSpatialInfos.at(agentId);

			std::vector<Obstacle> result;
			for(size_t obstId : _navMeshQuery.ObstacleQuery(agent.pos))
			{
				result.push_back(_navMesh->GetObstacle(obstId));
			}

			return result;
		}

		void Update(float timeStep)
		{
			for (auto & pair : _agentSpatialInfos)
			{
				size_t id = pair.first;
				AgentSpatialInfo & info = pair.second;

				UpdatePos(info, timeStep);
				UpdateOrient(info, timeStep);
			}

			if (_agentSpatialInfos.size() > 0) UpdateNeighbours();
		}

		void UpdatePos(AgentSpatialInfo & agent, float timeStep)
		{
			float delV = (agent.vel - agent.velNew).Length();

			if (delV > agent.maxAccel * timeStep) {
				float w = agent.maxAccel * timeStep / delV;
				agent.vel = (1.f - w) * agent.vel + w * agent.velNew;
			}
			else {
				agent.vel = agent.velNew;
			}

			agent.pos += agent.vel * timeStep;
		}

		void UpdateOrient(AgentSpatialInfo & agent, float timeStep)
		{
			float speed = agent.vel.Length();
		const float speedThresh = agent.prefSpeed / 3.f;
		Vector2 newOrient(agent.orient); // by default new is old
		Vector2 moveDir = agent.vel / speed;
		if (speed >= speedThresh)
		{
			newOrient = moveDir;
		}
		else
		{
			float frac = sqrtf(speed / speedThresh);
			Vector2 prefDir = agent.prefVelocity.getPreferred();
			// prefDir *can* be zero if we've arrived at goal.  Only use it if it's non-zero.
			if (prefDir.LengthSquared() > 0.000001f)
			{
				newOrient = frac * moveDir + (1.f - frac) * prefDir;
				newOrient.Normalize();
			}
		}

		// Now limit angular velocity.
		const float MAX_ANGLE_CHANGE = timeStep * agent.maxAngVel;
		float maxCt = cos(MAX_ANGLE_CHANGE);
		float ct = newOrient.Dot(agent.orient);
		if (ct < maxCt)
		{
			// changing direction at a rate greater than _maxAngVel
			float maxSt = sin(MAX_ANGLE_CHANGE);
			if (MathUtil::det(agent.orient, newOrient) > 0.f)
			{
				// rotate _orient left
				agent.orient = Vector2(
					maxCt * agent.orient.x - maxSt * agent.orient.y,
					maxSt * agent.orient.x + maxCt * agent.orient.y
				);
			}
			else
			{
				// rotate _orient right
				agent.orient = Vector2(
					maxCt * agent.orient.x + maxSt * agent.orient.y,
					-maxSt * agent.orient.x + maxCt * agent.orient.y
				);
			}
		}
		else
		{
			agent.orient = newOrient;
		}
		}

		void UpdateNeighbours()
		{
			int numAgents = _agentSpatialInfos.size();

			std::vector<AgentSpatialInfo> agentsInfos;
			agentsInfos.reserve(numAgents);
			for (auto & pair : _agentSpatialInfos) {
				agentsInfos.push_back(pair.second);
			}

			float minX = std::numeric_limits<float>::max();
			float minY = std::numeric_limits<float>::max();
			float maxX = std::numeric_limits<float>::min();
			float maxY = std::numeric_limits<float>::min();
			for (auto & info : agentsInfos) {
				if (info.pos.x < minX) minX = info.pos.x;
				if (info.pos.x > maxX) maxX = info.pos.x;
				if (info.pos.y < minY) minY = info.pos.y;
				if (info.pos.y > maxY) maxY = info.pos.y;
			}

			Point *agentsPositions = new Point[numAgents];
			int i = 0;
			for (auto & info : agentsInfos) {
				agentsPositions[i].x = info.pos.x - minX;
				agentsPositions[i].y = info.pos.y - minY;
				i++;
			}

			_neighborsSeeker.Init(agentsPositions, numAgents, maxX - minX, maxY - minY, _agentsSensitivityRadius);

			auto allNeighbors =_neighborsSeeker.FindNeighbors();

			_agentsNeighbours.clear();
			_agentsNeighbours.reserve(numAgents);
			i = 0;

			std::vector<AgentSpatialInfo> neighborsInfos;

			for (auto & info : agentsInfos) {
				auto neighbors = allNeighbors[i];

				neighborsInfos.clear();

				for (int j = 0; j < neighbors.neighborsCount; j++) {
					neighborsInfos.push_back(agentsInfos[neighbors.neighborsID[j]]);
				}

				_agentsNeighbours.insert({ info.id, neighborsInfos });

				i++;
			}

			delete[] agentsPositions;
		}

	private:
		std::map<size_t, AgentSpatialInfo> _agentSpatialInfos;
		NavMeshSpatialQuery _navMeshQuery;
		std::shared_ptr<NavMesh> _navMesh;

		NeighborsSeeker _neighborsSeeker;
		std::unordered_map<size_t, std::vector<AgentSpatialInfo>> _agentsNeighbours;
		float _agentsSensitivityRadius = 1;
	};

	NavSystem::NavSystem(std::shared_ptr<NavMeshComponent> component) :
		pimpl(std::make_unique<NavSystemImpl>(component))
	{
	}

	NavSystem::~NavSystem() = default;
	NavSystem::NavSystem(NavSystem&&) = default;
	NavSystem& NavSystem::operator=(NavSystem&&) = default;

	void NavSystem::AddAgent(size_t agentId, Vector2 position)
	{
		pimpl->AddAgent(agentId, position);
	}

	void NavSystem::AddAgent(AgentSpatialInfo spatialInfo)
	{
		pimpl->AddAgent(spatialInfo);
	}

	AgentSpatialInfo & NavSystem::GetSpatialInfo(size_t agentId)
	{
		return pimpl->GetSpatialInfo(agentId);
	}

	std::vector<AgentSpatialInfo> NavSystem::GetNeighbours(size_t agentId) const
	{
		return pimpl->GetNeighbours(agentId);
	}

	//TEST METHOD, MUST BE DELETED
	int NavSystem::CountNeighbors(size_t agentId) const {
		return pimpl->CountNeighbors(agentId);
	}

	std::vector<Obstacle> NavSystem::GetClosestObstacles(size_t agentId) const
	{
		return pimpl->GetClosestObstacles(agentId);
	}

	void NavSystem::Update(float timeStep)
	{
		pimpl->Update(timeStep);
	}

	void NavSystem::SetAgentsSensitivityRadius(float radius) {
		pimpl->SetAgentsSensitivityRadius(radius);
	}

	PublicSpatialInfo NavSystem::GetPublicSpatialInfo(size_t agentId)
	{
		return pimpl->GetPublicSpatialInfo(agentId);
	}
}
