from device import device_tick

# I'm at City Hall, facing due north
own_lat, own_lon, own_heading = 25.0330, 121.5654, 0

positions = {
    2: {"lat": 25.0430, "lon": 121.5654},   # ~1.1km due NORTH  -> LED 0, slow
    3: {"lat": 25.0330, "lon": 121.5754},   # ~1km due EAST     -> LED 2, slow
    4: {"lat": 25.0331, "lon": 121.5654},   # ~11m due north    -> LED 0, fast
}

for entry in device_tick(own_lat, own_lon, own_heading, positions):
    print(entry)

print("\n--- now twist wrist to face EAST (heading 90) ---")
for entry in device_tick(own_lat, own_lon, 90, positions):
    print(entry)