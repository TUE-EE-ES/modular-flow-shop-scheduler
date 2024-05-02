module FlowShops

import Base.isequal
import Base.isless

using DataStructures
using FilePaths

export JobId, OperationId, MachineId, ModuleId
export Operation, Machine, Job, FlowShop, TimeFunc
export ProductionLine, TransferPoint, TransferConstraints, ModulesTransferConstraints
export addop, readflowshop, getop, newmachine

struct JobId
    id::Int
end
isless(j1::JobId, j2::JobId) = isless(j1.id, j2.id)
isequal(j1::JobId, j2::JobId) = isequal(j1.id, j2.id)

struct OperationId
    id::Int
end

isless(o1::OperationId, o2::OperationId) = isless(o1.id, o2.id)
isequal(o1::OperationId, o2::OperationId) = isequal(o1.id, o2.id)

struct MachineId
    id::Int
end

isless(m1::MachineId, m2::MachineId) = isless(m1.id, m2.id)
isequal(m1::MachineId, m2::MachineId) = isequal(m1.id, m2.id)

struct ModuleId
    id::Int
end
isless(m1::ModuleId, m2::ModuleId) = isless(m1.id, m2.id)

abstract type AbstractOperation end

mutable struct MachineBase{T<:AbstractOperation}
    id::MachineId
    operations::Vector{T}

    MachineBase{T}(id::MachineId) where {T<:AbstractOperation} = new(id, Vector())
end

const FullOperationId = Tuple{JobId,OperationId}

struct Operation <: AbstractOperation
    id::FullOperationId
    time_processing::Int
    time_sequence_dependent::Dict{JobId,Int}
    size::Int
    machine::MachineBase{Operation}
end
isless(op1::Operation, op2::Operation) = isless(op1.id, op2.id)
isequal(op1::Operation, op2::Operation) = isequal(op1.id, op2.id)

const Machine = MachineBase{Operation}
const TimeFunc = Tuple{Operation,Operation}

newmachine(id::MachineId) = MachineBase{Operation}(id)

mutable struct Job
    id::JobId
    operations::SortedDict{OperationId,Operation}
    operations_per_machine::Dict{MachineId,SortedDict{OperationId,Operation}}
    Job(id::JobId) = new(
        id,
        SortedDict{OperationId,Operation}(),
        Dict{Machine,SortedDict{OperationId,Operation}}(),
    )
end

isless(j1::Job, j2::Job) = isless(j1.id, j2.id)

function addop(
    job::Job,
    opid::OperationId,
    time_processing::Int,
    time_sequence_dependent::Dict{JobId,Int},
    size::Int,
    machine::Machine,
)::Operation

    op = Operation((job.id, opid), time_processing, time_sequence_dependent, size, machine)
    job.operations[opid] = op
    get!(job.operations_per_machine, machine.id, SortedDict())[opid] = op
    push!(machine.operations, op)
    op
end

numops(job::Job, machineid::MachineId) = length(job.operations_per_machine[machineid])

mutable struct FlowShop
    extrainfo::Dict{String,Any}
    jobs::SortedDict{JobId,Job}

    # Machines are ordered by their index
    machines::SortedDict{MachineId,Machine}

    # Constraints
    times_si::Dict{TimeFunc,Int}
    deadlines::Dict{TimeFunc,Int}

    "File used to create this flow-shop"
    filepath::AbstractPath

    FlowShop(
        machines::SortedDict{MachineId,Machine} = SortedDict{MachineId,Machine}(),
        filepath = Path(),
    ) = new(
        Dict{String,Any}(),
        SortedDict{JobId,Job}(),
        machines,
        Dict{TimeFunc,Int}(),
        Dict{TimeFunc,Int}(),
        filepath,
    )
end

const TransferConstraints = Dict{JobId,Int}
struct TransferPoint
    times_si::TransferConstraints
    times_d::TransferConstraints
end

