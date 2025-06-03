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

struct Patch {
    std::vector<Organism*> occupants;
    double resource_level = 1.0;
    double danger_level = 0.0;
};

class World {
private:
    std::vector<Patch> patches;
    emp::Random random;
    double mutation_rate = 0.05;
    double mutation_sd = 0.025;
    double predator_death_rate = 0.1;
    std::function<Organism*(bool, double, double, double)> clone_func;

public:
    World(int num_patches) : patches(num_patches) {}

    void SetCloneFunction(std::function<Organism*(bool, double, double, double)> func) {
        clone_func = func;
    }

    void SetPredatorDeathRate(double rate) {
        predator_death_rate = rate;
    }

    void AddOrganism(Organism* org, int patch_index) {
        patches[patch_index].occupants.push_back(org);
    }

    void Step() {
        MoveOrganisms();
        Reproduce();
        CullDead();
    }

    void MoveOrganisms() {
        std::vector<std::vector<Organism*>> new_occupants(patches.size());

        for (size_t i = 0; i < patches.size(); ++i) {
            for (Organism* org : patches[i].occupants) {
                if (!random.P(org->GetMoveRate())) {
                    new_occupants[i].push_back(org);
                    continue;
                }

                std::vector<double> patch_scores(patches.size());

                for (size_t j = 0; j < patches.size(); ++j) {
                    double resource = patches[j].resource_level;
                    double danger;

                    if (org->IsPrey()) {
                        int predator_count = std::count_if(
                            patches[j].occupants.begin(), patches[j].occupants.end(),
                            [](Organism* o) { return !o->IsPrey(); });
                        danger = static_cast<double>(predator_count);
                    } else {
                        int predator_count = std::count_if(
                            patches[j].occupants.begin(), patches[j].occupants.end(),
                            [](Organism* o) { return !o->IsPrey(); });
                        int prey_count = patches[j].occupants.size() - predator_count;
                        resource = static_cast<double>(prey_count);
                        danger = static_cast<double>(predator_count);
                    }

                    double alpha = org->GetAlpha();
                    double tau = org->GetTau();
                    patch_scores[j] = alpha * (tau * resource - (1 - tau) * danger);
                }

                double total_score = std::accumulate(patch_scores.begin(), patch_scores.end(), 0.0);
                size_t chosen_patch = 0;

                if (total_score <= 0.0) {
                    chosen_patch = random.GetUInt(patches.size());
                } else {
                    for (double& score : patch_scores) score /= total_score;
                    double rand_val = random.GetDouble();
                    double cumulative = 0.0;
                    for (size_t k = 0; k < patch_scores.size(); ++k) {
                        cumulative += patch_scores[k];
                        if (rand_val <= cumulative) {
                            chosen_patch = k;
                            break;
                        }
                    }
                }

                new_occupants[chosen_patch].push_back(org);
            }
        }

        for (size_t i = 0; i < patches.size(); ++i) {
            patches[i].occupants = std::move(new_occupants[i]);
        }
    }

    void Reproduce() {
        std::vector<std::pair<Organism*, int>> babies;

        for (size_t i = 0; i < patches.size(); ++i) {
            double local_resource = patches[i].resource_level;

            for (Organism* org : patches[i].occupants) {
                double reproduction_chance = 0.25;

                if (org->IsPrey()) {
                    reproduction_chance *= local_resource;

                    int max_babies = 1;
                    if (local_resource >= 0.66) max_babies = 3;
                    else if (local_resource >= 0.33) max_babies = 2;

                    for (int k = 0; k < max_babies; ++k) {
                        if (random.P(reproduction_chance)) {
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
                } else {
                    if (random.P(reproduction_chance)) {
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

        for (auto& [baby, index] : babies) {
            patches[index].occupants.push_back(baby);
        }
    }

    void CullDead() {
        for (auto& patch : patches) {
            patch.occupants.erase(std::remove_if(
                patch.occupants.begin(), patch.occupants.end(),
                [&](Organism* org) {
                    // Random death for predators
                    if (!org->IsPrey() && random.P(predator_death_rate)) {
                        delete org;
                        return true;
                    }

                    // General check for IsDead() (future extensibility)
                    if (org->IsDead()) {
                        delete org;
                        return true;
                    }

                    return false;
                }),
                patch.occupants.end());
        }
    }

    const std::vector<Patch>& GetPatches() const { return patches; }
    std::vector<Patch>& GetPatchesMutable() { return patches; }

    double GetAverageAlpha(bool for_prey = true) const {
        std::vector<double> alphas;
        for (const auto& patch : patches)
            for (Organism* org : patch.occupants)
                if (org->IsPrey() == for_prey) alphas.push_back(org->GetAlpha());
        return alphas.empty() ? 0.0 : std::accumulate(alphas.begin(), alphas.end(), 0.0) / alphas.size();
    }

    double GetAverageTau(bool for_prey = true) const {
        std::vector<double> taus;
        for (const auto& patch : patches)
            for (Organism* org : patch.occupants)
                if (org->IsPrey() == for_prey) taus.push_back(org->GetTau());
        return taus.empty() ? 0.0 : std::accumulate(taus.begin(), taus.end(), 0.0) / taus.size();
    }
};

#endif












