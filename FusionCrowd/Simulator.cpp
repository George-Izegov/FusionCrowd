#include "Simulator.h"

#include "Navigation/AgentSpatialInfo.h"
#include "TacticComponent/NavMeshComponent.h"

namespace FusionCrowd
{
	Simulator::Simulator()
	{
		_navSystem = NavSystem();
	}

	bool Simulator::DoStep()
	{
		const float timeStep = 0.1f;
		for (IStrategyComponent & strategy : strategyComponents)
		{
			strategy.Update(timeStep);
		}

		for (ITacticComponent & tactic : tacticComponents)
		{
			tactic.Update(timeStep);
		}

		for (IOperationComponent & oper : operComponents)
		{
			oper.Update(timeStep);
		}

		_navSystem.Update(timeStep);

		return true;
	}

	NavSystem & Simulator::GetNavSystem()
	{
		return _navSystem;
	}

	Agent & Simulator::getById(size_t id)
	{
		return _agents.at(id);
	}

	size_t Simulator::AddAgent(float maxAngleVel, float radius, float prefSpeed, float maxSpeed, float maxAccel, Vector2 pos, Goal & g)
	{
		size_t id = _agents.size();

		AgentSpatialInfo info;
		info.pos = pos;
		info.radius = radius;
		info.maxAngVel = maxAngleVel;
		info.prefSpeed = prefSpeed;
		info.maxSpeed = maxSpeed;
		info.maxAccel = maxAccel;

		_navSystem.AddAgent(id, info);
		_agents.push_back(Agent(id, g));

		return id;
	}

	void Simulator::AddOperComponent(IOperationComponent & component)
	{
		operComponents.push_back(component);
	}

	void Simulator::AddTacticComponent(ITacticComponent & component)
	{
		tacticComponents.push_back(component);
	}

	void Simulator::AddStrategyComponent(IStrategyComponent& component)
	{
		strategyComponents.push_back(component);
	}

	void Simulator::InitSimulator()
	{
	}

	Simulator::~Simulator()
	{
	}
}
