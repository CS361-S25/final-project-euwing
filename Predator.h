#ifndef PREDATOR_H
#define PREDATOR_H

#include "Organism.h"

// Predator organisms seek prey and may die if they don't reproduce.
class Predator : public Organism {
public:
    // Constructor: initializes predator with alpha, tau, and move_rate
    Predator(double a, double t, double m)
        : Organism(a, t, m) {}

    // Predators return false for IsPrey
    bool IsPrey() const override {
        return false;
    }

    // Update predator's sensitivity to patch score differences (alpha)
    void SetAlpha(double new_alpha) override {
        alpha = new_alpha;
    }

    // Make a new Predator with the same alpha, tau, and move_rate
    Organism* Clone() const override {
        return new Predator(alpha, tau, move_rate);
    }
};

#endif

