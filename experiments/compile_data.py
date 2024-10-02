from enum import Enum
from os import listdir, linesep
from os.path import isfile, join
import re
from re import Pattern
from typing import Any, Dict, List, Set
import csv
from statistics import stdev, median, mean, quantiles


files: Set[str] = {f for f in listdir("output") if isfile(join("output", f))}
details_files = set(filter(lambda f: f.startswith("details"), files))
exec_inf_files = files - details_files


class CSVFields(Enum):
    DATA = 1
    SCHEDULING_END = 0
    MAKESPAN = 1
    ERROR_COUNT = 2
    DETAIL_BEGIN = 5
    REPARTITION_REQUEST = 0
    GRAPH_COPY_DURATION = 1
    REPARTITION_BEGIN = 2
    REPARTITION_END = 3
    RECONSTRUCTION_DURATION = 4
    APPLY_TIME = 5
    EXEC_INF_BEGIN = 1
    EXECUTED = 0
    QUEUED_BEGIN = 6


class InfFields(Enum):
    SCHEDULING_END = 0
    MAKESPAN = 1
    ERROR_COUNT = 2
    REPARTITIONS = 3
    GRAPH_COPY_DURATION_MEDIAN = 4
    THROUGHPUT_MEAN = 5
    THROUGHPUT_LOW = 6
    THROUGHPUT_MEDIAN = 7
    THROUGHPUT_HIGH = 8
    THROUGHPUT_MAX = 9
    DOWN_TIME = 10
    TIME_UNDER_REF_MEDIAN = 11
    QUEUES_CV_MEAN = 12
    QUEUES_CV_MEDIAN = 13
    QUEUES_CV_HIGH = 14


def scnot(num: str):
    tens = 0
    for c in reversed(num):
        if c == '0':
            tens += 1
        else:
            break
    if tens > 1:
        base = 0x2070
        if tens <= 3:
            base = 0xB0
        num = num[:-tens]
        if num == "1":
            return "10" + chr(base+tens)
        return num[:-tens] + "×10" + chr(base+tens)
    else:
        return num


def load_csv(csv_path: str) -> List[List[str]]:
    values: List[List[str]] = []
    with open(csv_path, "r") as csv_file:
        csv_reader = csv.reader(csv_file)
        for csv_row in csv_reader:
            if csv_row:
                values.append([value for value in csv_row])

    return values


