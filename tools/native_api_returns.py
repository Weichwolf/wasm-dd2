"""Read actual native API return observations; no reference-value substitution.

The provider writes clock values and computed RNG triples immediately before
returning. These records contain API counts and native caller PCs, not game
state at API returns. Frame/input state remains an independent observation.
"""
import struct

HEADER = b'DD2APIR1' + struct.pack('<I', 32)
RECORD = struct.Struct('<8I')


def returns(data):
    if not data.startswith(HEADER) or (len(data)-len(HEADER)) % RECORD.size:
        raise ValueError('Incomplete or unsupported native API return log')
    seed = 1
    clocks = randoms = 0
    for index, values in enumerate(RECORD.iter_unpack(data[len(HEADER):]), 1):
        kind, sequence, clock_count, random_count, before, after, value, caller = values
        if sequence != index or not caller:
            raise ValueError('Reordered API return or missing native caller')
        if kind == 1:
            clocks += 1
            if before or after:
                raise ValueError('Clock return contains unrelated RNG state')
            event = dict(kind='GetTickCount', value=value, caller=hex(caller))
        elif kind == 2:
            randoms += 1
            calculated = (seed * 1103515245 + 12345) & 0xffffffff
            if (before, after, value) != (seed, calculated, (calculated >> 16) & 32767):
                raise ValueError('Observed calculated RNG state/return differs')
            seed = after
            event = dict(kind='rand', before=before, after=after, result=value, caller=hex(caller))
        else:
            raise ValueError('Unknown native API return kind')
        if (clock_count, random_count) != (clocks, randoms):
            raise ValueError('Observed API return counts differ')
        yield dict(event, api_sequence=sequence, clock_calls=clocks, rng_calls=randoms)


def merge(boundaries, api_returns):
    """Order actual boundaries after their last completed API return.

    API values come solely from the provider log. Boundary states come solely
    from the debugger; no state is attached to unobserved API return points.
    """
    events = []
    previous = 0
    for boundary in boundaries:
        position = boundary['clock_calls'] + boundary['rng_calls']
        if not previous <= position <= len(api_returns):
            raise ValueError('Boundary outside the observed API return stream')
        completed = api_returns[position-1] if position else dict(clock_calls=0, rng_calls=0)
        if any(boundary[key] != completed[key] for key in ('clock_calls', 'rng_calls')):
            raise ValueError('Boundary moved to a different API return point')
        events.extend(api_returns[previous:position])
        events.append(boundary)
        previous = position
    events.extend(api_returns[previous:])
    return [dict(event, index=index) for index, event in enumerate(events, 1)]
