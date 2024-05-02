__version__ = "0.1.0"

import argparse

import modfs.runner as runner


def main():
    """Startup point of all the commands."""
    parser = argparse.ArgumentParser(
        prog="modfs", description="Modular scheduling tools for flow-shop problems (ModFS)"
    )

    subparsers = parser.add_subparsers(
        title="Subcommands", dest="cmd", required=True, metavar="cmd"
    )

    runner.add_arguments(
        subparsers.add_parser(
            "run",
            help="Run modular flow-shop problems",
            formatter_class=argparse.RawTextHelpFormatter,
        )
    )

    args = parser.parse_args()
    args.func(args)


if __name__ == "__main__":
    main()
