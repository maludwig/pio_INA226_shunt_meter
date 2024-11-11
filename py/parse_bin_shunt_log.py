import argparse
import csv

from py.row_reader import COLUMNS, RowReader

for x in range(1, 6):
    COLUMNS.append(f"bus_voltage_{x}")
    COLUMNS.append(f"shunt_voltage_{x}")
    COLUMNS.append(f"current_{x}")
    COLUMNS.append(f"power_{x}")


def parse_args():
    parser = argparse.ArgumentParser(description="Process some integers.")
    parser.add_argument("--input-file", help="The binary file to process")
    args = parser.parse_args()
    args.output_file = args.input_file.replace(".bin0", ".csv")
    return args


"/Users/mitchellludwig/Downloads/20230723T124600.csv"


def process_file(input_file, output_file):
    with open(input_file, "rb") as f_in:
        with open(output_file, "w", newline="") as f_out:
            writer = csv.DictWriter(f_out, fieldnames=COLUMNS)
            writer.writeheader()
            reader = RowReader(f_in)
            for row in reader.rows():
                print(row["timestamp"])
                writer.writerow(row)


def main():
    args = parse_args()
    process_file(args.input_file, args.output_file)


if __name__ == "__main__":
    main()
