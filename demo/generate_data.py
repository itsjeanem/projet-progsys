import csv
import json
from collections import defaultdict

INPUT_CSV = "../src/moniteur/moniteur_log.csv"
OUTPUT_JS = "data.js"

def convert_csv_to_js():
    data_by_time = defaultdict(dict)

    with open(INPUT_CSV, newline='') as csvfile:
        reader = csv.reader(csvfile)
        for row in reader:
            if len(row) == 4 and row[1] == "METRIC":
                timestamp, _, metric_type, value = row
                try:
                    value = float(value)
                    data_by_time[timestamp][metric_type] = value
                except ValueError:
                    continue

    dataset = []
    for time, metrics in sorted(data_by_time.items()):
        dataset.append({
            "time": time,
            "CPU": metrics.get("CPU", 0.0),
            "RAM": metrics.get("RAM", 0.0),
            "LOAD": metrics.get("LOAD", 0.0),
            "alert": metrics.get("CPU", 0.0) > 75.0
        })

    with open(OUTPUT_JS, "w") as jsfile:
        jsfile.write("const metricsData = ")
        jsfile.write(json.dumps(dataset, indent=2))
        jsfile.write(";\n")

if __name__ == "__main__":
    convert_csv_to_js()
    print(f"[OK] {OUTPUT_JS} généré.")
