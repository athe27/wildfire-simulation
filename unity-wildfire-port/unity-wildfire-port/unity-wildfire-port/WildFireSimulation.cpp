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
#include <cmath>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include "wildfireSimulation.h"

const std::vector<std::string> WildFireSimulation::windDirections = {
	"calm", "S", "N", "W", "E", "SW", "SE", "NW", "NE"
};

void WildFireSimulation::LoadHeightMapFromImage(const char* filename) {
	int width, height, channels;
	unsigned char* image_data = stbi_load(filename, &width, &height, &channels, STBI_grey);

	if (!image_data) {
		std::cerr << "Failed to load heightmap image: " << filename << std::endl;
		return;
	}

	// Ensure the image dimensions match or can be scaled to our grid size
	for (int y = 0; y < GRID_SIZE_Y; y++) {
		for (int x = 0; x < GRID_SIZE_X; x++) {
			// Calculate the corresponding pixel coordinates in the image
			int img_x = (x * width) / GRID_SIZE_X;
			int img_y = (y * height) / GRID_SIZE_Y;

			// Get the pixel value (0-255) and normalize it to 0-1
			float heightValue = image_data[img_y * width + img_x] / 255.0f;

			// Set the cell height (assuming max height of 10.0f to match your existing scale)
			grid[x][y].CellHeight = heightValue;
		}
	}

	// Free the image data
	stbi_image_free(image_data);
}

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

float PerlinNoise(float x, float y) {
	float frequency = 0.3f;
	return 0.5f * (sin(x * frequency) + cos(y * frequency)); // Simple noise function
}

