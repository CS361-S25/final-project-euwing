#ifndef WORLD_H
#define WORLD_H

#include "Organism.h"
#include "emp/math/Random.hpp"
#include <vector>
#include <algorithm>
#include <numeric>
#include <iostream>
#include <functional>
#include <cmath>

// A patch is one square of the world. Each patch has resources and may contain an organism.
struct Patch {
    std::vector<Organism*> occupants; // Usually either 0 or 1 organism
    double resource_level = 1.0;      // How much food is in the patch
    double danger_level = 0.0;        // Not used here, but could be used for threats
};

class World {
private:
    std::vector<Patch> patches;          // All patches in the world
    emp::Random random;                  // Random number generator
    double mutation_rate = 0.05;         // How often traits mutate during reproduction
    double mutation_sd = 0.025;          // Standard deviation of mutation changes
    double predator_death_rate = 0.0001; // Chance each predator dies per step
    std::function<Organism*(bool, double, double, double)> clone_func; // How to create new organisms

public:
    // Create a world with a given number of patches
    World(int num_patches) : patches(num_patches) {}

    // Set how organisms are cloned (used to pick the right type: Prey, Prey2, Predator)
    void SetCloneFunction(std::function<Organism*(bool, double, double, double)> func) {
        clone_func = func;
    }

    // Set how likely predators are to die each turn
    void SetPredatorDeathRate(double rate) {
        predator_death_rate = rate;
    }

    // Add an organism to a patch (only if the patch is empty)
    void AddOrganism(Organism* org, int patch_index) {
        if (patches[patch_index].occupants.empty()) {
            patches[patch_index].occupants.push_back(org);
        } else {
            delete org; // If patch is full, discard the organism
        }
    }

    // Do one full time step: move, reproduce, and remove dead organisms
    void Step() {
        MoveOrganisms();
        Reproduce();
        CullDead();
    }

    // Move all organisms to new patches (if they choose to move)
    void MoveOrganisms() {
        std::vector<std::vector<Organism*>> new_occupants(patches.size());

        for (size_t i = 0; i < patches.size(); ++i) {
            for (Organism* org : patches[i].occupants) {
                // Check if this organism decides to move
                if (!random.P(org->GetMoveRate())) {
                    if (new_occupants[i].empty()) new_occupants[i].push_back(org);
                    continue;
                }

                // Score each patch based on resource and danger
                std::vector<double> patch_scores(patches.size());
                for (size_t j = 0; j < patches.size(); ++j) {
                    double resource = patches[j].resource_level;
                    double danger;

                    if (org->IsPrey()) {
                        int predators = std::count_if(
                            patches[j].occupants.begin(), patches[j].occupants.end(),
                            [](Organism* o) { return !o->IsPrey(); });
                        danger = static_cast<double>(predators);
                    } else {
                        int predators = std::count_if(
                            patches[j].occupants.begin(), patches[j].occupants.end(),
                            [](Organism* o) { return !o->IsPrey(); });
                        int prey_count = patches[j].occupants.size() - predators;
                        resource = static_cast<double>(prey_count);
                        danger = static_cast<double>(predators);
                    }

                    double a = org->GetAlpha();
                    double t = org->GetTau();
                    patch_scores[j] = a * (t * resource - (1 - t) * danger);
                }

                // Choose where to go
                double total_score = std::accumulate(patch_scores.begin(), patch_scores.end(), 0.0);
                size_t chosen_patch = 0;

                if (total_score <= 0.0) {
                    // Move randomly if no patch looks good
                    chosen_patch = random.GetUInt(patches.size());
                } else {
                    // Normalize and pick based on probabilities
                    for (double& score : patch_scores) score /= total_score;
                    double r = random.GetDouble();
                    double running_total = 0.0;
                    for (size_t k = 0; k < patch_scores.size(); ++k) {
                        running_total += patch_scores[k];
                        if (r <= running_total) {
                            chosen_patch = k;
                            break;
                        }
                    }
                }

                // Move organism if destination is empty
                if (new_occupants[chosen_patch].empty()) {
                    new_occupants[chosen_patch].push_back(org);
                } else {
                    new_occupants[i].push_back(org); // Stay in place
                }
            }
        }

        // Apply new locations
        for (size_t i = 0; i < patches.size(); ++i) {
            patches[i].occupants = std::move(new_occupants[i]);
        }
    }

