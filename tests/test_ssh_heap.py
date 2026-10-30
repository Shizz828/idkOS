import importlib.util
import pathlib
import unittest

path = pathlib.Path(__file__).resolve().parents[1] / "scripts/check_ssh_heap.py"
spec = importlib.util.spec_from_file_location("ssh_heap", path)
module = importlib.util.module_from_spec(spec)
spec.loader.exec_module(module)


def line(attempt, stage, free=100000, largest=64000, sessions=0, dns=0, https=0):
    return (f"[ssh] attempt={attempt} {stage} free={free} largest={largest} minimum=123 "
            f"stack_free_min=999 sessions={sessions} channels=0 sockets=0 dns={dns} https={https}\n")


class HeapLogTests(unittest.TestCase):
    def test_actual_values_and_warmup(self):
        records = module.parse(["unrelated\n", line(1, "settled-idle", 120000),
                                line(3, "settled-idle"), line(4, "settled-idle", 99900, 63000)])
        output, clean = module.report(records)
        self.assertTrue(clean)
        self.assertIn("Settled free RAM: 100000 -> 99900 B (delta -100 B).", output)
        self.assertIn("Settled largest block: 64000 -> 63000 B (delta -1000 B).", output)

    def test_retained_resource(self):
        _, clean = module.report(module.parse([line(1, "after-failed-cleanup", sessions=1)]))
        self.assertFalse(clean)

    def test_bounded_late_dns(self):
        _, clean = module.report(module.parse([line(1, "after-disconnect-cleanup", dns=2)]))
        self.assertTrue(clean)
        _, clean = module.report(module.parse([line(1, "after-disconnect-cleanup", dns=3)]))
        self.assertFalse(clean)
        _, clean = module.report(module.parse([line(1, "settled-idle", dns=1)]))
        self.assertFalse(clean)

    def test_unfinished_attempt(self):
        _, clean = module.report(module.parse([line(1, "before-start")]))
        self.assertFalse(clean)

    def test_missing_data(self):
        _, clean = module.report(module.parse(["[ssh] attempt=1 before-start free=99\n"]))
        self.assertFalse(clean)


if __name__ == "__main__":
    unittest.main()
