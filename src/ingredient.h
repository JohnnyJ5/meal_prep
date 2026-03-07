#ifndef INGREDIENT_H
#define INGREDIENT_H

#include "measurement.h"
#include <ostream>
#include <stdexcept>
#include <string>

/**
 * @brief Represents an ingredient required for a meal.
 *
 * Contains the name, amount (measurement), and optional preparation
 * instructions.
 */
class Ingredient {
public:
  // Default constructor
  Ingredient() = default;

  /**
   * @brief Constructs an Ingredient.
   * @param name The name of the ingredient.
   * @param amount The measurement of the ingredient.
   * @param preparation Optional preparation instructions (default is "None").
   */
  Ingredient(const std::string &name, Measurement amount,
             const std::string &preparation = "None")
      : d_name(name), d_measurement(amount), d_preparation(preparation) {}

  // Copy constructor
  Ingredient(const Ingredient &other) = default;

  // Move constructor
  Ingredient(Ingredient &&other) noexcept = default;

  // Copy assignment
  Ingredient &operator=(const Ingredient &other) = default;

  // Move assignment
  Ingredient &operator=(Ingredient &&other) noexcept = default;

  std::string getName() const { return d_name; }
  Measurement getAmount() const { return d_measurement; }
  std::string getPreparation() const { return d_preparation; }

  /**
   * @brief Adds the amount of another ingredient of the same type.
   * @param other The ingredient to add.
   * @return A new Ingredient with the combined amount.
   * @throws std::invalid_argument if the ingredient names do not match.
   */
  Ingredient operator+(const Ingredient &other) const {
    if (d_name != other.d_name) {
      throw std::invalid_argument("Cannot add Ingredients of different types");
    }
    return Ingredient(d_name, d_measurement + other.d_measurement);
  }

  // In-place addition
  Ingredient &operator+=(const Ingredient &other) {
    *this = *this + other;
    return *this;
  }

  bool operator==(const Ingredient &other) const {
    return d_name == other.d_name;
  }

  // For use in std::set - compare by name first, then by amount
  bool operator<(const Ingredient &other) const {
    return d_name < other.d_name;
  }

private:
  std::string d_name;
  Measurement d_measurement;
  std::string d_preparation;
};

inline std::ostream &operator<<(std::ostream &os,
                                const Ingredient &Ingredient) {
  os << Ingredient.getName() << ": " << Ingredient.getAmount();
  return os;
}

#endif // Ingredient_H