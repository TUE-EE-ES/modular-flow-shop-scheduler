module Algorithms

using Base.Iterators
using DataStructures
using FilePaths

using ...FlowShops
import ..Specification as S

"Maps an operation ID from a global to a local flow-shop"
const OpsMapperGL = Dict{JobId,Dict{OperationId,Tuple{ModuleId,OperationId}}}

"Maps an operation ID from a local to a global flow-shop"
const OpsMapperLG = Dict{JobId,Dict{Tuple{ModuleId,OperationId},OperationId}}


function flowshop_generate(
    spec::S.SingleFlowShopSpec,
    jobpattern::S.JobPattern,
    filepath::AbstractPath,
)::FlowShop

    jobs, mapoldnew = _map_jobs(jobpattern)
    flowshop = FlowShop(_map_machines(spec.machines), filepath)

    for (jobid, jobid_new) in jobs
        # Create a new job
        job = Job(jobid_new)
        flowshop.jobs[jobid_new] = job
        job_spec = spec.jobs[jobid]

        for (opid, op_spec) in job_spec.ops
            opid_new = OperationId(opid)
            machineid = MachineId(op_spec.machine)
            addop(
                job,
                opid_new,
                op_spec.time_processing,
                _map_sd_times(op_spec.time_sequence_dependent, mapoldnew),
                op_spec.size,
                flowshop.machines[machineid],
            )
        end
        _add_time_func!(flowshop.times_si, job, job_spec.setup)
        _add_time_func!(flowshop.deadlines, job, job_spec.deadlines)
    end
    _add_time_func!(flowshop.times_si, flowshop, jobpattern.setup_times)
    _add_time_func!(flowshop.deadlines, flowshop, jobpattern.deadlines)
    info = _add_job_deadlines!(flowshop.deadlines, flowshop, jobs, jobpattern.deadlines_job)

    flowshop.extrainfo = Dict("jobs" => length(jobs), "deadlines" => info)
    flowshop
end

function _add_job_deadlines!(
    dest::Dict{TimeFunc,Int},
    flowshop::FlowShop,
    jobs::Vector{Tuple{Int,JobId}},
    deadlines::DefaultDict{Int,Union{Int,Nothing},Union{Int,Nothing}},
)::Dict{String,Int}

    deadlines_copy = copy(deadlines)
    deadlineinfo = Dict{String,Int}()

    default = deadlines.d.default
    if default !== nothing
        deadlineinfo["default"] = default
    end

    for (k, v) in deadlines_copy
        deadlineinfo[string(k)] = v
    end

    for ((_, jobid), (_, jobid_next)) in zip(jobs, jobs[2:end])
        deadline = deadlines_copy[jobid.id]

        if deadline === nothing
            continue
        end

        lastop = last(flowshop.jobs[jobid].operations).second
        lastop_next = last(flowshop.jobs[jobid_next].operations).second

        dest[(lastop_next, lastop)] = deadline
    end
    deadlineinfo
end

"""
Saves the constraints of a specific job into into the table `dest`
"""
function _add_time_func!(dest::Dict{TimeFunc,Int}, job::Job, value::Set{S.TimeFuncSpec})
    for v in value
        op_src = job.operations[OperationId(v.op1)]
        op_dst = job.operations[OperationId(v.op2)]

        if (op_src, op_dst) ∉ keys(dest)
            dest[(op_src, op_dst)] = v.time
        end
    end
end

"""
Saves fully defined constraints into table `dest`
"""
function _add_time_func!(
    dest::Dict{TimeFunc,Int},
    flowshop::FlowShop,
    value::Set{S.FullTimeFuncSpec},
)
    for v in value
        op_src = getop(flowshop, JobId(v.op_src[1]), OperationId(v.op_src[2]))
        op_dst = getop(flowshop, JobId(v.op_dst[1]), OperationId(v.op_dst[2]))
        dest[(op_src, op_dst)] = v.time
    end
end

