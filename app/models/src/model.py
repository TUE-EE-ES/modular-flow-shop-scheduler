import utils.flowshop as FS
from utils import xmlparser
from pathlib import Path


class Model:
    def __init__(
        self,
    ):
        self.njobs = 0
        self.nops = 0
        self.nmachines = 0
        self.m_assign = []
        self.processing = []
        self.setup = []
        self.setupIndep = []
        self.due = []
        self.dueIndep = []
        self.sizes = []

    def makeOpContainer(self):
        return [[0 for operation in range(self.nops)] for job in range(self.njobs)]

    def before(j, k):
        return j < k

    def jobOps(self):
        return [(i, j) for j in range(self.nops) for i in range(self.njobs)]

    def findMachine(i, j, X):
        n = 0
        for val in X[i][j]:
            if val:
                return n
            n += 1

    def getOpsOnMachine(self, mu):
        return [
            (i, j) for i in range(self.njobs) for j in range(self.nops) if self.m_assign[i][j][mu]
        ]

    def getStartOnMachine(self, mu):
        start = {0: 0, 1: 1, 2: 3}
        return (0, start[mu])

    def getEndOnMachine(self, mu):
        end = {0: 0, 1: 2, 2: 3}
        return (self.njobs - 1, end[mu])

    def extractInput(self, spec, scale):
        input = xmlparser.extractFlowShop(Path(spec), scale)
        return input

    def extractMaintPolicy(self, spec, scale):
        policy = xmlparser.extractMaintPolicy(spec, scale)
        return policy

    @staticmethod
    def extractProductionLine(spec: Path) -> FS.ProductionLine:
        return xmlparser.extractProductionLine(spec)
