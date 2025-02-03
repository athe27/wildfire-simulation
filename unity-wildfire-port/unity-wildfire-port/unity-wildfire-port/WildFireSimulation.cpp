#define _CRT_SECURE_NO_WARNINGS
#define _USE_MATH_DEFINES
#include <climits>
#include <vector>
#include <iostream>
#include <fstream>
#include <ctime>
#include <iomanip>
#include <sstream>
#include <string>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

#include "wildfireSimulation.h"

const std::vector<std::string> WildFireSimulation::windDirections = {
	"calm", "S", "N", "W", "E", "SW", "SE", "NW", "NE"
};

void WildFireSimulation::InitializeWindOffsets() {
	// Initialize all wind direction offsets
	windOffsets["calm"] = {
		{-1,-1}, {-1,0}, {-1,1}, {0,-1}, {0,1}, {1,-1}, {1,0}, {1,1}
	};
	windOffsets["N"] = {
		{-1,-1}, {-1,0}, {-1,1}, {0,-1}, {0,1}
	};
	windOffsets["S"] = {
		{0,-1}, {0,1}, {1,-1}, {1,0}, {1,1}
	};
	windOffsets["E"] = {
		{-1,0}, {-1,1}, {0,1}, {1,0}, {1,1}
	};
	windOffsets["W"] = {
		{-1,-1}, {-1,0}, {0,-1}, {1,-1}, {1,0}
	};
	windOffsets["NE"] = {
		{-1,0}, {-1,1}, {0,1}
	};
	windOffsets["NW"] = {
		{-1,-1}, {-1,0}, {0,-1}
	};
	windOffsets["SE"] = {
		{0,1}, {1,0}, {1,1}
	};
	windOffsets["SW"] = {
		{0,-1}, {1,-1}, {1,0}
	};

	UpdateWindDirection();
}

void WildFireSimulation::UpdateWindDirection() {
	// Randomly select a wind direction
	std::uniform_int_distribution<> dis(0, windDirections.size() - 1);
	currentWind = windDirections[dis(gen)];
}

bool WildFireSimulation::CheckWindNeighbors(IntVector2D location) {
	const auto& offsets = windOffsets[currentWind];

	for (const auto& offset : offsets) {
		int ni = location.X + offset.X;
		int nj = location.Y + offset.Y;

		// Check bounds
		if (ni >= 0 && nj >= 0 && ni < GRID_SIZE_X && nj < GRID_SIZE_Y) {
			if (grid[ni][nj].CurrentState == EGridCellState::ON_FIRE) {
				return true;
			}
		}
	}
	return false;
}

void WildFireSimulation::GenerateTemperatures() {
	float average_temp = 20.0f;
	float amplitude = 5.0f;
	float noise_level = 2.0f;
	int hours_in_day = 24;

	std::default_random_engine generator;
	std::normal_distribution<float> noise(0.0f, noise_level);
	for (int hour = 0; hour < hours_in_day; hour++) {
		// Generate the temperature based on the sinusoidal pattern
		float temp = average_temp + amplitude * std::sin(2 * M_PI * hour / 24 - M_PI / 2);
		// Add random noise
		temp += noise(generator);
		temperatures[hour] = temp;
	}
}

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
	GenerateTemperatures();
}

std::string getCurrentTimestamp() {
	auto now = std::time(nullptr);
	std::tm tm_struct;
#ifdef _WIN32
	localtime_s(&tm_struct, &now);  // Windows-specific
#else
	localtime_r(&now, &tm_struct);  // Linux/macOS
#endif

	std::ostringstream oss;
	oss << std::put_time(&tm_struct, "%Y-%m-%d_%H-%M-%S");  // Format: YYYY-MM-DD_HH-MM-SS
	return oss.str();
}