function _map_jobs(
    jobpattern::S.JobPattern,
)::Tuple{Vector{Tuple{Int,JobId}},Dict{Int,Vector{JobId}}}
    jobidnewidx = 0
    mapoldnew = Dict{Int,Vector{JobId}}(-1 => [JobId(-1)])
    result = Vector{Tuple{Int,JobId}}()
    for (jobid, count) in jobpattern.jobs
        for _ = 1:count
            jobidnew = JobId(jobidnewidx)
            push!(result, (jobid, jobidnew))
            push!(get!(mapoldnew, jobid, []), jobidnew)
            jobidnewidx += 1
        end
    end
    result, mapoldnew
end

function _map_machines(machines_spec::Vector{Int})::SortedDict{MachineId,Machine}
    result = SortedDict{MachineId,Machine}()
    for machineid in machines_spec
        machineid_new = MachineId(machineid)
        machine = newmachine(machineid_new)
        result[machineid_new] = machine
    end
    result
end

function production_line_assemble(
    flowshops::Dict{ModuleId,FlowShop},
    pattern::S.ModulePattern,
    filepath::AbstractPath = Path(),
)::ProductionLine

    modules_ids = _map_modules(pattern)
    modules = Dict{ModuleId,FlowShop}()
    first = true
    numjobs = nothing
    for (id_module_pattern, id_module) in modules_ids
        modules[id_module] = flowshops[id_module_pattern]

        if first
            first = false
            numjobs = length(modules[id_module].jobs)
        end
    end

    transfers = ModulesTransferConstraints()

    for ((_, id_module), (_, id_module_n)) in zip(modules_ids, modules_ids[2:end])
        # Get transfer constraints
        mod = modules[id_module]
        tp = TransferPoint(
            _get_transfer_constraints(mod, id_module, pattern.setup_times),
            _get_transfer_constraints(mod, id_module, pattern.deadlines, false),
        )
        _check_validity(tp)
        transfers[(id_module, id_module_n)] = tp
    end

    extrainfo = Dict(
        "jobs" => numjobs,
        "modules" => length(modules_ids),
        "modules_info" => SortedDict(
            moduleid.id => flowshop.extrainfo for (moduleid, flowshop) in modules
        ),
    )

    ProductionLine(modules, transfers, extrainfo, filepath)
end

function _map_modules(modules_spec::S.ModulePattern)::Vector{Tuple{ModuleId,ModuleId}}
    modules = Vector{Tuple{ModuleId,ModuleId}}()
    moduleid = 0
    for (moduleid_pattern, count) in modules_spec.modules
        for _ = 1:count
            push!(modules, (ModuleId(moduleid_pattern), ModuleId(moduleid)))
            moduleid += 1
        end
    end
    modules
end

function _get_transfer_constraints(
    f_from::FlowShop,
    id_module::ModuleId,
    times::S.TransferSpec,
    add_default::Bool = true,
)::TransferConstraints
    result = TransferConstraints()

    # Get global defaults
    job_times_default = get(times, -1, Dict())
    job_times_module = get(times, id_module, Dict())
    val_default = get(job_times_default, -1, add_default ? 0 : -1)

    for jobid in keys(f_from.jobs)
        # Find the value for this job and default to global if not found
        val_job_global = get(job_times_default, jobid, val_default)
        val_definitive = get(job_times_module, jobid, val_job_global)

        # A negative value means that we are removing it. Useful for when setting a default
        # deadline and then removing a specific one
        if val_definitive >= 0
            result[jobid] = val_definitive
        elseif add_default
            throw(DomainError("Invalid value $val_definitive for job $jobid"))
        end
    end
    result
end

function _map_sd_times(
    timesd::Dict{Int,Int},
    mapoldnew::Dict{Int,Vector{JobId}},
)::Dict{JobId,Int}
    result = Dict{JobId,Int}()
    for (jobid, time) ∈ timesd
        # Check if the jobid is used by the pattern
        if jobid ∉ keys(mapoldnew)
            continue
        end
        for jobid_new ∈ mapoldnew[jobid]
            result[jobid_new] = time
        end
    end
    result
end

function _check_validity(
    transfer::TransferPoint
)

    for (jobid, value) ∈ transfer.times_d
        # Find value for setup
        s_value = get(transfer.times_si, jobid, -1)

        if s_value > value
            throw(DomainError("Setup time $s_value is greater than deadline $value"))
        end
    end
end

# Module
end