#ifndef PREY_H
#define PREY_H

#include "Organism.h"

class Prey : public Organism {
public:
    Prey(double a, double t, double m)
        : Organism(a, t, m) {}

    bool IsPrey() const override { return true; }

    void SetAlpha(double new_alpha) override {
        alpha = new_alpha;
    }

    Organism* Clone() const override {
        return new Prey(alpha, tau, move_rate);
    }

    bool IsDead() const override {
        return false;
    }

    double GetMoveRate() const override {
        return move_rate;
    }
};

#endif