class Inf:
    def __init__(
        self,
        exec_inf_path: str,
        ref: "Inf" = None,
        ref_threshould: int = 0,
        dir: str = "output/",
    ) -> None:

        self.throughputs: List[float] = []
        self.queues_cvs: List[float] = []

        self.ref_throughput: float = 0
        if ref is not None:
            self.ref_throughput = ref.summary[InfFields.THROUGHPUT_MEDIAN] * (
                1 - ref_threshould
            )

        self.exec_inf_path: str = exec_inf_path
        self.details_path: str = "details_" + self.exec_inf_path.removesuffix(".csv")
        self.dir = dir

        self.summary: Dict[InfFields, float] = {}
        self.rank: Dict[InfFields, int] = {}

        self.load_summary()
        self.name: str = Inf.get_name(self.exec_inf_path)

    @classmethod
    def get_name(cls, exec_inf_path: str) -> str:
        name_string = ""
        params = exec_inf_path.removesuffix(".csv").split("_")

        name = params[6]
        if name == "non":
            name = "NonStop"
            params.remove("stop")
        elif name == "async":
            name = "Async"
        elif name == "old":
            name = "Base"
        else:
            if params[6] == "1":
                return "SW"
            return "RR"

        name_string += name
        window = params[len(params) - 3]
        if window != "0":
            window = scnot(window)
            name_string += " W=" + window
        queue = params[len(params) - 2]
        if queue != "0":
            queue = scnot(queue)
            name_string += " Q=" + queue
        dp = params[len(params) - 1]
        if dp != "0":
            dp = scnot(dp)
            name_string += " " + "\u0394" + "p=" + dp

        return name_string

    def load_summary(self) -> None:
        for field in InfFields:
            self.summary[field] = -1

        values = load_csv(self.dir + self.details_path)
        self.summary[InfFields.SCHEDULING_END] = float(
            values[CSVFields.SCHEDULING_END.value][CSVFields.DATA.value]
        )
        self.summary[InfFields.MAKESPAN] = float(
            values[CSVFields.MAKESPAN.value][CSVFields.DATA.value]
        )
        self.summary[InfFields.ERROR_COUNT] = float(
            values[CSVFields.ERROR_COUNT.value][CSVFields.DATA.value]
        )
        try:
            self.summary[InfFields.REPARTITIONS] = (
                len(values) - CSVFields.DETAIL_BEGIN.value
            )
            graph_copy_durations = [
                float(values[i][CSVFields.GRAPH_COPY_DURATION.value])
                for i in range(CSVFields.DETAIL_BEGIN.value, len(values))
            ]
            self.summary[InfFields.GRAPH_COPY_DURATION_MEDIAN] = median(
                graph_copy_durations
            )
        except Exception as e:
            pass
            #print(e)

        down_time = 0
        under_ref_median = 0

        values = load_csv(self.dir + self.exec_inf_path)

        prev_excecuted = 0
        for i in range(CSVFields.EXEC_INF_BEGIN.value, len(values)):
            cols = len(values[i])

            executed = float(values[i][CSVFields.EXECUTED.value])
            throughput = executed - prev_excecuted
            prev_excecuted = executed
            self.throughputs.append(throughput)
            if throughput < self.ref_throughput:
                under_ref_median += 1
            if throughput == 0:
                down_time += 1

            try:
                queue_lenghts = [
                    float(values[i][q])
                    for q in range(CSVFields.QUEUED_BEGIN.value, cols - 1)
                ]
                if len(queue_lenghts) > 1:
                    queue_mean = mean(queue_lenghts)
                    queue_stddev = stdev(queue_lenghts)
                    queue_cv = queue_stddev / queue_mean if queue_mean > 0 else 0
                    self.queues_cvs.append(queue_cv)
            except Exception as e:
                #print('Missing queue lengths')
                #print(e)
                pass

        self.summary[InfFields.DOWN_TIME] = down_time / self.summary[InfFields.MAKESPAN]
        self.summary[InfFields.TIME_UNDER_REF_MEDIAN] = (
            under_ref_median / self.summary[InfFields.MAKESPAN]
        )

        self.summary[InfFields.THROUGHPUT_MEAN] = mean(self.throughputs)
        self.summary[InfFields.THROUGHPUT_LOW] = quantiles(self.throughputs, n=20)[1]
        self.summary[InfFields.THROUGHPUT_MEDIAN] = median(self.throughputs)
        self.summary[InfFields.THROUGHPUT_HIGH] = quantiles(self.throughputs, n=20)[18]

        self.summary[InfFields.THROUGHPUT_MAX] = max(self.throughputs)

        try:
            self.summary[InfFields.QUEUES_CV_MEAN] = mean(self.queues_cvs)
            self.summary[InfFields.QUEUES_CV_MEDIAN] = median(self.queues_cvs)
            self.summary[InfFields.QUEUES_CV_HIGH] = quantiles(self.queues_cvs, n=20)[
                18
            ]
        except Exception as e:
            pass
            #print(e)

    @classmethod
    def set_rank(cls, infs: List["Inf"]) -> None:
        for field in InfFields:
            infs.sort(key=lambda inf: inf.summary[field], reverse=False)
            for i in range(len(infs)):
                infs[i].rank[field] = i

    @classmethod
    def get_header(cls) -> List[str]:
        inf_fields = [field.name for field in InfFields]
        # rank_fields = [f"RANK_{field.name}" for field in InfFields]
        return inf_fields  # + rank_fields

    def get_list(self) -> List[str]:
        summary_list = [str(self.summary[field]) for field in InfFields]
        # rank_list = [str(self.rank[field]) for field in InfFields]
        return summary_list  # + rank_list


def query_pattern(
    details: bool | None = None,
    rate: int | None = None,
    initial: int | None = None,
    workload: str | None = None,
    method: str | None = None,
    partitions: int | None = None,
    version: str | None = None,
    window: int | None = None,
    queue: int | None = None,
    interval: int | None = None,
) -> Pattern:
    pattern: str = None
    if details is None or details is True:
        pattern = ""
    else:
        pattern = re.escape("details_")

    params: List[Any] = [
        rate,
        initial,
        workload,
        method,
        partitions,
        version,
        window,
        queue,
    ]

    for param in params:
        if param is not None:
            pattern += re.escape(f"{param}_")
        else:
            pattern += r".*" + re.escape("_")

    if interval is not None:
        pattern += re.escape(f"{interval}")
    else:
        pattern += r".*"

    return re.compile(pattern)


def summary_csv(infs: List[Inf], output_file: str) -> None:
    infs.sort(key=lambda inf: inf.exec_inf_path)

    summary_f = open(output_file, "w")
    summary_f.write("EXPERIMENT," + ",".join(Inf.get_header()) + linesep)
    for inf in infs:
        summary_f.write(f"{inf.name}," + ",".join(inf.get_list()) + linesep)