const ModulesTransferConstraints = Dict{Tuple{ModuleId,ModuleId},TransferPoint}
struct ProductionLine
    modules::Dict{ModuleId,FlowShop}
    transfer_set::ModulesTransferConstraints
    extrainfo::Dict{String,Any}

    "File used to create this production line"
    filepath::AbstractPath
end

getop(flowshop::FlowShop, jobid::JobId, opid::OperationId) =
    flowshop.jobs[jobid].operations[opid]

function _find_max_operations_per_machine(flowshop::FlowShop)::Dict{MachineId,Int}
    maxopspermachine = Dict{MachineId,Int}()
    for job in values(flowshop.jobs)
        for (machineid, ops) in job.operations_per_machine
            value = get(maxopspermachine, machineid, 0)
            maxopspermachine[machineid] = max(value, length(ops))
        end
    end
    maxopspermachine
end

_ind(value::Int) = "  "^value

import EzXML as XML

function readflowshop(filename::String)
    document = XML.readxml(filename)
    root = XML.root(document)
    _readflowvector(root)
end

function _readflowvector(root::XML.Node)
    flowMapping = Dict{OperationId,MachineId}()
    for component in XML.findall("//flowVector/component", root)
        opId = OperationId(parse(Int, component["index"]))
        machineId = MachineId(parse(Int, component["value"]))
        flowMapping[opId] = machineId
    end
    flowMapping
end

function _readJobs(root::XML.Node)

end

function toXML(flowshop::FlowShop, filename::String)
    f = open(filename, "w")
    toXML(f, flowshop)
    close(f)
end

function toXML(
    f::IO,
    flowshop::FlowShop,
    module_id::Union{ModuleId,Nothing} = nothing;
    indent::Int = 0,
)
    maxopspermachine = _find_max_operations_per_machine(flowshop)

    # Add the generator path as a comment
    println(f, """$(_ind(indent))<!-- Generated by $(flowshop.filepath) -->""")

    print(f, """$(_ind(indent))<SPInstance type="FORPFSSPSD" """)
    if module_id !== nothing
        print(f, """id="$(module_id.id)" """)
    end
    println(f, ">")

    _xml_jobs(f, flowshop, indent = indent + 1)
    _xml_flow_vector(f, flowshop, indent = indent + 1)
    _xml_processing_times(f, flowshop, indent = indent + 1)
    _xml_sizes(f, flowshop, indent = indent + 1)

    _xml_time_func(f, "setupTimesIndependent", "s", flowshop.times_si, indent = indent + 1)
    _xml_sd_times(f, flowshop, maxopspermachine; indent = indent + 1)
    _xml_time_func(
        f,
        "relativeDueDates",
        "d",
        flowshop.deadlines,
        nothing,
        indent = indent + 1,
    )
    println(f, "$(_ind(indent))</SPInstance>")
end

function toXML(line::ProductionLine, filename::String)
    f = open(filename, "w")

    # Add the generator path as a comment
    println(f, """<!-- Generated by $(line.filepath) -->""")
    println(f, """<SPInstance type="Modular">""")
    for module_id in keys(line.modules) |> collect |> sort
        toXML(f, line.modules[module_id], module_id, indent = 1)
    end

    _xml_modules_transfer(f, line.transfer_set, indent = 1)
    println(f, """</SPInstance>""")
    close(f)
end

function _xml_jobs(f::IO, flowshop::FlowShop; indent = 1)
    println(f, """$(_ind(indent))<jobs count="$(length(flowshop.jobs))">""")
    maxops = maximum(length ∘ (x -> x[2].operations), flowshop.jobs)
    println(f, """$(_ind(indent+1))<operations count="$maxops"/>""")
    println(f, """$(_ind(indent))</jobs>""")
end

function _xml_flow_vector(f::IO, flowshop::FlowShop; indent::Int = 1)
    println(f, "$(_ind(indent))<flowVector>")
    for (jobid, job) in flowshop.jobs
        for (opid, op) in job.operations
            machineid = op.machine.id
            println(
                f,
                """$(_ind(indent+1))<component job="$(jobid.id)" \
                    index="$(opid.id)" value="$(machineid.id)"/>""",
            )
        end
    end
    println(f, "$(_ind(indent))</flowVector>")
