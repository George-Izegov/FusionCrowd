#include "pch.h"

#include "GoalShapeTestCase.h"

#include "Export/ComponentId.h"
#include "Export/Math/Shapes.h"
#include "Export/Export.h"

using namespace FusionCrowd;

namespace TestFusionCrowd
{
	GoalShapeTestCase::GoalShapeTestCase()
		: ITestCase(8, 500, true)
	{ }

	void GoalShapeTestCase::Pre()
	{
		std::unique_ptr<ISimulatorBuilder, decltype(&BuilderDeleter)> builder(BuildSimulator(), BuilderDeleter);
		builder
			->WithNavMesh("Resources/square.nav")
			->WithOp(ComponentIds::ORCA_ID);

		_sim = std::shared_ptr<ISimulatorFacade>(builder->Build(), SimulatorFacadeDeleter);

		SetupPointShape();
		SetupDiskShape();
	}

	void GoalShapeTestCase::SetupPointShape()
	{
		// Agents should come as close as possible from all directions
		size_t a1 = _sim->AddAgent(-5, 20, ComponentIds::ORCA_ID, ComponentIds::NAVMESH_ID, ComponentIds::NO_COMPONENT);
		size_t a2 = _sim->AddAgent( 5, 20, ComponentIds::ORCA_ID, ComponentIds::NAVMESH_ID, ComponentIds::NO_COMPONENT);
		size_t a3 = _sim->AddAgent( 0, 15, ComponentIds::ORCA_ID, ComponentIds::NAVMESH_ID, ComponentIds::NO_COMPONENT);
		size_t a4 = _sim->AddAgent( 0, 25, ComponentIds::ORCA_ID, ComponentIds::NAVMESH_ID, ComponentIds::NO_COMPONENT);

		Point goal { 0, 20 };

		_sim->SetAgentGoal(a1, goal);
		_sim->SetAgentGoal(a2, goal);
		_sim->SetAgentGoal(a3, goal);
		_sim->SetAgentGoal(a4, goal);
	}

	void GoalShapeTestCase::SetupDiskShape()
	{
		// Agents should stop at 3 units away
		size_t a1 = _sim->AddAgent(-5,  0, ComponentIds::ORCA_ID, ComponentIds::NAVMESH_ID, ComponentIds::NO_COMPONENT);
		size_t a2 = _sim->AddAgent( 5,  0, ComponentIds::ORCA_ID, ComponentIds::NAVMESH_ID, ComponentIds::NO_COMPONENT);
		size_t a3 = _sim->AddAgent( 0, -5, ComponentIds::ORCA_ID, ComponentIds::NAVMESH_ID, ComponentIds::NO_COMPONENT);
		size_t a4 = _sim->AddAgent( 0,  5, ComponentIds::ORCA_ID, ComponentIds::NAVMESH_ID, ComponentIds::NO_COMPONENT);

		Disk goal { 0, 0, 3 };

		_sim->SetAgentGoal(a1, goal);
		_sim->SetAgentGoal(a2, goal);
		_sim->SetAgentGoal(a3, goal);
		_sim->SetAgentGoal(a4, goal);
	}

}