    // Handle reproduction for all organisms
    void Reproduce() {
        std::vector<std::pair<Organism*, int>> babies; // List of new organisms and their location

        for (size_t i = 0; i < patches.size(); ++i) {
            double resources = patches[i].resource_level;

            for (Organism* org : patches[i].occupants) {
                double chance = 0.25;

                if (org->IsPrey()) {
                    // Prey have more babies in better patches
                    chance *= resources;
                    int max_babies = (resources >= 0.66) ? 3 : (resources >= 0.33 ? 2 : 1);

                    for (int b = 0; b < max_babies; ++b) {
                        if (random.P(chance)) {
                            Organism* baby = org->Clone();

                            double a = baby->GetAlpha();
                            double t = baby->GetTau();
                            double m = baby->GetMoveRate();
                            bool is_prey = baby->IsPrey();

                            // Mutate traits
                            if (random.P(mutation_rate))
                                a = std::clamp(a + random.GetRandNormal(0, mutation_sd), 0.0, 1.0);
                            if (random.P(mutation_rate))
                                t = std::clamp(t + random.GetRandNormal(0, mutation_sd), 0.0, 1.0);

                            if (clone_func) {
                                delete baby;
                                baby = clone_func(is_prey, a, t, m);
                            }

                            babies.emplace_back(baby, i);
                        }
                    }
                } else {
                    // Predators just reproduce randomly
                    if (random.P(chance)) {
                        Organism* baby = org->Clone();

                        double a = baby->GetAlpha();
                        double t = baby->GetTau();
                        double m = baby->GetMoveRate();
                        bool is_prey = baby->IsPrey();

                        if (random.P(mutation_rate))
                            a = std::clamp(a + random.GetRandNormal(0, mutation_sd), 0.0, 1.0);
                        if (random.P(mutation_rate))
                            t = std::clamp(t + random.GetRandNormal(0, mutation_sd), 0.0, 1.0);

                        if (clone_func) {
                            delete baby;
                            baby = clone_func(is_prey, a, t, m);
                        }

                        babies.emplace_back(baby, i);
                    }
                }
            }
        }

        // Place babies in their parent’s patch if there's room
        for (auto& [baby, index] : babies) {
            if (patches[index].occupants.empty()) {
                patches[index].occupants.push_back(baby);
            } else {
                delete baby;
            }
        }
    }

    // Remove dead organisms
    void CullDead() {
        for (auto& patch : patches) {
            patch.occupants.erase(std::remove_if(
                patch.occupants.begin(), patch.occupants.end(),
                [&](Organism* org) {
                    if (!org->IsPrey() && random.P(predator_death_rate)) {
                        delete org;
                        return true;
                    }
                    if (org->IsDead()) {
                        delete org;
                        return true;
                    }
                    return false;
                }),
                patch.occupants.end());
        }
    }

    // Return all patches (read-only)
    const std::vector<Patch>& GetPatches() const { return patches; }

    // Return all patches (editable)
    std::vector<Patch>& GetPatchesMutable() { return patches; }

    // Helper functions to compute averages (not needed for core simulation)
    double GetAverageAlpha(bool for_prey = true) const {
        std::vector<double> values;
        for (const auto& patch : patches)
            for (Organism* org : patch.occupants)
                if (org->IsPrey() == for_prey) values.push_back(org->GetAlpha());
        return values.empty() ? 0.0 : std::accumulate(values.begin(), values.end(), 0.0) / values.size();
    }

    double GetAverageTau(bool for_prey = true) const {
        std::vector<double> values;
        for (const auto& patch : patches)
            for (Organism* org : patch.occupants)
                if (org->IsPrey() == for_prey) values.push_back(org->GetTau());
        return values.empty() ? 0.0 : std::accumulate(values.begin(), values.end(), 0.0) / values.size();
    }
};

#endif














