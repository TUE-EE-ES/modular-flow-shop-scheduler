module Utils

import YAML

function print_defaults(args::Dict{String, Any}, defaults::Dict{String, Any})
    if !haskey(args, "print-defaults") || !args["print-defaults"]
        return
    end
    for (k, v) in defaults
        println("--$k=$v")
    end
    exit(0)
end

function load_config(
    args::Dict{String,Any}, 
    defaults::Dict{String,Any}, 
    handle::Bool=true)::Dict{String,Any}
    """Load the CLI configuration.

    The configuration is loaded with the following precedence:
    1. Default arguments
    2. JSON arguments
    3. CLI arguments

    Args:
        args: CLI arguments
        defaults: Dictionary containing default values. Keys are strings corresponding to the
                  CLI arguments.
        handle: Whether to interpret the arguments such as printing the defaults.

    Returns:
        Dictionary containing the configuration.
    """
    result = copy(defaults)

    # Load arguments passed from CLI
    args_filtered = filter(v -> v[2] !== nothing, args)

    yaml_path = get(args_filtered, "yaml", nothing)
    if yaml_path !== nothing && yaml_path isa String
        json_file = YAML.load_file(args_filtered["yaml"])
        merge!(result, json_file)
    end
    merge!(result, args_filtered)
    if handle
        print_defaults(result, defaults)
    end
    return result
end

end