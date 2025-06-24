import csv
import json

INPUT_CSV = "../src/moniteur/moniteur_log.csv"
OUTPUT_JS = "data.js"

def convert_csv_to_js():
    data = []
    with open(INPUT_CSV, newline='') as csvfile:
        reader = csv.reader(csvfile)
        for row in reader:
            if len(row) == 3 and row[1] == "METRIC":
                try:
                    value = float(row[2])
                    data.append({
                        "time": row[0],
                        "value": value,
                        "alert": value > 75.0
                    })
                except ValueError:
                    continue

    with open(OUTPUT_JS, "w") as jsfile:
        jsfile.write("const cpuData = ")
        jsfile.write(json.dumps(data, indent=2))
        jsfile.write(";\n")

if __name__ == "__main__":
    convert_csv_to_js()
    print(f"[OK] {OUTPUT_JS} généré à partir de {INPUT_CSV}")
