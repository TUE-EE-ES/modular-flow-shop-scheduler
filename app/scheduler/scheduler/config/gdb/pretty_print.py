import gdb
import gdb.printing


def _get_optional(val):
    vis = gdb.default_visualizer(val)
    if vis is None or vis.contained_value is None:
        return ""
    return str(vis.contained_value)


class StrongTypePrinter:
    def __init__(self, val):
        self.val = val

    def to_string(self):
        return str(self.val['value'])


class OperationPrinter:
    def __init__(self, val):
        self.val = val

    def to_string(self):
        return "({}, {})".format(self.val['jobId'], self.val['operationId'])


class MathIntervalPrinter:
    def __init__(self, val):
        self.val = val
        
    def to_string(self):
        strmin = _get_optional(self.val['m_min'])
        strmax = _get_optional(self.val['m_max'])
        
        if len(strmin) == 0:
            strmin = "-inf"
        if len(strmax) == 0:
            strmax = "inf"
        return "[{}, {}]".format(strmin, strmax)


def build_pretty_printer():
    pp = gdb.printing.RegexpCollectionPrettyPrinter("scheduler")
    pp.add_printer('FMS::Utils::StrongType', '^FMS::Utils::StrongType<.*>$', StrongTypePrinter)
    pp.add_printer('FORPFSSPSD::operation', '^FORPFSSPSD::operation$', OperationPrinter)
    pp.add_printer('Math::Interval', '^Math::Interval<.*>$', MathIntervalPrinter)
    
    return pp


gdb.printing.register_pretty_printer(
    gdb.current_objfile(),
    build_pretty_printer())
