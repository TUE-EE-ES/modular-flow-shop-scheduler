//
// Created by jmarc on 11/6/2021.
//

#include "FORPFSSPSD/boundary.hpp"

#include <fmt/compile.h>

using namespace FORPFSSPSD;

Boundary::Boundary(delay siSrcFstDstFst,
                   delay siSrcSndDstSnd,
                   std::optional<delay> dDstFstSrcFst,
                   std::optional<delay> dDstSndSrcSnd) :
    // For the meaning of these variables and why we compute them like this see the paper about
    // modular scheduling
    m_tISSD(std::nullopt), m_tSSID(std::nullopt) {

    if (dDstFstSrcFst) {
        m_tISSD = siSrcSndDstSnd - *dDstFstSrcFst;
    }

    if (dDstSndSrcSnd) {
        m_tSSID = -siSrcFstDstFst + *dDstSndSrcSnd;
    }
}

TimeInterval Boundary::translateToDestination(const TimeInterval &value) const {
    try {
        return value + std::make_tuple(m_tISSD, m_tSSID);
    } catch (std::domain_error &) {
        throw BoundaryTranslationError("The translated interval is not valid");
    }
}

TimeInterval Boundary::translateToSource(const TimeInterval &value) const {
    const auto tSSID = m_tSSID ? std::make_optional(-*m_tSSID) : std::nullopt;
    const auto tISSD = m_tISSD ? std::make_optional(-*m_tISSD) : std::nullopt;

    try {
        return value + std::make_tuple(tSSID, tISSD);
    } catch (std::domain_error &) {
        throw BoundaryTranslationError("The translated interval is not valid");
    }
}
