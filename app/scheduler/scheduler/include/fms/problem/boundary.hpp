//
// Created by jmarc on 11/6/2021.
//

#ifndef FMS_PROBLEM_BOUNDARY_HPP
#define FMS_PROBLEM_BOUNDARY_HPP

#include "fms/delay.hpp"
#include "fms/math/interval.hpp"

namespace fms::problem {
using TimeInterval = math::Interval<delay>;

class BoundaryTranslationError : public std::logic_error {
public:
    explicit BoundaryTranslationError(const char *msg) : std::logic_error(msg) {}
};

/**
 * @brief Represents a boundary between two modules of a modular flow-shop.
 *
 * It is assumed that jobs travel from the last operation of the source module (output) to the first
 * operation of the destination module (input) and that the first job travels before the second job.
 */
class Boundary {
public:
    /**
     * @brief Build a new boundary definition
     * @param siSrcFstDstFst Sequence-independent setup time between the last operation of the
     *                       first job in the source module and the first operation of the first
     *                       job in the destination module. It must include the processing time.
     * @param siSrcSndDstSnd Sequence-independent setup time between the last operation of the
     *                       second job in the source module and the first operation of the second
     *                       job in the destination module. It must include the processing time.
     * @param dDstFstSrcFst Deadline between the first operation of the first job in the destination
     *                      module and the last operation of the first job in the source module.
     * @param dDstSndSrcSnd Deadline between the first operation of the second job in the
     *                      destination module and the last operation of the second job in the
     *                      source module.
     */
    Boundary(delay siSrcFstDstFst,
             delay siSrcSndDstSnd,
             std::optional<delay> dDstFstSrcFst,
             std::optional<delay> dDstSndSrcSnd);

    /**
     * @brief Translates a TimeInterval between two jobs of the source module to a TimeInterval
     *        of the same two jobs on the destination module.
     * @param value
     * @return
     */
    [[nodiscard]] TimeInterval translateToDestination(const TimeInterval &value) const;

    /**
     * @brief Translates a TimeInterval between two jobs of the destination module to a TimeInterval
     *        of the same two jobs on the source module.
     * @param value
     * @return
     */
    [[nodiscard]] TimeInterval translateToSource(const TimeInterval &value) const;
    
private:
    /**
     * @brief Value used to translate from the infimum of the source bound to the infimum of the
     *        destination bound and also to translate from the supremum of the destination bound to
     *        the supremum of the source bound.
     *
     * It is equivalent to @f$si(lst_x(j_{u+1}), fst_y(j_{u+1})) - dl(fst_y(j_u),lst_x(j_u))@f$
     */
    std::optional<delay> m_tISSD;
    
    /**
     * @brief Value used to translate from the supremum of the source bound to the supremum of
     *        the destination bound and also to translate from the infimum of the destination bound
     *        to the infimum of the source bound.
     *
     * It is equivalent to @f$-si(lst_x(j_{u}), fst_y(j_{u})) + dl(fst_y(j_{u+1}),lst_x(j_{u+1}))@f$
     */
    std::optional<delay> m_tSSID;
};
} // namespace fms::problem

#endif // FMS_PROBLEM_BOUNDARY_HPP
