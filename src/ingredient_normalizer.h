#pragma once

#include <string>

struct NormalizedIngredient {
    std::string name;
    std::string preparation;
};

/**
 * @brief Collapses internal whitespace runs to a single space and trims leading/trailing
 * whitespace.
 */
std::string collapseAndTrimWhitespace(const std::string &str);

/**
 * @brief Formats a string in Title Case (handling hyphenated words like "All-Purpose"
 * and keeping minor internal connectors like "of" lowercase).
 */
std::string toTitleCaseIngredient(const std::string &str);

/**
 * @brief Normalizes an ingredient name (and optional preparation), resolving known
 * aliases/plurals/typos and applying Title Case.
 */
NormalizedIngredient normalizeIngredient(const std::string &rawName,
                                         const std::string &rawPrep = "None");

/**
 * @brief Normalizes an ingredient category string (trims, collapses whitespace, Title Cases).
 */
std::string normalizeIngredientCategory(const std::string &rawCategory);
