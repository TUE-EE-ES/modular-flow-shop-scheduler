"""Module to load default arguments. It extends over argparse."""
import argparse
import sys

import commentjson as json


def load_config(args: argparse.Namespace, defaults: dict, handle: bool = False) -> dict:
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
    # Load default arguments
    res = defaults.copy()
    args_dict = vars(args)
    args_filtered = filter(lambda x: x[1] is not None, args_dict.items())

    # Find a JSON parameter and if it's there, load it
    if "json" in args_dict and args_dict["json"] is not None:
        with open(args_dict["json"], "r") as f:
            args_json = json.load(f)
        # Add JSON arguments
        res.update(args_json)

    # Finally, add CLI arguments
    res.update(args_filtered)

    if handle:
        print_defaults(args_dict, defaults)

    return res


def print_defaults(args: dict, defaults: dict):
    """Print the default arguments.

    Args:
        args (dict): Parsed arguments represented as a dict.
        defaults (dict): Default arguments to print if "print_defaults" found in arguments.
    """
    if "print_defaults" not in args or not args["print_defaults"]:
        return
    for k, v in defaults.items():
        print(f"--{str(k).replace('_', '-')}={v}")

    sys.exit(0)
