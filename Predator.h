#ifndef PREDATOR_H
#define PREDATOR_H

#include "Organism.h"

class Predator : public Organism {
public:
    Predator(double a, double t, double m)
        : Organism(a, t, m) {}

    bool IsPrey() const override { return false; }

    void SetAlpha(double new_alpha) override {
        alpha = new_alpha;
    }

    Organism* Clone() const override {
        return new Predator(alpha, tau, move_rate);
    }

    bool IsDead() const override {
        return false;
    }
};

#endif