def throughput_csv(infs: List[Inf], output_file: str) -> None:
    throughput_f = open(output_file, "w")
    labels = [inf.name for inf in infs]
    throughput_f.write(",".join(labels) + linesep)

    inf_count = len(infs)
    inf_throughput_iters = [iter(inf.throughputs) for inf in infs]
    i = 0
    while True:
        done_count = 0
        throughputs = []
        for throughput_iter in inf_throughput_iters:
            try:
                throughputs.append(str(next(throughput_iter) / 1000))
            except StopIteration:
                done_count += 1
                throughputs.append("")

        throughput_f.write(",".join(throughputs) + linesep)
        if inf_count == done_count:
            break
        i += 1


ArrivalRate = int


def get_distinct_arrival_rates(files: List[str]) -> List[ArrivalRate]:
    arrival_rates: Set[ArrivalRate] = set()
    for file in files:
        arrival_rates.add(ArrivalRate(file.split("_")[0]))
    return sorted(arrival_rates)


def get_other_arrival_rate(inf: Inf, rate: ArrivalRate, ref: Inf = None) -> Inf:
    params = inf.exec_inf_path.split("_")
    params[0] = str(rate)
    exec_inf_path = "_".join(params)
    return Inf(exec_inf_path, ref)


def arrival_makespan_csv(
    arrival_rates: List[ArrivalRate], ref_inf: Inf, infs: List[Inf], output_file: str
) -> None:
    arrival_rate_infs: Dict[ArrivalRate, List[Inf]] = {}
    for arrival_rate in arrival_rates:
        arrival_rate_infs[arrival_rate] = []
        other_ref_inf = get_other_arrival_rate(ref_inf, arrival_rate)
        for inf in infs:
            other_inf = get_other_arrival_rate(inf, arrival_rate, other_ref_inf)
            arrival_rate_infs[arrival_rate].append(other_inf)
    
    throughput_f = open(output_file, "w")
    labels = [inf.name for inf in next(iter(arrival_rate_infs.values()))]
    throughput_f.write("," + ",".join(labels) + linesep)
    for arrival_rate, infs in arrival_rate_infs.items():
        infs = sorted(infs, key=lambda inf: inf.name)
        makespans = [str(inf.summary[InfFields.MAKESPAN]) for inf in infs]
        if arrival_rate == 0:
            arrival_rate = "inf"
        throughput_f.write(str(arrival_rate) + "," + ",".join(makespans) + linesep)