end

function _xml_processing_times(f::IO, flowshop::FlowShop; indent = 1)

    println(f, """$(_ind(indent))<processingTimes default="0">""")
    for (jobid, job) in flowshop.jobs
        for (opid, op) in job.operations
            if op.time_processing == 0
                continue
            end

            println(
                f,
                """$(_ind(indent+1))<p j="$(jobid.id)" op="$(opid.id)" \
                value="$(op.time_processing)"/>""",
            )
        end
    end
    println(f, "$(_ind(indent))</processingTimes>")
end

function _xml_sizes(f::IO, flowshop::FlowShop; indent = 1)

    println(f, """$(_ind(indent))<sizes default="0">""")
    for (jobid, job) in flowshop.jobs
        for (opid, op) in job.operations
            if op.size == 0
                continue
            end
            println(
                f,
                """$(_ind(indent+1))<z j="$(jobid.id)" op="$(opid.id)" \
                value="$(op.size)"/>""",
            )
        end
    end
    println(f, "$(_ind(indent))</sizes>")
end

function _xml_time_func(
    f::IO,
    nodename::String,
    nodesubname::String,
    func::Dict{TimeFunc,Int},
    default::Union{Int,Nothing} = 0;
    indent::Int = 1,
)
    if default === nothing
        println(f, """$(_ind(indent))<$nodename>""")
    else
        println(f, """$(_ind(indent))<$nodename default="$default">""")
    end

    func = func |> collect |> sort
    for ((op1, op2), v) in func
        if v == default
            continue
        end

        println(
            f,
            """$(_ind(indent+1))<$nodesubname j1="$(op1.id[1].id)" op1="$(op1.id[2].id)" \
            j2="$(op2.id[1].id)" op2="$(op2.id[2].id)" value="$v"/>""",
        )
    end
    println(f, "$(_ind(indent))</$nodename>")
end

"""Find all the sequence-dependent setup times of a flow-shop problem.

The sequence-dependent setup times of a job `j` can be as follows:

```
(j)────→(k)
 │     🡕 │
 │    ╱  │
 │   ╱   │
 │  ╱    │
 🡓 🡗     🡓
(l)────→(m)
```

Where the \$j \\rightarrow k\$ and \$l \\rightarrow m\$ are with the immediately next job 
and the next job with the same plexity as the current job. \$j \\rightarrow m\$, 
\$l \\rightarrow k\$ and \$k \\rightarrow l\$ can be with any job.
"""
function _xml_sd_times(
    f::IO,
    flowshop::FlowShop,
    maxmachinesops::Dict{MachineId,Int},
    default::Union{Int,Nothing} = 0;
    indent::Int = 1,
)
    if default === nothing
        println(f, """$(_ind(indent))<setupTimes>""")
    else
        println(f, """$(_ind(indent))<setupTimes default="$default">""")
    end

    joblist = flowshop.jobs |> values |> collect

    firstmachine = true
    for machineid in keys(flowshop.machines)
        if firstmachine
            firstmachine = false
            ordermachineid = nothing

            machines = flowshop.machines |> keys |> collect
            for machineido in machines[2:end]
                if maxmachinesops[machineido] > 1
                    ordermachineid = machineido
                    break
                end
            end
        else
            ordermachineid = machineid
        end

        maxmachineops = maxmachinesops[machineid]

        jobsmachineops = Dict{JobId,Vector{Operation}}()
        for job in joblist
            jobsmachineops[job.id] =
                job.operations_per_machine[machineid] |> values |> collect
        end

        for (jobi, job) in enumerate(joblist)
            machine_ops::Vector{Operation} = jobsmachineops[job.id]
            job_opsinmachine = length(machine_ops)

            for jobni = 1:(jobi-1)
                jobnext = joblist[jobni]
                op_pos = 1 + (maxmachineops - job_opsinmachine)
                for op in machine_ops
                    _xml_sd_with_job(
                        f,
                        maxmachineops,
                        jobi,
                        jobni,
                        jobnext,
                        op,
                        op_pos,
                        jobsmachineops[jobnext.id],
                        indent + 1,
                        default,
                    )
                    op_pos += 1
                end
            end

            bnext = true
            bplexity = false
            for jobni = (jobi+1):length(joblist)
                jobnext = joblist[jobni]
                bplexitytmp = numops(joblist[jobni], ordermachineid) == job_opsinmachine

                # Only connect on the same level (cases j->k and l->m) if it's the next one
                # or if we found the first job with the same plexity
                same = bnext | (bplexitytmp & !bplexity)
                op_curr_pos = 1 + (maxmachineops - job_opsinmachine)
                for op in machine_ops
                    _xml_sd_with_job(
                        f,
                        maxmachineops,
                        jobi,
                        jobni,
                        jobnext,
                        op,
                        op_curr_pos,
                        jobsmachineops[jobnext.id],
                        indent + 1,
                        default,
                        same,
                    )
                    op_curr_pos += 1
                end

                bnext = false
                bplexity |= bplexitytmp
            end
        end
    end
    println(f, "$(_ind(indent))</setupTimes>")
