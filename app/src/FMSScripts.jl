module FMSScripts
using ArgParse

include("data/FlowShop.jl")
include("generator/Generator.jl")

function parse_commandline()
    s = ArgParseSettings()
    Generator.add_subcommands(s)
    return parse_args(s)
end

function main(args::Dict{String, Any})
    if args["%COMMAND%"] == "gen"
        Generator.main_generate(args["gen"])
    end
end

function julia_main()::Cint
    parsed_args = parse_commandline()
    println(parsed_args)
    main(parsed_args)
    return 0
end

end # module
