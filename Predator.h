#ifndef PREDATOR_H
#define PREDATOR_H

#include "Organism.h"

// This class represents a predator in the simulation.
// It inherits from the Organism base class.
class Predator : public Organism {
public:
    // Constructor: sets the predator's traits
    Predator(double a, double t, double m)
        : Organism(a, t, m) {}

    // This returns false because this is a predator (not prey)
    bool IsPrey() const override { return false; }

    // Change the alpha trait (e.g., through evolution)
    void SetAlpha(double new_alpha) override {
        alpha = new_alpha;
    }

    // Make a new predator with the same traits (used for reproduction)
    Organism* Clone() const override {
        return new Predator(alpha, tau, move_rate);
    }

    // Predators don't die from internal logic — death is handled in the world
    bool IsDead() const override {
        return false;
    }
};

#endif



