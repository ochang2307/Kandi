def relative_bearing(target_bearing, device_heading):
    relbear = (target_bearing-device_heading +360) % 360
    return relbear

def led_for_bearing(relative_brg):
    led_index = round(relative_brg / 45) % 8
    return led_index

    