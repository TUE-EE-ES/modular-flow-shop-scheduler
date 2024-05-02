

module Specification

using DataStructures
using FilePaths
import YAML

const OperationId = Tuple{Int,Int}

"Specification of an operation"
struct OpSpec
    machine::Int
    time_processing::Int

    """Sequence dependent setup time with other jobs

    The key `k` is a job id such that if any operation of `k` is scheduled after the current
    operation, then the setup time (value) is applied.
    """
    time_sequence_dependent::Dict{Int,Int}

    size::Int

    OpSpec(op::Dict) = new(
        op["machine"],
        op["time_processing"],
        _parse_op_setup_times(op),
        get(op, "size", 0),
    )
end

"""Time function with a full id (Job Id and Operation Id)"""
struct FullTimeFuncSpec
    op_src::OperationId
    op_dst::OperationId
    time::Int
end

"""Time function with a partial id (Operation Id only)"""
struct TimeFuncSpec
    op1::Int
    op2::Int
    time::Int
end

"""
Specification of a transfer point. 

The first key is the id of the module from which jobs flow  after the pattern example. If 
negative or not specified, applies to all modules. YAML key `id_module`.

The second key is the id of the job where the time must be applied. If negative, or not \
specified applies to all jobs. YAML key `id_job`.

The value is the constraint time. YAML key `time`.
"""
const TransferSpec = Dict{Int,Dict{Int,Int}}

struct JobSpec
    ops::SortedDict{Int,OpSpec}
    deadlines::Set{TimeFuncSpec}
    setup::Set{TimeFuncSpec}

    JobSpec(job::Dict) = new(
        SortedDict(_int(op_id) => OpSpec(op) for (op_id, op) in get(job, "ops", [])),
        load_time_func_spec(get(job, "deadlines", [])),
        load_time_func_spec(get(job, "setup", [])),
    )
end

struct SingleFlowShopSpec
    jobs::Dict{Int,JobSpec}
    machines::Vector{Int}

    SingleFlowShopSpec(flowshop::Dict) =
        new(_single_job(get(flowshop, "jobs", [])), get(flowshop, "machines", []))
end

struct ModulePattern
    modules::Vector{Tuple{Int,Int}}
    setup_times::TransferSpec
    deadlines::TransferSpec

    ModulePattern(pattern::Dict) = new(
        map(x -> (x[1], x[2]), get(pattern, "modules", [])),
        load_time_module_spec(
            get(pattern, "setup_times", Vector{Dict{Any,Any}}()),
            get(pattern, "default_setup_time", 0),
        ),
        load_time_module_spec(
            get(pattern, "deadlines", Vector{Dict{Any,Any}}()),
            get(pattern, "default_deadline", -1),
        ),
    )
end

struct JobPattern
    jobs::Vector{Tuple{Int,Int}}
    setup_times::Set{FullTimeFuncSpec}
    deadlines::Set{FullTimeFuncSpec}
    deadlines_job::DefaultDict{Int,Union{Int,Nothing},Union{Int,Nothing}}

    JobPattern(pattern::Dict) = new(
        _load_job_patterns(get(pattern, "jobs", [])),
        load_full_time_func_spec(get(pattern, "setup_times", Vector{Dict{Any,Any}}())),
        load_full_time_func_spec(get(pattern, "deadlines", Vector{Dict{Any,Any}}())),
        DefaultDict{Int,Union{Int,Nothing},Union{Int,Nothing}}(
            get(pattern, "deadlines_job_default", nothing),
            get(pattern, "deadlines_job", Dict{Int,Union{Int,Nothing}}()),
        ),
    )
end

struct FlowShopSpec
    output_name::String
    description::String
    filepath::AbstractPath
    flow_shops::Dict{Int,SingleFlowShopSpec}
    patterns::Vector{JobPattern}
    pattern_modules::Vector{ModulePattern}
end

function load_specification(filepath::AbstractPath)::Vector{FlowShopSpec}
    filepath = abspath(filepath)
    if !isdir(filepath)
        spec =
            load_single_file(filepath, replace(basename(filepath), ".fs.yaml" => ""))
        return [spec]
    end

    specs = Vector{FlowShopSpec}()
    for (root, _, files) in walkdir(filepath, follow_symlinks = true)
        for file in files
            if !endswith(file, ".fs.yaml")
                continue
            end
            curr_filepath = joinpath(root, file) |> AbstractPath
            file_relative = relpath(curr_filepath, filepath)
            spec = load_single_file(
                curr_filepath,
                replace(basename(file_relative), ".fs.yaml" => ""),
            )
            push!(specs, spec)
        end
    end
    return specs