void WildFireSimulation::WriteGridResultsToImage()
{
	std::vector<uint8_t> image_data(GRID_SIZE_X * GRID_SIZE_Y * 3);  // RGB image

	// Fill the image with some color (example: red gradient)
	for (int Index_Y = 0; Index_Y < GRID_SIZE_Y; ++Index_Y) {
		for (int Index_X = 0; Index_X < GRID_SIZE_X; ++Index_X) {

			WildFireGridCell& ThisGridCell = grid[Index_X][Index_Y];

			switch (ThisGridCell.CurrentState) {
			case EGridCellState::NOT_ON_FIRE:
			{
				image_data[(Index_Y * GRID_SIZE_X + Index_X) * 3 + 0] = 0;    // Red channel
				image_data[(Index_Y * GRID_SIZE_X + Index_X) * 3 + 1] = 255;  // Green channel
				image_data[(Index_Y * GRID_SIZE_X + Index_X) * 3 + 2] = 0;    // Blue channel
			}
			break;
			case EGridCellState::ON_FIRE:
			{
				image_data[(Index_Y * GRID_SIZE_X + Index_X) * 3 + 0] = 255;  // Red channel
				image_data[(Index_Y * GRID_SIZE_X + Index_X) * 3 + 1] = 0;    // Green channel
				image_data[(Index_Y * GRID_SIZE_X + Index_X) * 3 + 2] = 0;    // Blue channel
			}
			break;
			case EGridCellState::DESTROYED:
			{
				image_data[(Index_Y * GRID_SIZE_X + Index_X) * 3 + 0] = 255;
				image_data[(Index_Y * GRID_SIZE_X + Index_X) * 3 + 1] = 255;
				image_data[(Index_Y * GRID_SIZE_X + Index_X) * 3 + 2] = 255;
			}
			break;
			}
		}
	}

	std::string new_image_filename = "OutputImages/" + getCurrentTimestamp() + ".png";
	const char* new_image_filename_as_char = new_image_filename.c_str();

	// Write the image to a PNG file
	if (stbi_write_png(new_image_filename_as_char, GRID_SIZE_X, GRID_SIZE_Y, 3, image_data.data(), GRID_SIZE_X * 3)) {
		std::cout << "Image saved successfully!" << std::endl;
	}
	else {
		std::cerr << "Failed to save image!" << std::endl;
	}
}

void WildFireSimulation::UpdateWildFireSimulation(float dt)
{
	// Allocate newGrid on the heap
	WildFireGridCell** newGrid = new WildFireGridCell * [GRID_SIZE_X];
	for (int x = 0; x < GRID_SIZE_X; x++) {
		newGrid[x] = new WildFireGridCell[GRID_SIZE_Y];
	}

	// Copy the current grid to the new grid
	for (int y = 0; y < GRID_SIZE_Y; y++) {
		for (int x = 0; x < GRID_SIZE_X; x++) {
			newGrid[x][y] = grid[x][y];
		}
	}

	int hourOfDay = currentTickCounter % 24;
	float currentTemperature = temperatures[hourOfDay];
	bool isHot = currentTemperature > 25.0f;

	// Go through all the cells that need to be updated. We only want to update/query half the cells each update. Thus, we will manage a tick counter
	// that will determine which cells should be updated. We do this so that we can save on some performance/CPU cycles.
	for (int index_Y = 0; index_Y < GRID_SIZE_Y; index_Y++) {
		for (int index_X = 0; index_X < GRID_SIZE_X; index_X++) {
			WildFireGridCell& ThisGridCell = grid[index_Y][index_X];

			// Check if this grid cell needs to be updated this tick.
			/*if ((ThisGridCell.gridCellID % 2) != (currentTickCounter % 2)) {
				continue;
			}*/

			switch (ThisGridCell.CurrentState) {
			case EGridCellState::NOT_ON_FIRE:
			{
				// Check if this cell should catch fire based on neighbors
				IntVector2D location = { index_X, index_Y };
				bool neighborOnFire =
					CheckWindNeighbors(location);

				// If we have fire neighbors, check probability to catch fire
				if (neighborOnFire) {
					float flammableProb = 0.f;
					switch (ThisGridCell.Material) {
					case EGridCellMaterial::GRASS:
						flammableProb = FLAMMABLE_PROBABILITY_FOR_GRASS;
						break;
					case EGridCellMaterial::WATER:
						flammableProb = FLAMMABLE_PROBABILITY_FOR_WATER;
						break;
					case EGridCellMaterial::BEDROCK:
						flammableProb = FLAMMABLE_PROBABILITY_FOR_BEDROCK;
						break;
					}

					// Generate random probability between 0 and 1
					float randomProb = static_cast<float>(rand()) / RAND_MAX;
					if (isHot) {
						randomProb *= 2;
					}
					if (randomProb < flammableProb) {
						newGrid[index_X][index_Y].CurrentState = EGridCellState::ON_FIRE;
					}
				}
			}
			break;
			case EGridCellState::ON_FIRE:
			{
				// Burning cells turn to destroyed after one tick
				newGrid[index_X][index_Y].CurrentState = EGridCellState::DESTROYED;
			}
			break;
			case EGridCellState::DESTROYED:
			{
				// Do nothing
			}
			break;
			}
		}
	}

	// Update the main grid with the new states
	for (int y = 0; y < GRID_SIZE_Y; y++) {
		for (int x = 0; x < GRID_SIZE_X; x++) {
			grid[x][y] = newGrid[x][y];
		}
	}

	// Free memory (deallocate heap memory)
	for (int x = 0; x < GRID_SIZE_X; x++) {
		delete[] newGrid[x];
	}
	delete[] newGrid;

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
