#include "delayGraph/exportStrings.hpp"

namespace DelayGraph::export_utilities::tikz {

const char *const kPreamble = R"(
\documentclass{standalone}
\usepackage{tikz}
\usetikzlibrary{
    calc,
    decorations.pathreplacing,
    shapes, 
    fit, 
    intersections, 
    decorations, 
    positioning
}

\begin{document}
\begin{tikzpicture}[
    operation/.style={
        circle,
        draw=black,
        fill=white,
        inner sep=0pt, 
        minimum size=1.0em
    },
    cluster/.style={
        shape=ellipse,
        draw
    },
    every node/.style={
        node distance=3.5em
    },
    constraint/.style={
        -latex,
        bend left=10
    },
    setup/.style={
        constraint
    },
    ssetup/.style={
        setup,
        color=blue,
        draw=blue
    },
    deadline/.style={
        constraint,
        color=red, 
        draw=red, 
        dashed
    }
]
)";

const char *const kPrintNodes = R"(
% draw the jobs
\foreach \x[count=\xi from 0] in \jobs {
    \foreach \y[count=\yi from 0] in \operations {
        \node[operation, minimum size = 3em] (\x\y) at ($(10*\xi em,-30*\yi em)$) {\tiny ${\x,\y}$};
    }
}
)";

const char *const kEnding = R"(

\end{tikzpicture}
\end{document}
)";

} // namespace DelayGraph::export_utilities::tikz

