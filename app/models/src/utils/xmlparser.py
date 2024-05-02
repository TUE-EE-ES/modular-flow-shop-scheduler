import xml.etree.ElementTree as ET
from utils.flowshop import (
    Flowshop,
    Job,
    JobId,
    Machine,
    MachineId,
    OperationId,
    MaintenancePolicy,
    ProductionLine,
    ModuleId,
)
from pathlib import Path
from typing import Optional


def _get_attrib(node: ET.Element, attrib: str, search: str = ""):
    if len(search) > 0:
        node_found = node.find(f"{search}[@{attrib}]")
        node_name = search
        if node_found is None:
            raise ValueError(f"Required element {search} with attribute {attrib} not found")
    else:
        node_found = node
        node_name = node.tag
    node_attrib = node_found.get(attrib)
    if node_attrib is None:
        raise ValueError(f"Required attribute {attrib} of {node_name} not found")
    return node_attrib


def _get_int_attrib(node: ET.Element, attrib: str, search: str = ""):
    value = _get_attrib(node, attrib, search)
    try:
        return int(value)
    except ValueError:
        return int(float(value))


def _get_default(node: ET.Element) -> Optional[int]:
    default = node.get("default")
    if default is not None:
        try:
            return int(default)
        except ValueError:
            return None
    return default


def _get_required_node(node: ET.Element, search: str):
    node_found = node.find(search)
    if node_found is None:
        raise ValueError(f"Required element {search} not found")
    return node_found


def readXML(path: Path) -> ET.Element:
    if not path.exists():
        raise FileNotFoundError(f"File {path} does not exist")

    tree = ET.parse(path)
    root = tree.getroot()
    return root


def createFlowShop(root: ET.Element, scale=1):
    njobs = _get_int_attrib(root, "count", "jobs")
    noperations = _get_int_attrib(root, "count", "jobs/operations")

    tmpFlow: list[tuple[int, int, int]] = []
    nmachines = 0
    for item in root.findall("flowVector"):
        for entry in item:
            j = _get_int_attrib(entry, "job")
            op = _get_int_attrib(entry, "index")
            m = _get_int_attrib(entry, "value")
            nmachines = max(nmachines, m + 1)
            tmpFlow.append((j, op, m))

    fs = Flowshop(nmachines, njobs, noperations, scale)
    for j, op, m in tmpFlow:
        fs.X[j][op][m] = 1

    for item in root.findall("processingTimes"):
        default = _get_default(item)
        if default is not None:
            fs.p.fill(default)

        for entry in item:
            j = _get_int_attrib(entry, "j")
            op = _get_int_attrib(entry, "op")
            fs.p[j][op] = _get_int_attrib(entry, "value")

    for item in root.findall("absoluteDeadlines"):
        for entry in item:
            j = _get_int_attrib(entry, "j")
            fs.deadlines[j] = _get_int_attrib(entry, "value")

    for item in root.findall("setupTimes"):
        default = _get_default(item)
        if default is not None:
            fs.setup.fill(default)
        for entry in item:
            j1 = _get_int_attrib(entry, "j1")
            op1 = _get_int_attrib(entry, "op1")
            j2 = _get_int_attrib(entry, "j2")
            op2 = _get_int_attrib(entry, "op2")
            fs.setup[j1][op1][j2][op2] = _get_int_attrib(entry, "value")

    for item in root.findall("setupTimesIndependent"):
        for entry in item:
            j1 = _get_int_attrib(entry, "j1")
            op1 = _get_int_attrib(entry, "op1")
            j2 = _get_int_attrib(entry, "j2")
            op2 = _get_int_attrib(entry, "op2")
            value = _get_int_attrib(entry, "value")
            fs.setupIndep[j1][op1][j2][op2] = value
            fs.setupIndepDict[
                ((JobId(j1), OperationId(op1)), (JobId(j2), OperationId(op2)))
            ] = value

    for item in root.findall("relativeDueDates"):
        for entry in item:
            j1 = _get_int_attrib(entry, "j1")
            op1 = _get_int_attrib(entry, "op1")
            j2 = _get_int_attrib(entry, "j2")
            op2 = _get_int_attrib(entry, "op2")
            fs.due[j2][op2][j1][op1] = int(float(entry.attrib["value"]))

    for item in root.findall("relativeDueDatesIndependent"):
        for entry in item:
            j1 = _get_int_attrib(entry, "j1")
            op1 = _get_int_attrib(entry, "op1")
            j2 = _get_int_attrib(entry, "j2")
            op2 = _get_int_attrib(entry, "op2")
            value = int(float(entry.attrib["value"]) / fs.scale)
            fs.dueIndep[j2][op2][j1][op1] = value
            fs.dueIndepDict[((JobId(j1), OperationId(op1)), (JobId(j2), OperationId(op2)))] = value

    for item in root.findall("sizes"):
        default = _get_default(item)
        if default is not None:
            fs.z.fill(default)

        for entry in item:
            j = _get_int_attrib(entry, "j")
            op = _get_int_attrib(entry, "op")
            fs.z[j][op] = _get_int_attrib(entry, "value")

    # create shop machine objects
    fs.machines.clear()
    for m_id in range(fs.nmachines):
        fs.machines[MachineId(m_id)] = Machine(idx=MachineId(m_id))

    # create shop job and operation objects
    fs.jobs.clear()
    job_ids: set[int] = set()
    for job_id, _, _ in tmpFlow:
        job_ids.add(job_id)

    for job_id in sorted(job_ids):
        job = Job(JobId(job_id))
        fs.add_job(job)

    for job_id, op_id, machine_id in tmpFlow:
        machine = fs.machines[MachineId(machine_id)]
        job = fs.jobs_dict[JobId(job_id)]
        job.add_op(
            OperationId(op_id),
            fs.p[job_id][op_id],
            fs.z[job_id][op_id],
            machine,
            0,
        )

    return fs


