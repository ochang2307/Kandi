from navigation import distance, bearing
from Led_Logic import relative_bearing, led_for_bearing


def blink_rate_for(dist_m):
    # distance -> blink rate, per the LED UX design
    if dist_m < 10:
        return "arrived"
    elif dist_m < 50:
        return "fast"
    elif dist_m < 200:
        return "medium"
    else:
        return "slow"


def device_tick(own_lat, own_lon, own_heading, positions):
    # positions: dict of member_id -> {"lat": ..., "lon": ...}
    # returns one display entry per member
    display = []
    for member_id, pos in positions.items():
        d = distance(own_lat, own_lon, pos["lat"], pos["lon"])
        b = bearing(own_lat, own_lon, pos["lat"], pos["lon"])
        rb = relative_bearing(b, own_heading)
        led = led_for_bearing(rb)
        rate = blink_rate_for(d)
        display.append({
            "member": member_id,
            "led": led,
            "blink": rate,
            "distance": round(d, 1),
        })
    return display