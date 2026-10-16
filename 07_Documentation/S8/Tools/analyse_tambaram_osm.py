"""Offline topology audit of the retained ODbL reference, not route authoring.

Distances are horizontal WGS84 spherical approximations, not railway chainages.
No gaps are bridged and no service/platform assignment is inferred.
"""
import collections
import hashlib
import heapq
import json
import math
import xml.etree.ElementTree as ET
from pathlib import Path

ROOT = Path(__file__).resolve().parents[3]
OUT = ROOT / '06_Routes/ChennaiEgmoreTambaram/References/OSM'


def distance(a, b):
    lat1, lat2 = map(math.radians, (a['lat'], b['lat']))
    dlat = lat2-lat1
    dlon = math.radians(b['lon']-a['lon'])
    return 6371008.8 * 2 * math.asin(min(1, math.sqrt(
        math.sin(dlat/2)**2 + math.cos(lat1)*math.cos(lat2)*math.sin(dlon/2)**2)))


def main():
    source = OUT/'corridor-reference.json'
    data = json.loads(source.read_text(encoding='utf-8'))
    nodes = {int(k): v for k, v in data['nodes'].items()}
    stations = {}
    # Station POIs need not belong to a way, so inspect retained XML too.
    for record in data['sources']:
        raw = (OUT/record['file']).read_bytes()
        assert hashlib.sha256(raw).hexdigest() == record['sha256'], record['file']
        for element in ET.fromstring(raw).findall('node'):
            tags = {t.attrib['k']: t.attrib['v'] for t in element.findall('tag')}
            if tags.get('railway') in ('station', 'halt'):
                stations[int(element.attrib['id'])] = dict(
                    lat=float(element.attrib['lat']), lon=float(element.attrib['lon']), tags=tags)
    adjacency = collections.defaultdict(list)
    railway = {}
    for key, way in data['ways'].items():
        tags = way['tags']
        if tags.get('railway') != 'rail' or tags.get('service'):
            continue
        railway[int(key)] = way
        for a, b in zip(way['nodes'], way['nodes'][1:]):
            assert a in nodes and b in nodes, 'Incomplete source topology'
            length = distance(nodes[a], nodes[b])
            adjacency[a].append((b, length, int(key)))
            adjacency[b].append((a, length, int(key)))
    components, seen = [], set()
    for root in sorted(adjacency):
        if root in seen:
            continue
        pending, members, ways = [root], set(), set()
        seen.add(root)
        while pending:
            node = pending.pop()
            members.add(node)
            for neighbor, _, way in adjacency[node]:
                ways.add(way)
                if neighbor not in seen:
                    seen.add(neighbor)
                    pending.append(neighbor)
        components.append(dict(nodeCount=len(members), wayIds=sorted(ways),
            south=min(nodes[n]['lat'] for n in members), north=max(nodes[n]['lat'] for n in members)))
    station_report = []
    for key, station in sorted(stations.items(), key=lambda p: -p[1]['lat']):
        nearest = sorted(adjacency, key=lambda n: distance(station, nodes[n]))[:4]
        station_report.append(dict(osmNode=key, **station, nearestRailNodes=[
            dict(node=n, distanceMetres=round(distance(station, nodes[n]), 3),
                 ways=sorted({w for _, _, w in adjacency[n]})) for n in nearest]))
    # Enumerate bounded endpoint candidates, retaining exact OSM connectivity.
    # A shortest path is research evidence only, never an operational selection.
    origin = next(s for s in station_report if s['tags'].get('ref') == 'MS')
    destination = next(s for s in station_report if s['tags'].get('ref') == 'TBM')
    candidates = []
    for start in origin['nearestRailNodes']:
        costs, previous, queue = {start['node']: 0.0}, {}, [(0.0, start['node'])]
        while queue:
            cost, node = heapq.heappop(queue)
            if cost != costs[node]:
                continue
            for neighbor, length, way in adjacency[node]:
                next_cost = cost+length
                if next_cost < costs.get(neighbor, math.inf):
                    costs[neighbor] = next_cost
                    previous[neighbor] = (node, way)
                    heapq.heappush(queue, (next_cost, neighbor))
        for end in destination['nearestRailNodes']:
            if end['node'] not in costs:
                continue
            node, path, ways = end['node'], [end['node']], []
            while node != start['node']:
                node, way = previous[node]
                path.append(node)
                ways.append(way)
            path.reverse()
            ways.reverse()
            distinct_ways = list(dict.fromkeys(ways))
            candidates.append(dict(startNode=start['node'], endNode=end['node'],
                lengthMetres=round(costs[end['node']], 3), nodes=path, segmentWayIds=ways,
                ways=[dict(id=w, tags=railway[w]['tags']) for w in distinct_ways]))
    report = dict(attribution=data['attribution'], licence=data['licence'],
        copyrightUrl=data['copyrightUrl'], sourceSha256=hashlib.sha256(source.read_bytes()).hexdigest(),
        limitations=['Undirected topology audit; track direction and platform assignment are not verified.',
                    'Railway service ways excluded; no inferred or synthetic gap connections.',
                    'Nearest nodes are inspection candidates, not selected service endpoints.'],
        railwayWayCount=len(railway), components=sorted(components, key=lambda c: -c['nodeCount']),
        stations=station_report, candidatePaths=candidates)
    (OUT/'topology-audit.json').write_text(json.dumps(report, indent=2), encoding='utf-8')
    print(json.dumps(dict(railwayWays=len(railway), components=len(components),
        stations=[dict(name=s['tags'].get('name'), ref=s['tags'].get('ref'))
                  for s in station_report if s['tags'].get('network') == 'IR'],
        candidates=[dict(start=c['startNode'], end=c['endNode'], metres=c['lengthMetres'],
                         ways=len(c['ways'])) for c in candidates]), indent=2))


if __name__ == '__main__':
    main()
