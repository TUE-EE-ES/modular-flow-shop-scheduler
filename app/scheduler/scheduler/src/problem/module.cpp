//
// Created by 20204729 on 05/05/2021.
//
#include "fms/pch/containers.hpp"
#include "fms/pch/fmt.hpp"

#include "fms/problem/module.hpp"

#include "fms/problem/operation.hpp"

using namespace fms;
using namespace fms::problem;

void problem::Module::addInputBounds(const IntervalSpec &intervals) {
    for (const auto &[jobFstId, jobIntervals] : intervals) {
        for (const auto &[jobSndId, interval] : jobIntervals) {
            const auto &op = jobs(jobFstId).front();
            const auto &opNext = jobs(jobSndId).front();
            addInterval(op, opNext, interval);
        }
    }
}

void problem::Module::addOutputBounds(const IntervalSpec &intervals) {
    for (const auto &[jobFstId, jobIntervals] : intervals) {
        for (const auto &[jobSndId, interval] : jobIntervals) {
            const auto &op = jobs(jobFstId).back();
            const auto &opNext = jobs(jobSndId).back();
            addInterval(op, opNext, interval);
        }
    }
}

void problem::Module::addInterval(const Operation &from,
                                  const Operation &to,
                                  const TimeInterval &value) {

    if (value.min()) {
        // Because we are adding the interval in the form of a setup time, we need to subtract the
        // processing time as it will be added later on.
        const auto &processingTime = getProcessingTime(from);
        addExtraSetupTime(from, to, value.minValue() - processingTime);
    }

    if (value.max()) {
        addExtraDueDate(to, from, value.maxValue());
    }
}
