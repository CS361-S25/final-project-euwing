#ifndef PREY2_H
#define PREY2_H

#include "Organism.h"

class Prey2 : public Organism {
public:
    Prey2(double a, double t, double m)
        : Organism(a, t, m) {}

    bool IsPrey() const override { return true; }

    void SetAlpha(double new_alpha) override {
        alpha = new_alpha;
    }

    Organism* Clone() const override {
        return new Prey2(alpha, tau, move_rate);
    }

    bool IsDead() const override {
        return false;
    }

    double GetMoveRate() const override {
        return 0.0; // Prey2 never moves
    }
};

#endif


