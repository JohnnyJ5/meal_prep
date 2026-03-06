#ifndef MEASUREMENTS_H
#define MEASUREMENTS_H

#include <iostream>
#include <string>

enum class MeasurementUnit {
    GRAM,
    OUNCE,
    CUP,
    TABLESPOON,
    TEASPOON,
    POUND,
    WHOLE,
    HALF,
    SMALL,
    CLOVE,
    HEAD
};

namespace Preparation {
    const std::string k_WHOLE = "WHOLE";
    const std::string k_HALF = "HALF";
    const std::string k_CHOPPED = "CHOPPED";
    const std::string k_DICED = "DICED";
    const std::string k_STRIPS = "STRIPS";
    const std::string k_SHREDDED = "SHREDDED";
    const std::string k_MINCED = "MINCED";
    const std::string k_GRATED = "GRATED";
    const std::string k_THIN_SLICED = "THIN_SLICED";
    const std::string K_PEELED = "PEELED";
}

class Measurement {
public:
    
    Measurement() = default;
    Measurement(double v, MeasurementUnit u)
        : d_value(v), d_measurementUnit(u) 
    {}

    Measurement(const Measurement &) = default;
    Measurement(Measurement &&) = default;
    Measurement &operator=(const Measurement &) = default;
    Measurement &operator=(Measurement &&) = default;

    Measurement operator+(const Measurement& other) const {
        if (d_measurementUnit != other.d_measurementUnit) {
            // Convert both measurements to a common unit (teaspoons for volume)
            double thisValue = convertToTeaspoons(d_value, d_measurementUnit);
            double otherValue = convertToTeaspoons(other.d_value, other.d_measurementUnit);
            double resultValue = thisValue + otherValue;
            
            // Return result in the first measurement's unit
            return Measurement(convertFromTeaspoons(resultValue, d_measurementUnit), d_measurementUnit);
        }
        else {
            return Measurement(d_value + other.d_value, d_measurementUnit);
        }
    }

    double getValue() const { return d_value; }
    MeasurementUnit getUnit() const { return d_measurementUnit; }

private:
    // Convert any volume unit to teaspoons
    static double convertToTeaspoons(double value, MeasurementUnit unit) {
        switch (unit) {
            case MeasurementUnit::TABLESPOON:
                return value * 3.0;  // 1 tbsp = 3 tsp
            case MeasurementUnit::TEASPOON:
                return value;        // Already in teaspoons
            case MeasurementUnit::CUP:
                return value * 48.0; // 1 cup = 48 tsp
            case MeasurementUnit::OUNCE:
                return value * 6.0;  // 1 fl oz = 6 tsp (assuming fluid ounces)
            default:
                // For non-volume units, return as-is (no conversion)
                return value;
        }
    }
    
    // Convert from teaspoons back to target unit
    static double convertFromTeaspoons(double tspValue, MeasurementUnit targetUnit) {
        switch (targetUnit) {
            case MeasurementUnit::TABLESPOON:
                return tspValue / 3.0;  // tsp to tbsp
            case MeasurementUnit::TEASPOON:
                return tspValue;        // Already in teaspoons
            case MeasurementUnit::CUP:
                return tspValue / 48.0; // tsp to cups
            case MeasurementUnit::OUNCE:
                return tspValue / 6.0;  // tsp to fl oz
            default:
                // For non-volume units, return as-is
                return tspValue;
        }
    }

    double d_value;
    MeasurementUnit d_measurementUnit;
};

inline std::ostream & operator<<(std::ostream &os, const Measurement &m) {
    os << m.getValue() << " ";
    switch (m.getUnit()) {
        case MeasurementUnit::GRAM: os << "grams"; break;
        case MeasurementUnit::OUNCE: os << "ounces"; break;
        case MeasurementUnit::CUP: os << "cups"; break;
        case MeasurementUnit::TABLESPOON: os << "tablespoons"; break;
        case MeasurementUnit::TEASPOON: os << "teaspoons"; break;
        case MeasurementUnit::POUND: os << "pounds"; break;
        case MeasurementUnit::WHOLE: os << "whole"; break;
        case MeasurementUnit::HALF: os << "half"; break;
        case MeasurementUnit::SMALL: os << "small"; break;
        case MeasurementUnit::CLOVE: os << "cloves"; break;
        case MeasurementUnit::HEAD: os << "heads"; break;
        default: os << "unknown unit"; break;
    }
    return os;
}




#endif // FOOD_MEASUREMENTS_H