end

"""
Map of connections:

```
(j)────→(k)
 │     🡕 │
 │    ╱  │
 │   ╱   │
 │  ╱    │
 🡓 🡗     🡓
(l)────→(m)
```
"""
function _xml_sd_with_job(
    f::IO,
    maxops::Int,
    jobcurri::Int,
    jobnexti::Int,
    jobnext::Job,
    op::Operation,
    op_curr_pos::Int,
    jobnext_ops::Vector{Operation},
    indent::Int,
    default::Int,
    same::Bool = false,
)
    maxops_jobnext = length(jobnext_ops)
    

    for (op_next_pos, op_next) in enumerate(jobnext_ops)
        op_next_pos += maxops - maxops_jobnext

        # Discard k->j, m->l and m->j
        if jobnexti < jobcurri && op_curr_pos >= op_next_pos
            continue
        end

        # Discard j->m
        if jobnexti > jobcurri && op_curr_pos < op_next_pos
            continue
        end

        # Discard j->k and l->m if they are not the next one
        if !same && op_curr_pos == op_next_pos
            continue
        end

        val = get(op.time_sequence_dependent, JobId(-1), nothing)
        val = get(op.time_sequence_dependent, jobnext.id, val)

        if val === nothing || val == default
            return
        end

        println(
            f,
            """$(_ind(indent))<s j1="$(op.id[1].id)" op1="$(op.id[2].id)" \
            j2="$(op_next.id[1].id)" op2="$(op_next.id[2].id)" value="$val"/>""",
        )
    end
end

function _xml_modules_transfer(f::IO, transfer::ModulesTransferConstraints; indent = 1)
    println(f, """$(_ind(indent))<transfers>""")

    for ((id_from, id_to), point) in transfer
        println(
            f,
            """$(_ind(indent+1))<modulesTransfer id_from="$(id_from.id)" \
            id_to="$(id_to.id)"> """,
        )
        _xml_job_times(f, point.times_si, "setupTimes", "s", indent = indent + 2)
        _xml_job_times(f, point.times_d, "relativeDueDates", "d", indent = indent + 2)
        println(f, """$(_ind(indent+1))</modulesTransfer>""")
    end
    println(f, """  </transfers>""")
end

function _xml_job_times(
    f::IO,
    constraints::TransferConstraints,
    name::String,
    elem::String;
    indent = 1,
)
    println(f, """$(_ind(indent))<$name>""")
    for jobid in constraints |> keys |> collect |> sort
        time = constraints[jobid]
        println(f, """$(_ind(indent+1))<$elem j="$(jobid.id)" value="$time"/>""")
    end
    println(f, """$(_ind(indent))</$name>""")
end
end