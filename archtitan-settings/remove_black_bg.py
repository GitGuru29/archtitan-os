from PIL import Image

def remove_black_bg(input_path, output_path, tolerance=30):
    img = Image.open(input_path).convert("RGBA")
    data = img.getdata()
    new_data = []
    for item in data:
        # Check if the pixel is dark enough (close to black)
        if item[0] <= tolerance and item[1] <= tolerance and item[2] <= tolerance:
            new_data.append((0, 0, 0, 0)) # Fully transparent
        else:
            new_data.append(item)
    img.putdata(new_data)
    img.save(output_path, "PNG")

remove_black_bg("assets/icons/performance.png", "assets/icons/performance_nobg.png")