def initMaintenancePolicy(root, scale):
    # flowVector = [int(child.attrib['value']) for child in root.find('flowVector')]
    # nmachines = flowVector[-1] + 1

    nmaint = int(root.find("types").attrib["count"])

    policy = MaintenancePolicy(nmaint, scale)

    for item in root.findall("processingTimes"):
        for entry in item:
            t = _get_int_attrib(entry, "t")
            policy.maintProcessing[t] = int(float(entry.attrib["value"])) / policy.scale

    for item in root.findall("thresholds"):
        for entry in item:
            t = _get_int_attrib(entry, "t")
            s = float(entry.attrib["s"]) / policy.scale
            e = float(entry.attrib["e"]) / policy.scale
            policy.maintThresholds[t] = (s, e)

    return policy


def extractFlowShop(path: Path, scale=1):
    root = readXML(path)
    input = createFlowShop(root, scale)
    return input


def extractMaintPolicy(path: Path, scale=1):
    root = readXML(path)
    policy = initMaintenancePolicy(root, scale)
    return policy


def extractProductionLine(path: Path) -> ProductionLine:
    root = readXML(path)
    problem_type = root.get("type")
    if problem_type not in ["Modular", "FORPFSSPSD"]:
        raise ValueError(f"Problem type {problem_type} not supported")

    modules = ProductionLine.Modules()

    if problem_type == "Modular":
        for node in root.findall("./SPInstance[@type='FORPFSSPSD'][@id]"):
            module_id = ModuleId(_get_int_attrib(node, "id"))
            modules[module_id] = createFlowShop(node)
    else:
        modules[ModuleId(0)] = createFlowShop(root)

    nodes_transfers = root.findall("./transfers/modulesTransfer[@id_from][@id_to]")
    if len(nodes_transfers) != len(modules) - 1:
        raise ValueError("Not enough transfer information")

    setup = ProductionLine.TransferConstraint()
    due = ProductionLine.TransferConstraint()
    for node in nodes_transfers:
        id_module_from = ModuleId(_get_int_attrib(node, "id_from"))
        id_module_to = ModuleId(_get_int_attrib(node, "id_to"))
        key = (id_module_from, id_module_to)
        setup[key] = dict()
        due[key] = dict()

        if id_module_from + 1 != id_module_to and 0 <= id_module_from < len(modules):
            raise ValueError("Transfer information not valid")

        for node_s in node.findall("./setupTimes/s[@j][@value]"):
            j = JobId(_get_int_attrib(node_s, "j"))
            setup[key][j] = _get_int_attrib(node_s, "value")

        for node_d in node.findall("./relativeDueDates/d[@j][@value]"):
            j = JobId(_get_int_attrib(node_d, "j"))
            due[key][j] = _get_int_attrib(node_d, "value")

    return ProductionLine(modules, setup, due)


if __name__ == "__main__":
    import argparse

    parser = argparse.ArgumentParser(description="Extract input from xml file")
    parser.add_argument(
        "--type", help="type of input", choices=["shop", "maintenance", "line"], default="shop"
    )
    parser.add_argument("path", help="path to xml file", type=Path)

    args = vars(parser.parse_args())

    if args["type"] == "shop":
        extractFlowShop(args["path"])
    elif args["type"] == "maintenance":
        extractMaintPolicy(args["path"])
    elif args["type"] == "line":
        extractProductionLine(args["path"])