void WildFireSimulation::InitializeWildFireFimulation()
{
	// load cell height map
	LoadHeightMapFromImage("HeightMaps/HeightMap1.png");

	// Initialize the grid with default values.
	for (int index_Y = 0; index_Y < GRID_SIZE_Y; index_Y++) {
		for (int index_X = 0; index_X < GRID_SIZE_X; index_X++) {
			// Set the ID to be a unique value for this specific grid cell.
			grid[index_X][index_Y].gridCellID = (index_Y * GRID_SIZE_Y) + index_X;
			grid[index_X][index_Y].CurrentState = EGridCellState::NOT_ON_FIRE;
			grid[index_X][index_Y].Material = (std::rand() % 2 == 0) ?
				EGridCellMaterial::GRASS : EGridCellMaterial::TREE;
			grid[index_X][index_Y].CellLocation.X = index_X;
			grid[index_X][index_Y].CellLocation.Y = index_Y;
		}
	}

	// Set the initial grid cell on fire.
	// grid[GRID_SIZE_X / 2][GRID_SIZE_Y / 2].CurrentState = EGridCellState::ON_FIRE;

	// Assign water and bedrock in clusters
	for (int index_Y = 0; index_Y < GRID_SIZE_Y; index_Y++) {
		for (int index_X = 0; index_X < GRID_SIZE_X; index_X++) {
			float bedrockProb = BASE_BEDROCK_PROB;
			float waterProb = BASE_WATER_PROB;

			// Step 3: Check neighbors for clustering effect
			if (HasNeighborWithMaterial(index_X, index_Y, EGridCellMaterial::BEDROCK)) {
				bedrockProb *= BEDROCK_CLUSTER_MULTIPLIER;
			}
			if (HasNeighborWithMaterial(index_X, index_Y, EGridCellMaterial::WATER)) {
				waterProb *= WATER_CLUSTER_MULTIPLIER;
			}

			// Random assignment based on probability
			float randValue = static_cast<float>(rand()) / RAND_MAX;
			if (randValue < bedrockProb) {
				grid[index_X][index_Y].Material = EGridCellMaterial::BEDROCK;
			}
			else if (randValue < waterProb) {
				grid[index_X][index_Y].Material = EGridCellMaterial::WATER;
			}
		}
	}

	currentTickCounter = 0;
	GenerateTemperatures();

	InitializeWindOffsets();
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
				switch (ThisGridCell.Material) {
				case EGridCellMaterial::GRASS: 
				{
					image_data[(Index_Y * GRID_SIZE_X + Index_X) * 3 + 0] = 0;    // Red channel
					image_data[(Index_Y * GRID_SIZE_X + Index_X) * 3 + 1] = 255 * ThisGridCell.CellHeight;  // Green channel
					image_data[(Index_Y * GRID_SIZE_X + Index_X) * 3 + 2] = 0;    // Blue channel
				}
				break;
				case EGridCellMaterial::TREE: 
				{
					image_data[(Index_Y * GRID_SIZE_X + Index_X) * 3 + 0] = 0;    // Red channel
					image_data[(Index_Y * GRID_SIZE_X + Index_X) * 3 + 1] = 150 * ThisGridCell.CellHeight;  // Green channel
					image_data[(Index_Y * GRID_SIZE_X + Index_X) * 3 + 2] = 0;    // Blue channel
				}
				break;
				case EGridCellMaterial::BEDROCK:
				{
					image_data[(Index_Y * GRID_SIZE_X + Index_X) * 3 + 0] = 100;    // Red channel
					image_data[(Index_Y * GRID_SIZE_X + Index_X) * 3 + 1] = 100;  // Green channel
					image_data[(Index_Y * GRID_SIZE_X + Index_X) * 3 + 2] = 100;    // Blue channel
					}
				break;
				case EGridCellMaterial::WATER:
				{
					image_data[(Index_Y * GRID_SIZE_X + Index_X) * 3 + 0] = 135;    // Red channel
					image_data[(Index_Y * GRID_SIZE_X + Index_X) * 3 + 1] = 206;  // Green channel
					image_data[(Index_Y * GRID_SIZE_X + Index_X) * 3 + 2] = 250;    // Blue channel
				}
				break;
				}
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
				image_data[(Index_Y * GRID_SIZE_X + Index_X) * 3 + 0] = 0;
				image_data[(Index_Y * GRID_SIZE_X + Index_X) * 3 + 1] = 0;
				image_data[(Index_Y * GRID_SIZE_X + Index_X) * 3 + 2] = 0;
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
			WildFireGridCell& ThisGridCell = grid[index_X][index_Y];

			// Check if this grid cell needs to be updated this tick.
			/*if ((ThisGridCell.gridCellID % 2) != (currentTickCounter % 2)) {
				continue;
			}*/

			switch (ThisGridCell.CurrentState) {
			case EGridCellState::NOT_ON_FIRE:
			{
				// Check if this cell should catch fire based on neighbors
				IntVector2D location = { index_X, index_Y };
				bool neighborOnFire = //FindOnFireNeighboringGridCells(location).size() > 0;
					CheckWindNeighbors(location);

				// If we have fire neighbors or random fire can start, check probability to catch fire
				float fireCatchProb = static_cast<float>(rand()) / RAND_MAX;

				if (neighborOnFire || FIRE_PROB > fireCatchProb) {
					float flammableProb = 0.f;
					switch (ThisGridCell.Material) {
					case EGridCellMaterial::GRASS:
						flammableProb = FLAMMABLE_PROBABILITY_FOR_GRASS;
						break;
					case EGridCellMaterial::TREE:
						flammableProb = FLAMMABLE_PROBABILITY_FOR_TREE;
						break;
					case EGridCellMaterial::WATER:
						flammableProb = FLAMMABLE_PROBABILITY_FOR_WATER;
						break;
					case EGridCellMaterial::BEDROCK:
						flammableProb = FLAMMABLE_PROBABILITY_FOR_BEDROCK;
						break;
					}
					// Rain probability
					float rainAmount = (static_cast<float>(rand()) / RAND_MAX) * 100;


					// Generate random probability between 0 and 1
					float randomProb = static_cast<float>(rand()) / RAND_MAX;
					if (isHot) {
						randomProb *= 2;
					}
					if (rainAmount < 24) {
						randomProb *= 0.018
					}
					else if (rainAmount > 96) {
						randomProb *= 0.061
					}
					 
					if (flammableProb > randomProb) {
						newGrid[index_X][index_Y].CurrentState = EGridCellState::ON_FIRE;
					}
				}
				// Possibility that a grass cell grows into a tree cell
				else if (ThisGridCell.Material == EGridCellMaterial::GRASS) {
					float regrowTreeProb = TREE_REGROW_PROBABILITY;
					float newTreeProb = static_cast<float>(rand()) / RAND_MAX;
					if (regrowTreeProb > newTreeProb) {
						newGrid[index_X][index_Y].Material = EGridCellMaterial::TREE;
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
				float regrowGrassProb = GRASS_REGROW_PROBABILITY;
				// Generate random probability to see if destroyed cell can turn into grass
				float newGrassProb = static_cast<float>(rand()) / RAND_MAX;
				if (regrowGrassProb > newGrassProb) {
					newGrid[index_X][index_Y].CurrentState = EGridCellState::NOT_ON_FIRE;
					newGrid[index_X][index_Y].Material = EGridCellMaterial::GRASS;
				}
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

bool WildFireSimulation::HasNeighborWithMaterial(int x, int y, EGridCellMaterial material)
{
	// Directions: left, right, up, down
	int dx[] = { -1, 1, 0, 0 };
	int dy[] = { 0, 0, -1, 1 };

	for (int i = 0; i < 4; i++) {
		int nx = x + dx[i];
		int ny = y + dy[i];

		// Ensure within grid bounds
		if (nx >= 0 && nx < GRID_SIZE_X && ny >= 0 && ny < GRID_SIZE_Y) {
			if (grid[nx][ny].Material == material) {
				return true;
			}
		}
	}
	return false;
}
