#include <gtest/gtest.h>
#include <environmentalselectionoperator.h>

PartialSolution make_ps(delay a, delay b) {
    PartialSolution ps (
    {}, {});

    ps.setAverageProductivity(a);
    ps.setMakespanLastScheduledJob(b);
    return ps;
}

TEST(environmental_selector, no_reduction) {
    EnvironmentalSelectionOperator ev(10);
    ASSERT_TRUE(ev.reduce({}).empty());

    ASSERT_EQ(ev.reduce({make_ps(0,0), make_ps(0,1)}).size(), 2u);

}

TEST(environmental_selector, DISABLED_reduction) {
    EnvironmentalSelectionOperator ev(2);
    ASSERT_EQ(ev.reduce({make_ps(0,0), make_ps(0,1), make_ps(0,2)}).size(), 2u);

    ASSERT_EQ(ev.reduce({make_ps(0,0), make_ps(0,1), make_ps(0,2), make_ps(0,3)}).size(), 2u);
}

