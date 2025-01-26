#include <climits>
#include "wildfireSimulation.h"

int SimulationAdd()
{
	return 0;
}

void WildFireSimulation::InitializeWildFireFimulation()
{
	// Initialize the grid with default values.
	for (int index_Y = 0; index_Y < GRID_SIZE_Y; index_Y++) {
		for (int index_X = 0; index_X < GRID_SIZE_X; index_X++) {
			// Set the ID to be a unique value for this specific grid cell.
			grid[index_X][index_Y].gridCellID = (index_Y * GRID_SIZE_Y) + index_X;
			grid[index_X][index_Y].CurrentState = EGridCellState::NOT_ON_FIRE;
			grid[index_X][index_Y].Material = EGridCellMaterial::GRASS;
			grid[index_X][index_Y].CellLocation.X = index_X;
			grid[index_X][index_Y].CellLocation.Y = index_Y;
		}
	}

	currentTickCounter = 0;
}

void WildFireSimulation::UpdateWildFireSimulation()
{
	// Go through all the cells that need to be updated. We only want to update/query half the cells each update. Thus, we will manage a tick counter
	// that will determine which cells should be updated. We do this so that we can save on some performance/CPU cycles.

	for (int index_Y = 0; index_Y < GRID_SIZE_Y; index_Y++) {
		for (int index_X = 0; index_X < GRID_SIZE_X; index_X++) {
			WildFireGridCell& ThisGridCell = grid[index_Y][index_X];

			// Check if this grid cell needs to be updated this tick.
			if ((ThisGridCell.gridCellID % 2) != (currentTickCounter % 2)) {
				continue;
			}

			switch (ThisGridCell.CurrentState) {
			case EGridCellState::NOT_ON_FIRE:
			{
				// ToDo: Implement simulation for a cell not on fire.
			}
			break;
			case EGridCellState::ON_FIRE:
			{
				// ToDo: Implement simulation for a cell not on fire.
				// ToDo: Check if any of my neighbors are not on fire and should be put on fire using the random probability check.
			}
			break;
			case EGridCellState::DESTROYED:
			{
				// ToDo: Implement simulation for a cell destroyed by fire.
			}
			break;
			}
		}
	}


	// Reset the tick counter if it exceeds the maximum possible value of an integer on this system.
	if (currentTickCounter >= INT_MAX) {
		currentTickCounter = 0;
	}
	else {
		currentTickCounter++;
	}
}

std::vector<WildFireGridCell> WildFireSimulation::FindNeighboringGridCellsByState(IntVector2D LocationToCheck, EGridCellState StateToCheck, int SearchSize)
{
	// ToDo: Change this so that it returns a list of 

	// Determine the initial X index.
	int initialIndex_X = LocationToCheck.X - SearchSize;
	if (initialIndex_X < MIN_GRID_INDEX_X) {
		initialIndex_X = MIN_GRID_INDEX_X;
	}
	else if (initialIndex_X > MAX_GRID_INDEX_X) {
		initialIndex_X = MAX_GRID_INDEX_X;
	}

	// Determine the initial Y index.
	int initialIndex_Y = LocationToCheck.Y - SearchSize;
	if (initialIndex_Y < MIN_GRID_INDEX_Y) {
		initialIndex_Y = MIN_GRID_INDEX_Y;
	}
	else if (initialIndex_Y > MAX_GRID_INDEX_Y) {
		initialIndex_Y = MAX_GRID_INDEX_Y;
	}

	std::vector<WildFireGridCell> allNeighboringCellsWithState;

	for (int index_Y = initialIndex_Y; index_Y < (LocationToCheck.Y + SearchSize); index_Y++) {
		for (int index_X = initialIndex_X; index_X < (LocationToCheck.X + SearchSize); index_X++) {

			// Do not check for the exact same grid cell.
			if (index_Y == LocationToCheck.Y && index_X == LocationToCheck.X) {
				continue;
			}

			WildFireGridCell& ThisGridCell = grid[index_Y][index_X];

			if (ThisGridCell.CurrentState == StateToCheck) {
				allNeighboringCellsWithState.push_back(ThisGridCell);
			}
		}
	}

	return allNeighboringCellsWithState;
}

int WildFireSimulation::GetNumberOfCellsByState(EGridCellState StateToCheckBy)
{
	// Go through each cell and determine if it has the current state.
	int numberOfCellsWithState = 0;

	for (int index_Y = MIN_GRID_INDEX_Y; index_Y <= MAX_GRID_INDEX_Y; index_Y++) {
		for (int index_X = MIN_GRID_INDEX_X; index_X <= MAX_GRID_INDEX_X; index_X++) {

			WildFireGridCell& ThisGridCell = grid[index_Y][index_X];

			if (ThisGridCell.CurrentState == StateToCheckBy) {
				numberOfCellsWithState++;
			}
		}
	}

	return numberOfCellsWithState;
}