def export(workload: str, ref_file: str, exec_inf_files: List[str]) -> None:
    exec_inf_files = sorted(
        list(
            set(
                filter(
                    lambda f: query_pattern(workload=workload).match(f),
                    exec_inf_files,
                )
            )
        )
    )

    arrival_rates: List[ArrivalRate] = get_distinct_arrival_rates(exec_inf_files)

    exec_inf_files = sorted(
        list(
            set(
                filter(
                    lambda f: query_pattern(rate=0, workload=workload).match(f),
                    exec_inf_files,
                )
            )
        )
    )

    rr = set(
        filter(
            lambda f: query_pattern(workload=workload, method="ROUND_ROBIN").match(f),
            exec_inf_files,
        )
    )

    old = set(
        filter(
            lambda f: query_pattern(
                workload=workload, method="METIS", partitions=8, version="old"
            ).match(f),
            exec_inf_files,
        )
    )

    old_default = set(
        filter(
            lambda f: query_pattern(
                workload=workload,
                method="METIS",
                partitions=8,
                version="old",
                window=0,
                queue=0,
            ).match(f),
            exec_inf_files,
        )
    )

    old_no_window = set(
        filter(
            lambda f: query_pattern(
                workload=workload, method="METIS", partitions=8, version="old", window=0
            ).match(f),
            exec_inf_files,
        )
    )

    old_no_queue = set(
        filter(
            lambda f: query_pattern(
                workload=workload, method="METIS", partitions=8, version="old", queue=0
            ).match(f),
            exec_inf_files,
        )
    )

    old_window = old - old_no_window

    async_all = set(
        filter(
            lambda f: query_pattern(
                workload=workload,
                method="METIS",
                partitions=8,
                version="async",
            ).match(f),
            exec_inf_files,
        )
    )

    async_default = set(
        filter(
            lambda f: query_pattern(
                workload=workload,
                method="METIS",
                partitions=8,
                version="async",
                window=0,
                queue=0,
            ).match(f),
            exec_inf_files,
        )
    )

    async_non_stop_default = set(
        filter(
            lambda f: query_pattern(
                workload=workload,
                method="METIS",
                partitions=8,
                version="async",
                window=0,
                queue=0,
                interval=0
            ).match(f),
            exec_inf_files,
        )
    )

    async_no_window = set(
        filter(
            lambda f: query_pattern(
                workload=workload,
                method="METIS",
                partitions=8,
                version="async",
                window=0,
            ).match(f),
            exec_inf_files,
        )
    )
    async_window = async_all - async_no_window

    async_no_queue = set(
        filter(
            lambda f: query_pattern(
                workload=workload,
                method="METIS",
                partitions=8,
                version="async",
                queue=0,
            ).match(f),
            exec_inf_files,
        )
    )

    async_queue = async_all - async_no_queue

    rr_infs = [Inf(exec_inf_file) for exec_inf_file in rr]
    rr_min = min(rr_infs, key=lambda inf: inf.summary[InfFields.MAKESPAN])

    ref_inf = Inf(ref_file)

    all_infs = [Inf(exec_inf_file, ref_inf) for exec_inf_file in exec_inf_files]

    old_default_infs = [Inf(exec_inf_file, ref_inf) for exec_inf_file in old_default]
    old_default_min = min(
        old_default_infs, key=lambda inf: inf.summary[InfFields.MAKESPAN]
    )
    old_window_infs = [Inf(exec_inf_file, ref_inf) for exec_inf_file in old_window]
    old_window_lowest_makespan = min(
        old_window_infs, key=lambda inf: inf.summary[InfFields.MAKESPAN]
    )
    old_lowest_makespans = [
        old_default_min,
        old_window_lowest_makespan,
    ]

    old_lowest_makespan = min(
        old_lowest_makespans, key=lambda inf: inf.summary[InfFields.MAKESPAN]
    )
    async_default_infs = [
        Inf(exec_inf_file, ref_inf) for exec_inf_file in async_default
    ]
    async_non_stop_default_infs = [
        Inf(exec_inf_file, ref_inf) for exec_inf_file in async_non_stop_default
    ]


    async_window_infs = [
        Inf(exec_inf_file, ref_inf) for exec_inf_file in async_window
    ]

    async_queue_infs = [
        Inf(exec_inf_file, ref_inf) for exec_inf_file in async_queue
    ]

    window = (
        async_default_infs + async_window_infs + [old_lowest_makespan] + rr_infs
    )

    queue = (
        async_default_infs + async_queue_infs + [old_lowest_makespan] + rr_infs
    )

    async_queue_min = min(
        async_queue_infs, key=lambda inf: inf.summary[InfFields.MAKESPAN]
    )
    async_window_min = min(
        async_window_infs, key=lambda inf: inf.summary[InfFields.MAKESPAN]
    )
    async_default_min = min(
        async_default_infs, key=lambda inf: inf.summary[InfFields.MAKESPAN]
    )

    mins = async_non_stop_default_infs + [async_default_min, async_queue_min, async_window_min, old_default_min] + [ref_inf]

    summary_csv(window, f"{workload}_window_summary.csv")
    throughput_csv(window, f"{workload}_window_throughputs.csv")
    arrival_makespan_csv(arrival_rates, ref_inf, window, f"{workload}_window_rates.csv")

    summary_csv(queue, f"{workload}_queue_summary.csv")
    throughput_csv(queue, f"{workload}_queue_throughputs.csv")
    arrival_makespan_csv(arrival_rates, ref_inf, queue, f"{workload}_queue_rates.csv")

    summary_csv(mins, f"{workload}_mins_summary.csv")
    throughput_csv(mins, f"{workload}_mins_throughputs.csv")
    arrival_makespan_csv(arrival_rates, ref_inf, mins, f"{workload}_min_rates.csv")

    summary_csv(all_infs, f"{workload}_summary.csv")
    throughput_csv(all_infs, f"{workload}_throughputs.csv")
    arrival_makespan_csv(arrival_rates, ref_inf, queue, f"{workload}_rates.csv")



ycsb_a_ref_file = "0_1000000_ycsb_a_ROUND_ROBIN_8_old_0_0_1000000000.csv"
ycsb_d_ref_file = "0_1000000_ycsb_d_ROUND_ROBIN_8_old_0_0_1000000000.csv"
ycsb_e_ref_file = "0_1000000_ycsb_e_ROUND_ROBIN_1_old_0_0_1000000000.csv"

export("ycsb_a", ycsb_a_ref_file, exec_inf_files)
export("ycsb_d", ycsb_d_ref_file, exec_inf_files)
export("ycsb_e", ycsb_e_ref_file, exec_inf_files)
