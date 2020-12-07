import argparse
import sys
import orjson

sys.setrecursionlimit(100000)

from flask import (
    Flask,
    request
)

app = Flask(__name__)


def foo(track, current, end, tot=0):
    if current == end:
        return tot
    tot += track[current]
    return foo(track, current+1, end, tot)

def solve1(data):
    res = []
    for start, end in data['items']:
        if start > end:
            res.append(foo(data['track'], end, start))
        else:
            res.append(foo(data['track'], start, end))
    return res

def solve2(items, track):
    mins = 9999999999
    maxs = 0
    for start, end in items:
        mins = min(mins, start, end)
        maxs = max(maxs, start, end)
    track_cumsum = np.cumsum(np.array([0] + track[mins:maxs+1]))
    return [abs(track_cumsum[end - mins] - track_cumsum[start - mins]) for start, end in items]

@app.route('/microchallenge', methods= ['POST'])
def microchallenge():
    data = orjson.loads(request.get_data())
    res = []
    if len(data['track']) <= 1500:
        res = solve1(data)
    else:
        res = solve2(data['items'], data['tracks'])
    return str(res)

if __name__ == '__main__':
    parser = argparse.ArgumentParser(description='Microchallenge Starter Pack')
    parser.add_argument('-p', metavar='p', type=int, default=27178)

    args = parser.parse_args()
    app.run(port=args.p, host="0.0.0.0")