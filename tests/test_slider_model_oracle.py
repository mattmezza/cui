#!/usr/bin/env python3
"""Independent Decimal oracle over exact binary64 inputs; no C helper formulas."""
import argparse
from decimal import Decimal, localcontext, ROUND_HALF_UP
import math
import json
import subprocess
import sys


def D(value):
    return Decimal.from_float(value)


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--probe', required=True)
    args = parser.parse_args()
    cases = []
    tiny = math.ulp(0.)
    maximum = sys.float_info.max
    # Decimal keeps enough digits for the entire binary64 exponent span, including
    # exact tiny-step lattice phase after subtracting a near-maximum origin.
    with localcontext() as ctx:
        ctx.prec = 1200
        snap_domains = [(-3., 7., 2.), (-1., 1., .1), (10., 20., .3),
                        (1e15, 1e15 + 8, .3), (-1e308, 0., tiny),
                        (0., maximum, tiny), (tiny, tiny * 19, tiny * 3),
                        (1e308, math.nextafter(1e308, math.inf), 1e290),
                        (tiny, 2., 1.), (-tiny, 2., 1.), (-.75, 2., 1.),
                        (tiny, sys.float_info.min * 4, sys.float_info.min + tiny),
                        (-tiny, sys.float_info.min * 4, sys.float_info.min + tiny)]
        for lo, hi, step in snap_domains:
            xs = [min(hi, max(lo, lo * (1-t) + hi * t))
                  for t in (0., .125, .25, .5, .75, .875, 1.)]
            xs += [x for x in (.5, math.nextafter(.5, 0), math.nextafter(.5, 1),
                               1.5, math.nextafter(1.5, 0), math.nextafter(1.5, 2))
                   if lo <= x <= hi]
            for x in xs:
                if x in (lo, hi):
                    expected = x
                else:
                    n = ((D(x) - D(lo)) / D(step)).to_integral_value(rounding=ROUND_HALF_UP)
                    expected = float(min(D(hi), max(D(lo), D(lo) + n * D(step))))
                cases.append((1, lo, hi, step, 0, 0, math.nan, lo, 1, x,
                              expected, 'exact_snap' if (x in (lo, hi) or
                                                       (abs(lo) == tiny and x == .5)) else 'snap'))
        # Independent Range oracle clamps exact lattice result to off-grid neighbor.
        cases.extend([(1, 0., 1., .25, 0, 1, .3, .7, 0, .9, .7, 'start'),
                      (1, 0., 1., .25, 0, 1, .3, .7, 1, .1, .3, 'snap')])
    with localcontext() as ctx:
        ctx.prec = 180
        logs = [(tiny, maximum), (1e308, math.nextafter(1e308, math.inf)),
                (1e308, math.nextafter(math.nextafter(1e308, math.inf), math.inf)),
                (1e308, 1.5e308), (tiny, tiny * 17), (.01, 1000.)]
        for lo, hi in logs:
            for t in (0., .001, .125, .25, .5, .75, .875, .999, 1.):
                # Weighted geometric mean: independent from log1p/branch/anchor C math.
                exact = ((1-D(t))*D(lo).ln() + D(t)*D(hi).ln()).exp()
                expected = lo if t == 0 else hi if t == 1 else float(exact)
                label = 'exact_inverse' if t in (0., 1.) else (
                    'inverse_close' if hi <= sys.float_info.min or hi-lo <= 4*math.ulp(lo) else 'inverse')
                cases.append((3, lo, hi, 0., 1, 0, 0., lo, 1, t, expected, label))
            for x in (lo, math.nextafter(lo, hi), lo + (hi-lo)*.25,
                      lo + (hi-lo)*.75, math.nextafter(hi, lo), hi):
                expected = 0. if x == lo else 1. if x == hi else float(
                    (D(x)/D(lo)).ln() / (D(hi)/D(lo)).ln())
                cases.append((2, lo, hi, 0., 1, 0, 0., lo, 1, x, expected, 'fraction'))
    with localcontext() as ctx:
        ctx.prec = 1200
        for lo, hi in [(-maximum/2, maximum/2), (1e308, math.nextafter(1e308, math.inf)),
                       (-17., -17.), (tiny, tiny*17)]:
            for t in (0., .125, .5, .875, 1.):
                expected = float(D(lo) + D(t)*(D(hi)-D(lo)))
                cases.append((3, lo, hi, 0., 0, 0, 0., lo, 1, t, expected, 'linear'))
            for x in (lo, lo + (hi-lo)*.25, lo + (hi-lo)*.75, hi):
                expected = 0. if lo == hi else float((D(x)-D(lo))/(D(hi)-D(lo)))
                cases.append((2, lo, hi, 0., 0, 0, 0., lo, 1, x, expected, 'linear'))
    lines = [' '.join(str(x) if isinstance(x, int) else x.hex() for x in case[:10])
             for case in cases]
    process = subprocess.run([args.probe, '--probe'], input='\n'.join(lines)+'\n',
                             text=True, capture_output=True, check=True)
    results = process.stdout.splitlines()
    assert len(results) == len(cases), (len(results), len(cases), process.stderr)
    worst = {}
    worst_cases = {}
    exact_counterexamples = []
    for case, result in zip(cases, results):
        fields = result.split()
        assert fields[0] == '1', (case, result)
        actual = float.fromhex(fields[1 if case[-1] == 'start' else -1])
        expected, label = case[-2:]
        # Binary64 transcendental implementations need not be bit-identical.
        # Snapping/linear algebra are much tighter; extreme log interpolation's
        # error is bounded in value ULPs, not hidden by a span-relative tolerance.
        ulps = (0 if label.startswith('exact_') else 512 if label == 'inverse' else
                1 if label == 'inverse_close' else 8 if label == 'fraction' else 2)
        error = abs(actual-expected) / math.ulp(expected)
        if error > worst.get(label, -1):
            worst_cases[label] = {'minimum': case[1].hex(), 'maximum': case[2].hex(),
                                  'input': case[9].hex(), 'actual': actual.hex(),
                                  'expected': expected.hex()}
        worst[label] = max(worst.get(label, 0), error)
        if case[0] == 1 and case[3] == 1 and abs(case[1]) == tiny and case[9] == .5:
            counterexample = {'minimum': case[1].hex(), 'requested': case[9].hex(),
                              'actual': actual.hex(), 'expected': expected.hex()}
            if counterexample not in exact_counterexamples:
                exact_counterexamples.append(counterexample)
        assert math.isfinite(actual) and abs(actual-expected) <= ulps * math.ulp(expected), (
            label, case[:10], actual.hex(), expected.hex())
    print(json.dumps({'cases': len(cases), 'decimal_snap_precision': 1200,
                      'decimal_log_precision': 180, 'decimal_linear_precision': 1200,
                      'maximum_observed_ulps': worst, 'worst_cases': worst_cases,
                      'exact_counterexamples': exact_counterexamples}, sort_keys=True))


if __name__ == '__main__':
    main()
