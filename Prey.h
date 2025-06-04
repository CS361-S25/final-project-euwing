#ifndef PREY_H
#define PREY_H

#include "Organism.h"

// This class represents the first type of prey (Prey 1)
// It inherits from the Organism base class
class Prey : public Organism {
public:
    // Constructor: initializes the prey's traits
    Prey(double a, double t, double m)
        : Organism(a, t, m) {}

    // This function tells us that this is a prey (not a predator)
    bool IsPrey() const override { return true; }

    // This lets us change the alpha trait later (evolution or mutation)
    void SetAlpha(double new_alpha) override {
        alpha = new_alpha;
    }

    // This makes a new prey with the same traits (used for reproduction)
    Organism* Clone() const override {
        return new Prey(alpha, tau, move_rate);
    }

    // Prey never die on their own — only predators can remove them
    bool IsDead() const override {
        return false;
    }

    // This tells the world how likely this prey is to move
    double GetMoveRate() const override {
        return move_rate;
    }
};

#endif



