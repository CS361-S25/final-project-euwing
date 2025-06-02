#ifndef PREY_H
#define PREY_H

#include "Organism.h"

// Prey organisms seek high-resource, low-danger patches and reproduce quickly.
class Prey : public Organism {
public:
    // Constructor: initializes prey with alpha, tau, and move_rate
    Prey(double a, double t, double m)
        : Organism(a, t, m) {}

    // Prey returns true for IsPrey
    bool IsPrey() const override {
        return true;
    }

    // Update prey's sensitivity to patch score differences (alpha)
    void SetAlpha(double new_alpha) override {
        alpha = new_alpha;
    }

    // Make a new Prey with the same alpha, tau, and move_rate
    Organism* Clone() const override {
        return new Prey(alpha, tau, move_rate);
    }
};

#endif

