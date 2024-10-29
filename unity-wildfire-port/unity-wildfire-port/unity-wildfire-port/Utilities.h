#include <cstdint>

enum class EAdvectionType : uint8_t {
	NORMAL = 1,
	BFECC = 2,
	MACCORMACK = 3
};

namespace MathUtilities {
	static int ClosestPowerOfTwo(int value) {
		if (value < 1) {
			return 1;
		}

		int lowerPower = 1;
		int upperPower = 2;

		// Find the nearest lower and upper powers of two
		while (upperPower < value) {
			lowerPower = upperPower;
			upperPower <<= 1;  // Multiply by 2 (equivalent to upperPower *= 2)
		}

		// If the upper power is closer or equal, return it; otherwise, return the lower power
		return (value - lowerPower < upperPower - value) ? lowerPower : upperPower;
	}
}