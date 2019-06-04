#include "FusionCrowdLinkUE4.h"

#include "Simulator.h"
#include "Agent.h"

#include "Math/consts.h"
#include "Math/Util.h"

#include "StrategyComponent/Goal/GoalSet.h"
#include "StrategyComponent/Goal/Goal.h"
#include "StrategyComponent/Goal/PointGoal.h"

#include "OperationComponent/IOperationComponent.h"
#include "OperationComponent/KaramouzasComponent.h"
#include "OperationComponent/ORCAComponent.h"
#include "OperationComponent/PedVOComponent.h"

#include "TacticComponent/NavMeshComponent.h"

#include <algorithm>
#include <memory>

using namespace DirectX::SimpleMath;

FusionCrowdLinkUE4::FusionCrowdLinkUE4(): agentsCount(0)
{
}

FusionCrowdLinkUE4::~FusionCrowdLinkUE4()
{
}

void FusionCrowdLinkUE4::StartFusionCrowd(char* navMeshDir)
{
	navMeshPath = (char*)malloc(strlen(navMeshDir) + 1);
	strcpy(navMeshPath, navMeshDir);

	sim = new FusionCrowd::Simulator(navMeshPath);
//	navMeshTactic = new FusionCrowd::NavMeshComponent(*sim, );
	kComponent = std::make_shared<FusionCrowd::Karamouzas::KaramouzasComponent>(*sim);
	orcaComponent = std::make_shared<FusionCrowd::ORCA::ORCAComponent>(*sim);
	pedvoComponent = std::make_shared<FusionCrowd::PedVO::PedVOComponent>(*sim);

	sim->AddOperComponent(kComponent);
	//sim->AddTacticComponent(*navMeshTactic);
	sim->AddOperComponent(orcaComponent);
	sim->AddOperComponent(pedvoComponent);
}

int FusionCrowdLinkUE4::GetAgentCount()
{
	return sim->GetAgentCount();
}

void FusionCrowdLinkUE4::AddAgents(int agentsCount)
{
	std::vector<Vector2> positions;
	positions.push_back(Vector2(-0.55f, 4.0f));
	positions.push_back(Vector2(-0.50f, -1.5f));
	positions.push_back(Vector2(-0.1f, -1.5f));
	positions.push_back(Vector2(-0.1f, -1.1f));
	positions.push_back(Vector2(-0.5f, -1.1f));
	positions.push_back(Vector2(0.3f, -1.1f));
	positions.push_back(Vector2(0.3f, -1.5f));


	auto goal = std::make_shared<FusionCrowd::PointGoal>(-3.0f, 5.0f);
	for(Vector2 pos : positions)
	{
		size_t id = sim->AddAgent(360, 0.19f, 0.05f, 0.2f, 5, pos, goal);
		sim->SetOperationComponent(id, pedvoComponent->GetName());

		/*
		if(id % 2 == 0)
			sim->SetOperationComponent(id, kComponent->GetName());
		else
			sim->SetOperationComponent(id, orcaComponent->GetName());
		*/
		//navMeshTactic->AddAgent(id);
	}

	sim->InitSimulator();
}

void FusionCrowdLinkUE4::SetOperationModel(size_t agentId, const char * name)
{
	sim->SetOperationComponent(agentId, std::string(name));
}


void FusionCrowdLinkUE4::GetPositionAgents(agentInfo* ueAgentInfo)
{
	FusionCrowd::NavSystem & nav = sim->GetNavSystem();
	bool info = sim->DoStep();
	agentsCount = sim->GetAgentCount();

	for (int i = 0; i < agentsCount; i++)
	{
		auto spatialInfo = nav.GetSpatialInfo(i);

		ueAgentInfo[i].id = spatialInfo.id;

		ueAgentInfo[i].pos = new float[2];
		ueAgentInfo[i].pos[0] = spatialInfo.pos.x;
		ueAgentInfo[i].pos[1] = spatialInfo.pos.y;

		ueAgentInfo[i].vel = new float[2];
		ueAgentInfo[i].vel[0] = spatialInfo.vel.x;
		ueAgentInfo[i].vel[1] = spatialInfo.vel.y;

		ueAgentInfo[i].orient = new float[2];
		ueAgentInfo[i].orient[0] = spatialInfo.orient.x;
		ueAgentInfo[i].orient[1] = spatialInfo.orient.y;

		ueAgentInfo[i].radius = spatialInfo.radius;

		auto opComp = sim->GetById(i).opComponent;
		auto name = opComp != nullptr ? opComp->GetName() : std::string("NO_OP_COMPONENT");

		ueAgentInfo[i].opCompName = new char [name.length() + 1];
		std::strcpy (ueAgentInfo[i].opCompName, name.c_str());
	}
}