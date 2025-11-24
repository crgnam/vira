#include <gtest/gtest.h>
#include <cmath>

#include "vira/units/units.hpp"

// Constants for conversion
constexpr double MILE_TO_METER = 1609.344;
constexpr double EPSILON = 1e-6;  // For floating point comparisons

// Test fixture for unit conversions
class UnitConversionTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Your setup code
    }

    void TearDown() override {
        // Your teardown code
    }

    // Rule of five: explicitly delete or define these functions
    UnitConversionTest() = default;
    ~UnitConversionTest() override = default;

    // Delete special member functions to prevent copying/moving
    UnitConversionTest(const UnitConversionTest&) = delete;
    UnitConversionTest& operator=(const UnitConversionTest&) = delete;
    UnitConversionTest(UnitConversionTest&&) = delete;
    UnitConversionTest& operator=(UnitConversionTest&&) = delete;
};

// Test that Mile to Meter conversion works correctly
TEST_F(UnitConversionTest, MileToMeterConversion) {
    // Assuming you have a class like:
    // Length<Mile> mile(1.0);
    // auto meters = mile.convertTo<Meter>();

    // Replace with your actual implementation:
    vira::units::Mile miles = 1.0;
    double expected_meters = miles.getValue() * MILE_TO_METER;

    // Replace with your actual conversion:
    vira::units::Meter actual_meters = miles; // This should use your conversion mechanism

    EXPECT_NEAR(actual_meters.getValue(), expected_meters, EPSILON);
}