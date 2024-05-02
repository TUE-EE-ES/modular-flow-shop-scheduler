include("../utils/ConfigLoader.jl")
include("Specification.jl")

module Generator
using ArgParse
using FilePaths; using FilePathsBase: /
using ProgressMeter
using DataStructures
import JSON

import ..Utils
import ..Specification
import ..FlowShops as FS
include("Algorithms.jl")

DEFAULT_GENERATE_ARGS = Dict(
    "print-defaults" => false,
    "out" => "./data/gen",
    "debug" => false,
    "processes" => nothing,
)

DEFAULT_MODULARISE_ARGS = Dict(
    "print-defaults" => false,
    "out" => "./data/gen/",
    "debug" => false,
    "processes" => nothing,
)

const PatternIds = Tuple{Int,Specification.ModulePattern}


function add_subcommands(args::ArgParseSettings)
    @add_arg_table! args begin
        "gen"
        help = "Generate modular flow-shop problems"
        action = :command
    end
    @add_arg_table! args["gen"] begin
        "spec"
        help = "YAML file containing the flow-shop specification or folder containing \
                YAML files."
        "--yaml"
        help = "YAML file (with comment support) containing arguments. Can be used instead \
                of passing the optional command line arguments. The command line arguments \
                always have preference."
        "--print-defaults"
        help = "Shows the default values used by the arguments"
        action = :store_true
        "--out", "-o"
        help = "Path where the output files should be saved, directories will be created \
                if they do not exist"
        "--force"
        help = "Force the generation of the files even if they already exist"
        action = :store_true
    end
    @add_arg_table! args begin
        "modularise"
        help = "Modularise existing flow-shop problems"
        action = :command
    end
    @add_arg_table! args["modularise"] begin
        "in"
        help = "Input folder containing all the xml files"
        "spec"
        help = "YAML file with the specification of the modularisation to perform on the \
                input files. Multiple specifications can be used to generate multiple \
                files."
        "--YAML"
        help = "YAML file (with comment support) containing arguments. Can be used instead \
                of passing the optional command line arguments. The command line arguments \
                always have preference."
        "--out"
        "-o"
        help = "Path where the output should be saved, the directory will be created if it \
                does not exist."
        "--print-defaults"
        help = "Shows the default values used by the arguments"
        action = :store_true
    end
end

function main_generate(args::Dict{String,Any})
    """Generate new flow-shops"""
    config = Utils.load_config(args, DEFAULT_GENERATE_ARGS)
    if !haskey(config, "spec") || config["spec"] === nothing
        print(stderr, "No flow-shop specification has been supplied")
        return
    end

    forceall = get(config, "force", false)

    args_spec = config["spec"] |> Path |> abspath
    if isdir(args_spec)
        specsdir = args_spec
    else
        specsdir = dirname(args_spec)
    end

    specs = Specification.load_specification(Path(config["spec"]))

    out_folder = config["out"] |> Path |> abspath
    mkpath(out_folder)


    for spec in specs
        count = length(spec.patterns) * length(spec.pattern_modules)
        println("Generating patterns of $(spec.output_name) ($count)")

        filesinfo = SortedDict{Int,Dict{String,Any}}()
        outfolder_spec = out_folder / relpath(dirname(spec.filepath), specsdir) / spec.output_name
        mkpath(outfolder_spec)
        patterns_job = Vector{Tuple{Specification.JobPattern,Vector{PatternIds}}}()

        id = 0
        for job_pattern in spec.patterns
            patterns_modules = Vector{PatternIds}()
            for module_pattern in spec.pattern_modules
                push!(patterns_modules, (id, module_pattern))
                id += 1
            end
            push!(patterns_job, (job_pattern, patterns_modules))
        end

        count += length(spec.patterns) * length(spec.flow_shops)
        p = Progress(count, desc = "XML")
        l = ReentrantLock()
        println("Assmebling lines and creating XML files ($count)")
        Threads.@threads for (job_pattern, patterns_modules) in patterns_job
        # for (job_pattern, patterns_modules) in patterns_job
            flowshops = Dict{FS.ModuleId,FS.FlowShop}()
            for (module_id, flowshop_spec) in spec.flow_shops
                flowshops[FS.ModuleId(module_id)] =
                    Algorithms.flowshop_generate(flowshop_spec, job_pattern, spec.filepath)
                next!(p)
            end

            Threads.@threads for (patternid, module_pattern) in patterns_modules
            # for (patternid, module_pattern) in patterns_modules
                line = Algorithms.production_line_assemble(
                    flowshops,
                    module_pattern,
                    spec.filepath,
                )
                lock(l)
                filesinfo[patternid] = line.extrainfo
                unlock(l)

                outname = "$outfolder_spec/$patternid.xml"
                if !ispath(outname) || forceall
                    FS.toXML(line, outname)
                end
                next!(p)
            end
        end

        open(outfolder_spec * "/files_info.json", "w") do f
            JSON.print(
                f,
                Dict(
                    "description" => spec.description,
                    "files" => filesinfo,
                    "spec_file" => convert(String, spec.filepath),
                ),
                2,
            )
        end

    end
end

end