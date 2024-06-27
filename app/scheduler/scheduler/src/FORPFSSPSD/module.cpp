//
// Created by 20204729 on 05/05/2021.
//
#include "pch/containers.hpp"

#include "FORPFSSPSD/module.hpp"

#include "FORPFSSPSD/operation.h"

#include <fmt/core.h>

using namespace FORPFSSPSD;

void FORPFSSPSD::Module::addInputBounds(const IntervalSpec &intervals) {
    for (const auto &[jobFstId, jobIntervals] : intervals) {
        for (const auto &[jobSndId, interval] : jobIntervals) {
            const auto &op = getJobOperations(jobFstId).front();
            const auto &opNext = getJobOperations(jobSndId).front();
            addInterval(op, opNext, interval);
        }
    }
}

void FORPFSSPSD::Module::addOutputBounds(const IntervalSpec &intervals) {
    for (const auto &[jobFstId, jobIntervals] : intervals) {
        for (const auto &[jobSndId, interval] : jobIntervals) {
            const auto &op = getJobOperations(jobFstId).back();
            const auto &opNext = getJobOperations(jobSndId).back();
            addInterval(op, opNext, interval);
        }
    }
}

void FORPFSSPSD::Module::addInterval(const operation &from,
                                     const operation &to,
                                     const Math::Interval<delay> &value) {

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

