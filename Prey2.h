#ifndef PREY2_H
#define PREY2_H

#include "Organism.h"

// This class represents the second type of prey (Prey 2)
// Like Prey, it also inherits from the Organism base class
class Prey2 : public Organism {
public:
    // Constructor: sets up Prey2 with its traits
    Prey2(double a, double t, double m)
        : Organism(a, t, m) {}

    // This tells the program this is a prey (not a predator)
    bool IsPrey() const override { return true; }

    // Change alpha value (used during mutation or evolution)
    void SetAlpha(double new_alpha) override {
        alpha = new_alpha;
    }

    // Create a new Prey2 with the same traits (for reproduction)
    Organism* Clone() const override {
        return new Prey2(alpha, tau, move_rate);
    }

    // Prey2 also doesn’t die randomly — it stays alive unless eaten
    bool IsDead() const override {
        return false;
    }
};

#endif

