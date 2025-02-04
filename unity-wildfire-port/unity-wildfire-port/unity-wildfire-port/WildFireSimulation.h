#pragma once

#include <iostream>
#include <vector>
#include <map>
#include <string>
#include <random>

enum class EGridCellState
{
	NOT_ON_FIRE,
	ON_FIRE,
	DESTROYED
};

enum class EGridCellMaterial
{
	GRASS,
	WATER,
	BEDROCK
};

struct IntVector2D
{
	int X = 0;
	int Y = 0;
};

struct WildFireGridCell
{
	// This cell ID should be unique for every cell.
	int gridCellID;

	IntVector2D CellLocation;

	// Defines whether this grid cell is currently on fire or not.
	EGridCellState CurrentState = EGridCellState::NOT_ON_FIRE;

	// Defines how flammable this grid cell is.
	EGridCellMaterial Material = EGridCellMaterial::GRASS;
};

class WildFireSimulation
{
private:
	// The size of the grid cell array in the X-dimension
	static constexpr int GRID_SIZE_X = 64;

	// The size of the grid cell array in the Y-dimension
	static constexpr int GRID_SIZE_Y = 64;

	static constexpr float FIRE_PROB = 0.01f;

	static constexpr float FLAMMABLE_PROBABILITY_FOR_GRASS = 0.75f;
	static constexpr float FLAMMABLE_PROBABILITY_FOR_WATER = 0.f;
	static constexpr float FLAMMABLE_PROBABILITY_FOR_BEDROCK = 0.1f;

	static constexpr int MIN_GRID_INDEX_X = 0;
	static constexpr int MAX_GRID_INDEX_X = GRID_SIZE_X - 1;

	static constexpr int MIN_GRID_INDEX_Y = 0;
	static constexpr int MAX_GRID_INDEX_Y = GRID_SIZE_Y - 1;

	// This is the array of WildFireGridCells that we should manipulate and pull from.
	WildFireGridCell grid[GRID_SIZE_X][GRID_SIZE_Y];

	int currentTickCounter = 0;

	// Wind direction and offset maps
	std::string currentWind;
	static const std::vector<std::string> windDirections;
	std::map<std::string, std::vector<IntVector2D>> windOffsets;

	float temperatures[24];
	// Random number generation
	std::random_device rd;
	std::mt19937 gen;

	// Helper function to initialize wind offsets
	void InitializeWindOffsets();
	void UpdateWindDirection();
	bool CheckWindNeighbors(IntVector2D location);
	
	// Helper function for hourly temperature
	void GenerateTemperatures();

public:

	void InitializeWildFireFimulation();

	void WriteGridResultsToImage();

	// ToDo: We may need to pass over the DeltaTime if we have timers or anything like that.
	void UpdateWildFireSimulation(float dt);

	// Searches around a given grid cell location with the provided search size for any cells that have the given shape.
	std::vector<WildFireGridCell> FindNeighboringGridCellsByState(IntVector2D LocationToCheck, EGridCellState StateToCheck, int SearchSize = 1);

	std::vector<WildFireGridCell> FindOnFireNeighboringGridCells(IntVector2D LocationToCheck, int SearchSize = 1) {
		return FindNeighboringGridCellsByState(LocationToCheck, EGridCellState::ON_FIRE, SearchSize);
	}

	std::vector<WildFireGridCell> FindNotOnFireNeighboringGridCells(IntVector2D LocationToCheck, int SearchSize = 1) {
		return FindNeighboringGridCellsByState(LocationToCheck, EGridCellState::NOT_ON_FIRE, SearchSize);
	}

	std::vector<WildFireGridCell> FindDestroyedNeighboringGridCells(IntVector2D LocationToCheck, int SearchSize = 1) {
		return FindNeighboringGridCellsByState(LocationToCheck, EGridCellState::DESTROYED, SearchSize);
	}

	int GetGridSizeX() const {
		return GRID_SIZE_X;
	}

	int GetGridSizeY() const {
		return GRID_SIZE_Y;
	}

	int GetNumberOfCellsByState(EGridCellState StateToCheckBy);

	int GetNumberOfOnFireCells() {
		return GetNumberOfCellsByState(EGridCellState::ON_FIRE);
	}

	int GetNumberOfNotOnFireCells() {
		return GetNumberOfCellsByState(EGridCellState::NOT_ON_FIRE);
	}

	int GetNumberOfDestroyedCells() {
		return GetNumberOfCellsByState(EGridCellState::DESTROYED);
	}

};