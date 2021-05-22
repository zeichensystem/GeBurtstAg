import re
import sys

def convert(rgb):
	match = re.search(r"(\d+)\D*(\d+)\D*(\d+)", rgb)
	if match:
		r = int(match.group(1))
		g = int(match.group(2))
		b = int(match.group(3))

		r = round(r / 255.0 * 31)
		g = round(g / 255.0 * 31)
		b = round(b / 255.0 * 31)
		
		print(f"RGB15_SAFE({r}, {g}, {b})") 

if __name__ == "__main__":
	if len(sys.argv) > 1:
		convert(sys.argv[1])
	else:
		print("Usage: rgb2rgb15 rgbstring")
