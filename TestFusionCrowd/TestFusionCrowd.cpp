// TestFusionCrowd.cpp : This file contains the 'main' function. Program execution begins and ends there.

#include "pch.h"

#include <iostream>
#include <random>
#include <time.h>

#include "Agent.h"
#include "Simulator.h"
#include "Math/consts.h"

#include "StrategyComponent/Goal/GoalSet.h"
#include "StrategyComponent/Goal/Goal.h"
#include "StrategyComponent/Goal/PointGoal.h"

#include "TacticComponent/NavMeshComponent.h"

#include "OperationComponent/IOperationComponent.h"
#include "OperationComponent/HelbingComponent.h"
#include "OperationComponent/KaramouzasComponent.h"
#include "OperationComponent/ZanlungoComponent.h"
#include "OperationComponent/PedVOComponent.h"

#include "Navigation/NavSystem.h"
#include "OperationComponent/ORCAComponent.h"

using namespace DirectX::SimpleMath;

int main()
{
	std::string navPath = "Resources/simple.nav";

	FusionCrowd::Simulator sim(navPath.c_str());
	FusionCrowd::Karamouzas::KaramouzasComponent kComponent(sim);
	FusionCrowd::ORCA::ORCAComponent orcaComponent(sim);

	sim.AddOperComponent(kComponent);
	sim.AddOperComponent(orcaComponent);

	std::random_device rd;  //Will be used to obtain a seed for the random number engine
    std::mt19937 gen(rd());
	std::uniform_real_distribution<> uniform(-0.5f, 0.5f);

	FusionCrowd::PointGoal goal(-3.0f, 5.0f);


	std::vector<Vector2> positions;
	positions.push_back(Vector2(-0.55f, 4.0f));
	positions.push_back(Vector2(-0.50f, -1.5f));
	positions.push_back(Vector2(-0.1f, -1.5f));
	positions.push_back(Vector2(-0.1f, -1.1f));
	positions.push_back(Vector2(-0.5f, -1.1f));
	positions.push_back(Vector2(0.3f, -1.1f));
	positions.push_back(Vector2(0.3f, -1.5f));

	std::srand (time(NULL));
	std::random_shuffle(positions.begin(), positions.end());

	const int agentsCount = positions.size();
	for(int i = 0; i < agentsCount; i++)
	{
		//DirectX::SimpleMath::Vector2 pos(uniform(gen), uniform(gen));
		size_t id = sim.AddAgent(360, 0.19f, 0.05f, 0.2f, 5, positions[i], goal);

		if(i % 2 == 0)
			kComponent.AddAgent(id, 0.69f, 8.0f);
		else

		orcaComponent.AddAgent(id);
	}

	sim.InitSimulator();

	std::ofstream myfile;
    myfile.open ("traj.csv");
	size_t steps = 4000;

	auto & navSystem = sim.GetNavSystem();
	while (sim.DoStep() && steps--)
	{
		for(size_t i = 0; i < agentsCount; i++)
		{
			FusionCrowd::AgentSpatialInfo & agent = navSystem.GetSpatialInfo(i);
			if(i > 0) myfile << ",";

			myfile << agent.pos.x << "," << agent.pos.y;
		}
		myfile << std::endl;
	}

	myfile.close();
}
