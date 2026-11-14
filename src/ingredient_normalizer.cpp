#include "ingredient_normalizer.h"

#include <algorithm>
#include <cctype>
#include <sstream>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

namespace {

std::string toLowerAscii(const std::string &str) {
    std::string out = str;
    std::transform(out.begin(), out.end(), out.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return out;
}

std::string capitalizeHyphenatedWord(const std::string &word) {
    std::string out;
    out.reserve(word.size());
    bool capNext = true;
    for (unsigned char c : word) {
        if (c == '-') {
            out.push_back('-');
            capNext = true;
        } else if (capNext) {
            out.push_back(static_cast<char>(std::toupper(c)));
            capNext = false;
        } else {
            out.push_back(static_cast<char>(std::tolower(c)));
        }
    }
    return out;
}

const std::unordered_map<std::string, std::pair<std::string, std::string>> &getCanonicalMap() {
    static const std::unordered_map<std::string, std::pair<std::string, std::string>> kMap = {
        // Typos, spelling & abbreviations
        {"low sodumn chicken broth", {"Reduced Sodium Chicken Broth", ""}},
        {"cornstarch", {"Corn Starch", ""}},
        {"mayo", {"Mayonnaise", ""}},

        // Chicken
        {"boneless skinless chicken breast", {"Chicken Breast", ""}},

        // Salt
        {"salt", {"Salt", ""}},
        {"kosher salt", {"Salt", ""}},
        {"sea salt", {"Salt", ""}},

        // Pepper
        {"black pepper", {"Pepper", ""}},
        {"ground black pepper", {"Pepper", ""}},

        // Dairy
        {"unsalted butter", {"Butter", ""}},
        {"unsweetened almond milk", {"Almond Milk", ""}},
        {"heavy whipping cream", {"Heavy Cream", ""}},
        {"whole milk cottage cheese", {"Cottage Cheese", ""}},
        {"plain whole milk cottage cheese", {"Cottage Cheese", ""}},
        {"grated parmesan cheese", {"Parmesan Cheese", "Grated"}},

        // Eggs
        {"egg", {"Eggs", ""}},
        {"eggs", {"Eggs", ""}},
        {"large egg", {"Eggs", ""}},
        {"large eggs", {"Eggs", ""}},

        // Ground beef / flour / pantry / herbs / produce
        {"80% lean ground beef", {"Ground Beef", ""}},
        {"plain flour", {"All-Purpose Flour", ""}},
        {"mini chocolate chips", {"Chocolate Chips", ""}},
        {"natural peanut butter", {"Peanut Butter", ""}},
        {"ground flaxseed", {"Flax Seed", "Ground"}},
        {"ground cinnamon", {"Cinnamon", ""}},
        {"dried oregano", {"Oregano", "Dried"}},
        {"fresh basil leaves", {"Fresh Basil", ""}},
        {"fresh italian flat leaf parsley", {"Parsley", "Fresh"}},
        {"garlic cloves", {"Garlic", ""}},
        {"fresh blueberries", {"Blueberries", ""}},
        {"banana", {"Banana", ""}},
        {"mashed banana", {"Banana", "Mashed"}},
        {"boiling water", {"Water", "Boiling"}},
        {"water", {"Water", ""}},
    };
    return kMap;
}

}  // namespace

std::string collapseAndTrimWhitespace(const std::string &str) {
    std::string out;
    out.reserve(str.size());
    bool inSpace = true;  // true initially to skip leading whitespace
    for (unsigned char c : str) {
        if (std::isspace(c)) {
            if (!inSpace) {
                out.push_back(' ');
                inSpace = true;
            }
        } else {
            out.push_back(static_cast<char>(c));
            inSpace = false;
        }
    }
    if (!out.empty() && out.back() == ' ') {
        out.pop_back();
    }
    return out;
}

std::string toTitleCaseIngredient(const std::string &str) {
    std::string collapsed = collapseAndTrimWhitespace(str);
    if (collapsed.empty()) return "";

    static const std::unordered_set<std::string> kMinorWords = {"of",  "and",  "in", "to",
                                                                "for", "with", "on", "at"};

    std::istringstream iss(collapsed);
    std::vector<std::string> words;
    std::string w;
    while (iss >> w) {
        words.push_back(w);
    }

    std::string result;
    for (size_t i = 0; i < words.size(); ++i) {
        if (i > 0) result.push_back(' ');
        std::string lowerWord = toLowerAscii(words[i]);
        if (i > 0 && i + 1 < words.size() && kMinorWords.count(lowerWord) > 0) {
            result += lowerWord;
        } else {
            result += capitalizeHyphenatedWord(words[i]);
        }
    }
    return result;
}

NormalizedIngredient normalizeIngredient(const std::string &rawName, const std::string &rawPrep) {
    std::string collapsedName = collapseAndTrimWhitespace(rawName);
    std::string collapsedPrep = collapseAndTrimWhitespace(rawPrep);
    if (collapsedPrep.empty()) {
        collapsedPrep = "None";
    }

    if (collapsedName.empty()) {
        return {"", collapsedPrep};
    }

    std::string key = toLowerAscii(collapsedName);
    const auto &canonicalMap = getCanonicalMap();
    auto it = canonicalMap.find(key);
    if (it != canonicalMap.end()) {
        std::string finalName = it->second.first;
        std::string finalPrep = collapsedPrep;
        if (!it->second.second.empty() && (collapsedPrep == "None" || collapsedPrep == "none")) {
            finalPrep = it->second.second;
        }
        return {finalName, finalPrep};
    }

    return {toTitleCaseIngredient(collapsedName), collapsedPrep};
}

std::string normalizeIngredientCategory(const std::string &rawCategory) {
    return toTitleCaseIngredient(rawCategory);
}
