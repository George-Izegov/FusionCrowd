#pragma once

#include <vector>

#include "Agent.h"
#include "Config.h"

#include "StrategyComponent/IStrategyComponent.h"
#include "StrategyComponent/Goal/PointGoal.h"
#include "TacticComponent/NavMeshComponent.h"
#include "TacticComponent/ITacticComponent.h"
#include "OperationComponent/IOperationComponent.h"

#include "Navigation/NavSystem.h"

namespace FusionCrowd
{
	class FUSION_CROWD_API Simulator
	{
	public:
		Simulator(const char* navMeshPath);
		~Simulator();

		bool DoStep();

		size_t GetAgentCount() const { return _agents.size(); }
		Agent & GetById(size_t agentId);
		NavSystem & GetNavSystem();

	    size_t AddAgent(
			float maxAngleVel,
			float radius,
			float prefSpeed,
			float maxSpeed,
			float maxAccel,
			Vector2 pos,
			Goal & g
		);

		void AddOperComponent(IOperationComponent & operComponent);
		void AddTacticComponent(ITacticComponent & tacticComponent);
		void AddStrategyComponent(IStrategyComponent & strategyComponent);

		void InitSimulator();
	private:
		size_t GetNextId() const { return GetAgentCount(); }

		NavSystem * _navSystem;

		// TEMPORARY SOLUTION
		NavMeshComponent * _navMeshTactic;

		std::vector<FusionCrowd::Agent> _agents;
		std::vector<std::reference_wrapper<IStrategyComponent>> strategyComponents;
		std::vector<std::reference_wrapper<ITacticComponent>> tacticComponents;
		std::vector<std::reference_wrapper<IOperationComponent>> operComponents;
	};
}