end

function load_single_file(filepath::AbstractPath, name::String)::FlowShopSpec
    yaml::Dict{String,Any} = YAML.load_file(convert(String, filepath))

    patterns_modules::Dict{String,Any} = get(yaml, "patterns_modules", Dict{String,Any}())
    _add_subkeydefault!(patterns_modules, "default_setup_time", "patterns")
    _add_subkeydefault!(patterns_modules, "default_deadline", "patterns")

    FlowShopSpec(
        name,
        get(yaml, "description", ""),
        filepath,
        _single_flowshop(get(yaml, "flow_shops", [])),
        map(JobPattern, get(yaml, "patterns", [])),
        map(ModulePattern, get(patterns_modules, "patterns", [])),
    )
end

function load_time_module_spec(
    timemodule::Vector{Dict{Any,Any}},
    default::Int,
)::TransferSpec
    result = TransferSpec()
    result[-1] = Dict(-1 => default)

    for val in timemodule
        # Check if it has all the required keys
        if !"time" in keys(val)
            printstyled(stderr, "Failed to find 'time'. Ignoring.", color = :yellow)
            continue
        end

        id_module = max(get(val, "id_module", -1), -1)
        id_job = max(get(val, "id_job", -1), -1)
        time = val["time"]

        get!(result, id_module, Dict{Int,Int}())[id_job] = time
    end
    result
end

function load_time_func_spec(timefunc::Vector{Dict{Any,Any}})::Set{TimeFuncSpec}
    Set{TimeFuncSpec}(
        TimeFuncSpec(get(x, "op1", 0), get(x, "op2", 0), get(x, "time", 0)) for
        x in timefunc
    )
end

function load_full_time_func_spec(timefunc::Vector{Dict{Any,Any}})::Set{FullTimeFuncSpec}
    Set{FullTimeFuncSpec}(
        FullTimeFuncSpec(
            get(x, "op_src", (0, 0)),
            get(x, "op_dst", (0, 0)),
            get(x, "time", 0),
        ) for x in timefunc
    )
end

function verify_single_flow_shop_spec(spec::SingleFlowShopSpec)
    for (_, job) in spec.jobs
        idx = 1
        for (_, op) in job.ops
            while idx <= length(spec.machines) && op.machine != spec.machines[idx]
                idx += 1
            end

            if idx > length(spec.machines)
                error("A job does not respect the order of machines")
            end
        end
    end
end

function _add_subkeydefault!(dict::Dict{String,Any}, key_default::String, subkey::String)
    default_value = get(dict, key_default, nothing)
    if default_value !== nothing
        map(x -> x[key_default] = default_value, get(dict, subkey, []))
    end
end

_int(x::Union{String,Int})::Int = isa(x, String) ? parse(Int, x) : x

"""Ensures that the value is a dictionary and throws otherwise"""
_ensure_dict(x::Any, message::String) =
    isa(x, Dict) ? x : throw(DomainError("$(message)Expected a dictionary"))

function _parse_op_setup_times(op::Dict)::Dict{Int,Int}
    # Check whether it's a single value or a dictionary
    value = get(op, "time_sequence_dependent", Dict())
    if isa(value, Int)
        return Dict{Int,Int}(-1 => value)
    end
    valuedict = _ensure_dict(value, "Op parse: ")
    Dict{Int,Int}(_int(k) => _int(v) for (k, v) in valuedict)
end

_single_flowshop(flowshops::Dict) = Dict{Int,SingleFlowShopSpec}(
    begin
        spec = SingleFlowShopSpec(flowshop)
        verify_single_flow_shop_spec(spec)
        _int(id) => SingleFlowShopSpec(flowshop)
    end for (id, flowshop) in flowshops
)

_single_job(jobs::Dict) = Dict{Int,JobSpec}(_int(id) => JobSpec(job) for (id, job) in jobs)

function _load_job_patterns(jobs)::Vector{Tuple{Int,Int}}
    result = Vector{Tuple{Int,Int}}()

    for job in jobs
        if isa(job, Int)
            push!(result, (job, 1))
        elseif isa(job, Tuple)
            push!(result, job)
        elseif isa(job, Vector) && length(job) >= 2
            push!(result, (job[1], job[2]))
        else
            error("Invalid job pattern $job $(typeof(job)))")
        end
    end
    result
end

end # module