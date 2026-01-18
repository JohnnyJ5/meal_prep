#include <gtest/gtest.h>
#include "../measurement.h"

class MeasurementTest : public ::testing::Test {
protected:
    void SetUp() override {}
    void TearDown() override {}
};

// Test default constructor
TEST_F(MeasurementTest, DefaultConstructor) {
    Measurement m;
    // Default constructor creates an uninitialized measurement
    // Test that we can use it (e.g., assign to it)
    m = Measurement(5.0, MeasurementUnit::CUP);
    EXPECT_DOUBLE_EQ(m.getValue(), 5.0);
    EXPECT_EQ(m.getUnit(), MeasurementUnit::CUP);
}

// Test constructor with parameters
TEST_F(MeasurementTest, ConstructorWithParameters) {
    Measurement m(2.5, MeasurementUnit::CUP);
    EXPECT_EQ(m.getValue(), 2.5);
    EXPECT_EQ(m.getUnit(), MeasurementUnit::CUP);
}

// Test addition of same units
TEST_F(MeasurementTest, AdditionSameUnits) {
    Measurement m1(1.0, MeasurementUnit::CUP);
    Measurement m2(2.0, MeasurementUnit::CUP);
    Measurement result = m1 + m2;
    
    EXPECT_DOUBLE_EQ(result.getValue(), 3.0);
    EXPECT_EQ(result.getUnit(), MeasurementUnit::CUP);
}

// Test addition of different volume units (conversion)
TEST_F(MeasurementTest, AdditionDifferentVolumeUnits) {
    // 1 tablespoon = 3 teaspoons
    Measurement m1(1.0, MeasurementUnit::TABLESPOON);
    Measurement m2(3.0, MeasurementUnit::TEASPOON);
    Measurement result = m1 + m2;
    
    // Result should be in tablespoons (first unit)
    // 1 tbsp + 3 tsp = 1 tbsp + 1 tbsp = 2 tbsp
    EXPECT_NEAR(result.getValue(), 2.0, 0.001);
    EXPECT_EQ(result.getUnit(), MeasurementUnit::TABLESPOON);
}

// Test cup to teaspoon conversion
TEST_F(MeasurementTest, CupToTeaspoonConversion) {
    // 1 cup = 48 teaspoons
    Measurement m1(1.0, MeasurementUnit::CUP);
    Measurement m2(48.0, MeasurementUnit::TEASPOON);
    Measurement result = m1 + m2;
    
    EXPECT_NEAR(result.getValue(), 2.0, 0.001);
    EXPECT_EQ(result.getUnit(), MeasurementUnit::CUP);
}

// Test ounce conversion
TEST_F(MeasurementTest, OunceConversion) {
    // 1 fl oz = 6 tsp
    Measurement m1(1.0, MeasurementUnit::OUNCE);
    Measurement m2(6.0, MeasurementUnit::TEASPOON);
    Measurement result = m1 + m2;
    
    EXPECT_NEAR(result.getValue(), 2.0, 0.001);
    EXPECT_EQ(result.getUnit(), MeasurementUnit::OUNCE);
}

// Test non-volume units (should not convert)
TEST_F(MeasurementTest, NonVolumeUnitsNoConversion) {
    Measurement m1(1.0, MeasurementUnit::POUND);
    Measurement m2(2.0, MeasurementUnit::GRAM);
    Measurement result = m1 + m2;
    
    // Should add values as-is (no conversion)
    EXPECT_NEAR(result.getValue(), 3.0, 0.001);
    EXPECT_EQ(result.getUnit(), MeasurementUnit::POUND);
}

// Test whole units
TEST_F(MeasurementTest, WholeUnits) {
    Measurement m1(2.0, MeasurementUnit::WHOLE);
    Measurement m2(3.0, MeasurementUnit::WHOLE);
    Measurement result = m1 + m2;
    
    EXPECT_DOUBLE_EQ(result.getValue(), 5.0);
    EXPECT_EQ(result.getUnit(), MeasurementUnit::WHOLE);
}

// Test copy constructor
TEST_F(MeasurementTest, CopyConstructor) {
    Measurement m1(5.0, MeasurementUnit::CUP);
    Measurement m2(m1);
    
    EXPECT_DOUBLE_EQ(m2.getValue(), 5.0);
    EXPECT_EQ(m2.getUnit(), MeasurementUnit::CUP);
}

// Test assignment operator
TEST_F(MeasurementTest, AssignmentOperator) {
    Measurement m1(3.0, MeasurementUnit::TABLESPOON);
    Measurement m2;
    m2 = m1;
    
    EXPECT_DOUBLE_EQ(m2.getValue(), 3.0);
    EXPECT_EQ(m2.getUnit(), MeasurementUnit::TABLESPOON);
}
