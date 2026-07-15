import math
from tilt_compensation import pitch_roll, tilt_compensate, heading_from
# ^ adjust "compass" to whatever you named the file with your 3 functions


def generate_readings(pitch_deg, roll_deg, heading_deg):
    """
    Given a device orientation, produce the (ax,ay,az) and (mx,my,mz)
    a perfect sensor would report. This is the REVERSE of the algorithm:
    we start from a known truth and rotate the reference vectors INTO the
    tilted device frame. Feeding these back through your functions should
    recover the heading we started with.
    """
    # work in radians internally
    pitch = math.radians(pitch_deg)
    roll  = math.radians(roll_deg)
    heading = math.radians(heading_deg)

    # --- Accelerometer: measures gravity (points straight DOWN in the world) ---
    # In a level device, gravity is (0, 0, +1) on its axes (using g=1 for simplicity;
    # magnitude doesn't matter, only direction). Rotating the world "down" vector
    # into a device tilted by pitch/roll gives what each axis reads:
    ax = math.sin(pitch)
    ay = -math.sin(roll) * math.cos(pitch)
    az = math.cos(roll) * math.cos(pitch)

    # --- Magnetometer: measures Earth's field ---
    # Simplify: assume the field points toward magnetic north and horizontal
    # (ignore vertical "dip" component for the test). In the WORLD frame, a device
    # with some heading sees north split across its forward/left axes:
    #   forward component  =  cos(heading)
    #   left component     = -sin(heading)
    # Then we tilt that horizontal field into the device frame using pitch/roll,
    # the inverse of your tilt_compensate step:
    mx_level = math.cos(heading)
    my_level = -math.sin(heading)
    mz_level = 0.0

    # rotate the level field into the tilted device frame
    mx = mx_level * math.cos(pitch) + mz_level * math.sin(pitch)
    my = (mx_level * math.sin(roll) * math.sin(pitch)
          + my_level * math.cos(roll)
          - mz_level * math.sin(roll) * math.cos(pitch))
    mz = (-mx_level * math.cos(roll) * math.sin(pitch)
          + my_level * math.sin(roll)
          + mz_level * math.cos(roll) * math.cos(pitch))

    return (ax, ay, az), (mx, my, mz)

def run_pipeline(ax, ay, az, mx, my, mz):
    """Your actual algorithm, chained: sensors in, heading out."""
    pitch, roll = pitch_roll(ax, ay, az)
    Xh, Yh = tilt_compensate(mx, my, mz, pitch, roll)
    return heading_from(Xh, Yh)


def check(pitch_deg, roll_deg, heading_deg):
    accel, mag = generate_readings(pitch_deg, roll_deg, heading_deg)
    result = run_pipeline(*accel, *mag)

    # compare result to the heading we put in, handling wraparound
    # (e.g. 359.9 vs 0 should count as "close")
    diff = abs((result - heading_deg + 180) % 360 - 180)
    ok = "OK" if diff < 0.5 else "FAIL"
    print(f"[{ok}] pitch={pitch_deg:>4}  roll={roll_deg:>4}  "
          f"in={heading_deg:>5}  out={result:7.2f}  err={diff:.3f}")


print("--- Level device (no tilt): pure heading recovery ---")
for h in [0, 45, 90, 135, 180, 225, 270, 315]:
    check(0, 0, h)

print("\n--- Pitched only ---")
for h in [0, 90, 180, 270]:
    check(20, 0, h)

print("\n--- Rolled only ---")
for h in [0, 90, 180, 270]:
    check(0, 15, h)

print("\n--- Both pitch and roll (the real test) ---")
for h in [0, 45, 90, 135, 180, 225, 270, 315]:
    check(25, 15, h)

print("\n--- Extreme tilt ---")
check(40, 30, 60)
check(-35, -20, 200)