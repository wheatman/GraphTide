import csv
import datetime

def process_csv(input_file, output_file):
    with open(input_file, 'r') as infile, open(output_file, 'w') as outfile:
        reader = csv.reader(infile)
        writer = csv.writer(outfile,  delimiter='\t')
        
        # Read and process each row
        for row in reader:
            if len(row) < 6:
                continue  # Skip invalid rows
            
            base_number = row[0]
            other_numbers = row[1:5]
            timestamp_str = row[5]
            
            # Parse the timestamp if needed
            try:
                timestamp = datetime.datetime.strptime(timestamp_str, "%Y%m%d%H%M")
            except ValueError:
                print(f"Skipping invalid date format: {timestamp_str}")
                continue
            
            # Write a row for each non-"None" element paired with the base number
            for num in other_numbers:
                if num != "None":
                    writer.writerow([base_number, num, int(timestamp.timestamp())])

# Example usage
process_csv('interactions.csv', 'bluesky.edges')