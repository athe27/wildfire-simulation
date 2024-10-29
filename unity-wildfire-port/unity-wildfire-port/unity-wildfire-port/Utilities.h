#include <cstdint>

class Vector4
{
public:
	Vector4();
	Vector4(float inX, float inY, float inZ, float inW);
	~Vector4() {}

	float X;
	float Y;
	float Z;
	float W;

private:

};

Vector4::Vector4()
{
	this->X = 0.f;
	this->Y = 0.f;
	this->Z = 0.f;
	this->W = 0.f;
}

Vector4::Vector4(float inX, float inY, float inZ, float inW) {
	this->X = inX;
	this->Y = inY;
	this->Z = inZ;
	this->W = inW;
}

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