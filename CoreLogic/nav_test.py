from navigation import distance, bearing
#Distance Tests
# Test 1: zero distance — same point should return ~0
d = distance(25.0330, 121.5654, 25.0330, 121.5654)
print("Same point distance:", d, "(expect ~0)")

# Test 2: a known pair — your location to Taipei 101
# (look up both coordinates in Google Maps, plug them in)
d = distance( 25.033264086415144 , 121.56486801276817 , 25.04448385356689 , 121.54846663457496 )  
print("To Taipei 101:", d, "meters (2090 on gmaps)")

# Test 3: due north — same longitude, higher latitude
# pick a start point, then add ~0.01 to the latitude only
#d = distance( ... )
#print("Due north test:", d)






#Bearing Tests
# A reference starting point (Taipei City Hall area)
start_lat = 25.0330
start_lon = 121.5654

# Test 1: due NORTH — same longitude, higher latitude -> expect ~0 (or ~360)
b = bearing(start_lat, start_lon, start_lat + 0.01, start_lon)
print("Due north:", b, "(expect ~0 or ~360)")

# Test 2: due EAST — same latitude, higher longitude -> expect ~90
b = bearing(start_lat, start_lon, start_lat, start_lon + 0.01)
print("Due east: ", b, "(expect ~90)")

# Test 3: due SOUTH — same longitude, lower latitude -> expect ~180
b = bearing(start_lat, start_lon, start_lat - 0.01, start_lon)
print("Due south:", b, "(expect ~180)")

# Test 4: due WEST — same latitude, lower longitude -> expect ~270
b = bearing(start_lat, start_lon, start_lat, start_lon - 0.01)
print("Due west: ", b, "(expect ~270)")

# Test 5: NORTHEAST — both higher -> expect ~45 (roughly; see note below)
b = bearing(start_lat, start_lon, start_lat + 0.01, start_lon + 0.01)
print("Northeast:", b, "(expect ~45, slightly off — see note)")

# Test 6: real pair — your start to Taipei 101 (NE of City Hall) -> expect ~30-50ish
b = bearing(start_lat, start_lon, 25.04448385356689, 121.54846663457496)
print("To Taipei 101:", b, "(expect a NW-ish bearing — sanity check the direction)")
