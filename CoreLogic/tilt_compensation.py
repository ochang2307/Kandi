import math

def pitch_roll(ax,ay,az):
    pitch = math.atan2(-ax,math.sqrt((ay*ay)+(az*az)))
    roll = math.atan2(ay,az)
    return pitch, roll

def tilt_compensate(mx, my, mz, pitch, roll):
    Xh = (mx * math.cos(pitch)
          + my * math.sin(roll) * math.sin(pitch)
          + mz * math.cos(roll) * math.sin(pitch))
    Yh = (my * math.cos(roll)
          - mz * math.sin(roll))
    return Xh, Yh

def heading_from(Xh, Yh):
    heading = math.atan2(-Yh, Xh)
    heading = math.degrees(heading)
    heading = (heading + 360) % 360
    return heading
