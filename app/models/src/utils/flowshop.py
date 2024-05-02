from __future__ import annotations
import numpy as np
from dataclasses import dataclass, field
from collections import OrderedDict
import sys
from typing import NewType, Optional

np.set_printoptions(threshold=sys.maxsize)

MachineId = NewType("MachineId", int)
JobId = NewType("JobId", int)
OperationId = NewType("OperationId", int)
ModuleId = NewType("ModuleId", int)
FullOperationId = tuple[JobId, OperationId]


@dataclass
class Machine:
    idx: MachineId = MachineId(-1)
    operations: list[Operation] = field(default_factory=list)


@dataclass
class Job:
    idx: JobId = JobId(-1)
    operations: OrderedDict[OperationId, Operation] = field(default_factory=OrderedDict)
    deadline_absolute: int = sys.maxsize

    def add_op(
        self,
        op_idx: OperationId,
        processing: int,
        size: int,
        machine: Machine,
        start_time: int,
    ) -> Operation:
        """Add operation to the job.

        Args:
            op_idx (OperationId): Index of the operation.
            processing (int): Processing time of the operation.
            size (int): Size of the operation for maintenance purposes.
            machine (Machine): Instance of the Machine where the operation is executed.
        Returns:
            Operation: Instance of the added Operation.
        """
        op = Operation((self.idx, op_idx), processing, size, machine, start_time)
        self.operations[op_idx] = op
        machine.operations.append(op)

        return op


@dataclass
class Operation:
    idx: tuple[JobId, OperationId] = (JobId(-1), OperationId(-1))
    processing: int = 0
    size: int = 0
    machine: Machine = field(default_factory=Machine)
    start_time: int = 0


class Flowshop:
    def __init__(self, nmachines, njobs, nops, scale=1):
        self.nmachines = nmachines
        self.scale = scale
        self.njobs = njobs
        self.nops = nops
        self.X = np.zeros((njobs, nops, nmachines), dtype=np.int64)
        self.p = np.zeros((njobs, nops), dtype=np.int64)
        self.z = np.zeros((njobs, nops), dtype=np.int64)
        self.M = 10000000000
        self.setup = np.zeros((njobs, nops, njobs, nops))
        self.setupIndep = np.zeros((njobs, nops, njobs, nops))
        self.setupIndepDict: dict[tuple[FullOperationId, FullOperationId], int] = dict()
        self.due = np.full((njobs, nops, njobs, nops), np.iinfo(np.int64).max)
        self.dueIndep = np.full((njobs, nops, njobs, nops), np.iinfo(np.int64).max)
        self.dueIndepDict: dict[tuple[FullOperationId, FullOperationId], int] = dict()
        self.deadlines = np.full(njobs, np.iinfo(np.int64).max)

        # self.mp = [100000,1000000,15000000]
        self.mp = [100000, 200000, 300000]
        self.mp = [value / self.scale for value in self.mp]
        self.thresholds = [(1000000, 500000), (19000000, 10000000), (40000000, 20000000)]
        self.thresholds = [(a / self.scale, b / self.scale) for (a, b) in self.thresholds]

        self.jobs: list[Job] = []
        self.jobs_dict: dict[JobId, Job] = dict()
        self.machines: OrderedDict[MachineId, Machine] = OrderedDict()
        self.type = "flow"
        # self.type = "job"

    def add_job(self, job: Job):
        self.jobs.append(job)
        self.jobs_dict[job.idx] = job

    def query(self, op_from: FullOperationId, op_to: FullOperationId) -> int:
        return max(
            int(self.setup[op_from[0]][op_from[1]][op_to[0]][op_to[1]]),
            int(self.setupIndep[op_from[0]][op_from[1]][op_to[0]][op_to[1]]),
        )

    def due_date_indep(self, op_from: FullOperationId, op_to: FullOperationId) -> Optional[int]:
        r"""Obtain due date between `op_from` and `op_to`. If it has a value :math:`d`, it imposes
        the constraint:

        .. math::

            \Omega(op_{to}) + d \geq \Omega(op_{from})

        As you can see, the order is the opposite of the expected.

        Args:
            op_from (FullOperationId): Operation that must start before :math:`d` time units after
                `op_to` starts.
            op_to (FullOperationId): Operation that must start at least :math:`d` time units before
                `op_from` starts.

        Returns:
            Optional[int]: Due date value if it exists, None otherwise.
        """
        return self.dueIndepDict.get((op_from, op_to), None)


class ProductionLine:
    Modules = dict[ModuleId, Flowshop]
    TransferConstraint = dict[tuple[ModuleId, ModuleId], dict[JobId, int]]

    def __init__(self, modules: Modules, setup: TransferConstraint, due: TransferConstraint):
        self.modules = modules
        self.setup = setup
        self.due = due

        if len(self.modules) <= 0:
            raise ValueError("Production line must have at least one module.")
        self.module_last = sorted(self.modules.items(), key=lambda x: x[0])[-1][1]


class MaintenancePolicy:
    def __init__(self, nmaint, scale=1):
        self.scale = scale
        self.nmaintTypes = nmaint
        self.maintProcessing: list[float] = [0 for i in range(self.nmaintTypes)]
        self.maintThresholds: list[tuple[float, float]] = [(0, 0) for i in range(self.nmaintTypes)]
