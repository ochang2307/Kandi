import math

def distance(lat1, lon1, lat2, lon2):
    r = 6371000 #earth radius in meters
    #convert deg -> radians
    
    lat1r = math.radians(lat1)
    lon1r = math.radians(lon1)
    lat2r = math.radians(lat2)
    lon2r = math.radians(lon2)

    dlat = lat2r - lat1r  #delta latitudes
    dlon = lon2r - lon1r  #delta longitudes

    #from haversine formula for distance
    a = (math.sin(dlat/2)*math.sin(dlat/2)) + (math.cos(lat1r)*math.cos(lat1r)*math.sin(dlon/2)*math.sin(dlon/2))
    c = 2*math.atan2(math.sqrt(a),math.sqrt(1-a))
    d = r*c

    return d


def bearing(lat1, lon1, lat2, lon2):
    #convert deg -> radians
    
    lat1r = math.radians(lat1)
    lon1r = math.radians(lon1)
    lat2r = math.radians(lat2)
    lon2r = math.radians(lon2)

    
    dlon = lon2r - lon1r  #delta longitudes

    theta = math.atan2(math.sin(dlon)*math.cos(lat2r),
                       math.cos(lat1r)*math.sin(lat2r)-math.sin(lat1r)
                       *math.cos(lat2r)*math.cos(dlon))
    thetadeg = math.degrees(theta)
    ans = (thetadeg +360 ) % 360
    return ans