def reformat_movie_data(input_file, output_file):
    try:
        with open(input_file, 'r') as infile, open(output_file, 'w') as outfile:
            for line in infile:
                # Remove whitespace and split by the double-colon delimiter
                parts = line.strip().split('\t')
                
                # Check if the line has the expected 4 parts
                if len(parts) >= 2:
                    user_id = parts[0]
                    movie_id = parts[1]
                    
                    # Write the new format: "MovieID UserID"
                    outfile.write(f"{movie_id} {user_id}\n")
        
        print(f"Successfully processed! Check '{output_file}' for the results.")
        
    except FileNotFoundError:
        print("Error: The input file was not found.")
    except Exception as e:
        print(f"An error occurred: {e}")

# Usage
reformat_movie_data('u.data', 'formatted_ratings.txt')